#include "WebUIRuntimeBrowserWidget.h"

#include "Components/Overlay.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "TimerManager.h"
#include "WebBrowser.h"
#include "WebUIRuntime.h"
#include "WebUIRuntimeSettings.h"
#include "WebUIRuntimeSubsystem.h"

namespace
{
	constexpr int32 MaxLoadRetries = 10;
	constexpr float LoadRetryDelaySeconds = 0.5f;

	FString UrlEncodeQueryValue(const FString& Value)
	{
		FTCHARToUTF8 Converted(*Value);
		FString Encoded;
		Encoded.Reserve(Value.Len() * 3);

		for (int32 Index = 0; Index < Converted.Length(); ++Index)
		{
			const uint8 Byte = static_cast<uint8>(Converted.Get()[Index]);
			const bool bIsUnreserved =
				(Byte >= 'A' && Byte <= 'Z') ||
				(Byte >= 'a' && Byte <= 'z') ||
				(Byte >= '0' && Byte <= '9') ||
				Byte == '-' ||
				Byte == '_' ||
				Byte == '.' ||
				Byte == '~';

			if (bIsUnreserved)
			{
				Encoded.AppendChar(static_cast<TCHAR>(Byte));
			}
			else
			{
				Encoded += FString::Printf(TEXT("%%%02X"), Byte);
			}
		}

		return Encoded;
	}
}

void UWebUIRuntimeBrowserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureBrowserWidget();
	ApplyBrowserSettings();

	if (bAutoLoadOnConstruct && !IsDesignTime())
	{
		LoadWebUI();
	}
}

void UWebUIRuntimeBrowserWidget::NativeDestruct()
{
	ClearLoadRetryTimer();
	Super::NativeDestruct();
}

TSharedRef<SWidget> UWebUIRuntimeBrowserWidget::RebuildWidget()
{
	UWidgetTree* Tree = WidgetTree.Get();
	const bool bHadWidgetTreeRoot = Tree && Tree->RootWidget;
	EnsureBrowserWidget();
	ApplyBrowserSettings();

	Tree = WidgetTree.Get();
	if (NativeRootOverlay || (bHadWidgetTreeRoot && Tree && Cast<UPanelWidget>(Tree->RootWidget)))
	{
		return Super::RebuildWidget();
	}

	if (WebBrowser)
	{
		return WebBrowser->TakeWidget();
	}

	return Super::RebuildWidget();
}

void UWebUIRuntimeBrowserWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	ApplyBrowserSettings();
}

void UWebUIRuntimeBrowserWidget::LoadWebUI()
{
	ClearLoadRetryTimer();
	EnsureBrowserWidget();
	ApplyBrowserSettings();
	TryLoadWebUI(0);
}

void UWebUIRuntimeBrowserWidget::ReloadWebUI()
{
	LoadWebUI();
}

void UWebUIRuntimeBrowserWidget::SetWebUIId(const FString& InWebUIId)
{
	WebUIId = InWebUIId;
	OnWidgetStateChanged();
}

void UWebUIRuntimeBrowserWidget::SetUseEmbedMode(bool bInUseEmbedMode)
{
	bUseEmbedMode = bInUseEmbedMode;
	OnWidgetStateChanged();
}

void UWebUIRuntimeBrowserWidget::SetOverrideURL(const FString& InOverrideURL)
{
	OverrideURL = InOverrideURL.TrimStartAndEnd();
	OnWidgetStateChanged();
}

FString UWebUIRuntimeBrowserWidget::GetWebUIURL() const
{
	return BuildWebUIURL();
}

void UWebUIRuntimeBrowserWidget::EnsureBrowserWidget()
{
	if (WebBrowser)
	{
		return;
	}

	UWidgetTree* Tree = WidgetTree.Get();
	if (!Tree)
	{
		return;
	}

	if (!Tree->RootWidget)
	{
		NativeRootOverlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("WebUIRuntimeRootOverlay"));
		Tree->RootWidget = NativeRootOverlay;
	}

	if (UPanelWidget* Panel = Cast<UPanelWidget>(Tree->RootWidget))
	{
		WebBrowser = Tree->ConstructWidget<UWebBrowser>(UWebBrowser::StaticClass(), TEXT("WebUIRuntimeBrowser"));
		Panel->AddChild(WebBrowser);
	}
	else if (!WebBrowser)
	{
		WebBrowser = NewObject<UWebBrowser>(this, UWebBrowser::StaticClass(), TEXT("WebUIRuntimeBrowser"));
	}
}

void UWebUIRuntimeBrowserWidget::ApplyBrowserSettings()
{
	if (!WebBrowser)
	{
		return;
	}

	FBoolProperty* TransparencyProperty = FindFProperty<FBoolProperty>(UWebBrowser::StaticClass(), TEXT("bSupportsTransparency"));
	if (!TransparencyProperty)
	{
		TransparencyProperty = FindFProperty<FBoolProperty>(UWebBrowser::StaticClass(), TEXT("SupportsTransparency"));
	}

	if (TransparencyProperty)
	{
		TransparencyProperty->SetPropertyValue_InContainer(WebBrowser, bSupportsTransparency);
	}

	UE_LOG(LogWebUIRuntime, Verbose, TEXT("WebUI browser widget prepared. URL=%s"), *BuildWebUIURL());
}

void UWebUIRuntimeBrowserWidget::TryLoadWebUI(int32 RetryCount)
{
	const FString URL = BuildWebUIURL();
	if (URL.IsEmpty())
	{
		UE_LOG(LogWebUIRuntime, Warning, TEXT("WebUI URL is empty, cannot load browser widget."));
		return;
	}

	if (!WebBrowser)
	{
		UE_LOG(LogWebUIRuntime, Warning, TEXT("WebBrowser widget is not ready yet. URL=%s"), *URL);
		if (RetryCount < MaxLoadRetries)
		{
			ScheduleLoadRetry(RetryCount + 1, LoadRetryDelaySeconds);
		}
		return;
	}

	const bool bShouldWaitForServer = OverrideURL.IsEmpty();
	if (bShouldWaitForServer)
	{
		if (const UWebUIRuntimeSubsystem* Runtime = GetRuntimeSubsystem())
		{
			if (!Runtime->IsServerRunning())
			{
				UE_LOG(LogWebUIRuntime, Warning, TEXT("WebUI HTTP server is not running yet. Retrying browser load: %s"), *URL);
				if (RetryCount < MaxLoadRetries)
				{
					ScheduleLoadRetry(RetryCount + 1, LoadRetryDelaySeconds);
				}
				return;
			}
		}
		else
		{
			UE_LOG(LogWebUIRuntime, Warning, TEXT("WebUI runtime subsystem was not found. Loading browser URL anyway: %s"), *URL);
		}
	}

	WebBrowser->LoadURL(URL);
	UE_LOG(LogWebUIRuntime, Log, TEXT("WebUI browser loading URL: %s"), *URL);
}

void UWebUIRuntimeBrowserWidget::ScheduleLoadRetry(int32 RetryCount, float DelaySeconds)
{
	if (RetryCount > MaxLoadRetries)
	{
		UE_LOG(LogWebUIRuntime, Warning, TEXT("WebUI browser load failed after %d retries."), MaxLoadRetries);
		return;
	}

	if (UWorld* World = GetWorld())
	{
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUObject(this, &UWebUIRuntimeBrowserWidget::TryLoadWebUI, RetryCount);
		World->GetTimerManager().SetTimer(LoadRetryTimerHandle, TimerDelegate, DelaySeconds, false);
	}
	else
	{
		UE_LOG(LogWebUIRuntime, Warning, TEXT("No world is available for WebUI browser retry scheduling."));
	}
}

void UWebUIRuntimeBrowserWidget::ClearLoadRetryTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LoadRetryTimerHandle);
	}
}

void UWebUIRuntimeBrowserWidget::OnWidgetStateChanged()
{
	if (WebBrowser && !IsDesignTime())
	{
		ReloadWebUI();
	}
}

UWebUIRuntimeSubsystem* UWebUIRuntimeBrowserWidget::GetRuntimeSubsystem() const
{
	if (const UWorld* World = GetWorld())
	{
		return World->GetSubsystem<UWebUIRuntimeSubsystem>();
	}
	return nullptr;
}

FString UWebUIRuntimeBrowserWidget::BuildWebUIURL() const
{
	const FString TrimmedOverrideURL = OverrideURL.TrimStartAndEnd();
	if (!TrimmedOverrideURL.IsEmpty())
	{
		return TrimmedOverrideURL;
	}

	FString URL = BuildBaseWebUIURL();
	if (bUseEmbedMode)
	{
		AppendQueryParameter(URL, TEXT("embed"), TEXT("1"));
	}
	if (!WebUIId.IsEmpty())
	{
		AppendQueryParameter(URL, TEXT("webuiId"), WebUIId);
	}

	AppendRawQueryString(URL, AdditionalQueryString);
	return URL;
}

FString UWebUIRuntimeBrowserWidget::BuildBaseWebUIURL() const
{
	int32 Port = GetDefault<UWebUIRuntimeSettings>()->Port;
	if (const UWebUIRuntimeSubsystem* Runtime = GetRuntimeSubsystem())
	{
		if (Runtime->IsServerRunning() && Runtime->GetServerPort() > 0)
		{
			Port = Runtime->GetServerPort();
		}
	}

	const FString Host = bPreferLocalhost ? TEXT("127.0.0.1") : TEXT("localhost");
	return FString::Printf(TEXT("http://%s:%d/webui/"), *Host, Port);
}

FString UWebUIRuntimeBrowserWidget::NormalizeQueryString(const FString& InQueryString)
{
	FString Query = InQueryString;
	Query.TrimStartAndEndInline();
	while (Query.StartsWith(TEXT("?")) || Query.StartsWith(TEXT("&")))
	{
		Query.RightChopInline(1);
		Query.TrimStartAndEndInline();
	}
	return Query;
}

void UWebUIRuntimeBrowserWidget::AppendQueryParameter(FString& InURL, const FString& Key, const FString& Value)
{
	if (Key.IsEmpty() || Value.IsEmpty())
	{
		return;
	}

	const FString EncodedValue = UrlEncodeQueryValue(Value);
	if (InURL.Contains(TEXT("?")))
	{
		if (!InURL.EndsWith(TEXT("?")) && !InURL.EndsWith(TEXT("&")))
		{
			InURL.AppendChar(TEXT('&'));
		}
	}
	else
	{
		InURL.AppendChar(TEXT('?'));
	}

	InURL += Key;
	InURL.AppendChar(TEXT('='));
	InURL += EncodedValue;
}

void UWebUIRuntimeBrowserWidget::AppendRawQueryString(FString& InURL, const FString& RawQueryString)
{
	const FString Query = NormalizeQueryString(RawQueryString);
	if (Query.IsEmpty())
	{
		return;
	}

	if (InURL.Contains(TEXT("?")))
	{
		if (!InURL.EndsWith(TEXT("?")) && !InURL.EndsWith(TEXT("&")))
		{
			InURL.AppendChar(TEXT('&'));
		}
	}
	else
	{
		InURL.AppendChar(TEXT('?'));
	}

	InURL += Query;
}

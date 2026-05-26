#include "WebUIHostActor.h"

#include "Components/SceneComponent.h"
#include "WebUIHostComponent.h"
#include "WebUIRuntimeSubsystem.h"
#include "WebUIRuntimeSettings.h"
#include "HAL/PlatformProcess.h"

AWebUIHostActor::AWebUIHostActor()
{
	SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRootComponent);

	WebUIHostComponent = CreateDefaultSubobject<UWebUIHostComponent>(TEXT("WebUIHostComponent"));
}

void AWebUIHostActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SyncHostComponent();
}

void AWebUIHostActor::BeginPlay()
{
	SyncHostComponent();
	Super::BeginPlay();
}

bool AWebUIHostActor::StartWebUIServer()
{
	if (WebUIHostComponent)
	{
		SyncHostComponent();
		return WebUIHostComponent->StartWebUIServer();
	}
	return false;
}

void AWebUIHostActor::StopWebUIServer()
{
	if (WebUIHostComponent)
	{
		WebUIHostComponent->StopWebUIServer();
	}
}

UWebUIHostComponent* AWebUIHostActor::GetWebUIHostComponent() const
{
	return WebUIHostComponent;
}

FString AWebUIHostActor::GetWebUIId() const
{
	return WebUIId.IsEmpty() ? GetName() : WebUIId;
}

FString AWebUIHostActor::GetDescription() const
{
	return Description.IsEmpty() ? GetName() : Description;
}

bool AWebUIHostActor::ShouldAutoStartServer() const
{
	return bAutoStartServer;
}

void AWebUIHostActor::RegisterWebUIButton(FName ButtonId)
{
	if (WebUIHostComponent && !ButtonId.IsNone())
	{
		WebUIHostComponent->RegisterWebUIButton(ButtonId);
	}
}

void AWebUIHostActor::UnregisterWebUIButton(FName ButtonId)
{
	if (WebUIHostComponent)
	{
		WebUIHostComponent->UnregisterWebUIButton(ButtonId);
	}
}

void AWebUIHostActor::ClearWebUIButtons()
{
	if (WebUIHostComponent)
	{
		WebUIHostComponent->ClearWebUIButtons();
	}
}

const TArray<FName>& AWebUIHostActor::GetWebUIButtons() const
{
	static const TArray<FName> EmptyButtons;
	return WebUIHostComponent ? WebUIHostComponent->GetWebUIButtons() : EmptyButtons;
}

void AWebUIHostActor::NotifyWebUIButtonClicked(FName ButtonId)
{
	OnWebUIButtonClicked.Broadcast(ButtonId);
	K2_OnWebUIButtonClicked(ButtonId);
}

void AWebUIHostActor::SyncHostComponent() const
{
	if (!WebUIHostComponent)
	{
		return;
	}

	WebUIHostComponent->bAutoStartServer = bAutoStartServer;
	WebUIHostComponent->WebUIId = WebUIId;
	WebUIHostComponent->Description = Description;
	WebUIHostComponent->bAutoSaveChangedValues = bAutoSaveChangedValues;
	WebUIHostComponent->bAutoLoadSavedValues = bAutoLoadSavedValues;
}

FString AWebUIHostActor::GetBrowserURL() const
{
	UWebUIHostComponent* Comp = GetWebUIHostComponent();
	if (!Comp)
	{
		return FString();
	}

	int32 Port = 0;
	if (const UWorld* World = Comp->GetWorld())
	{
		if (const UWebUIRuntimeSubsystem* Runtime = World->GetSubsystem<UWebUIRuntimeSubsystem>())
		{
			Port = Runtime->GetServerPort();
		}
	}

	if (Port <= 0)
	{
		Port = GetDefault<UWebUIRuntimeSettings>()->Port;
	}

	const bool bAllowRemote = GetDefault<UWebUIRuntimeSettings>()->bAllowRemoteAccess;
	const FString Host = bAllowRemote ? FString(FPlatformProcess::ComputerName()) : TEXT("localhost");

	const FString CurrentWebUIId = Comp->GetWebUIId();
	return FString::Printf(TEXT("http://%s:%d/webui?webuiId=%s"), *Host, Port, *CurrentWebUIId);
}

FString AWebUIHostActor::GetEmbeddedURL() const
{
	const FString BrowserURL = GetBrowserURL();
	if (BrowserURL.IsEmpty())
	{
		return BrowserURL;
	}

	return BrowserURL + TEXT("&embed=1");
}

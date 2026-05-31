#include "WebUIHostComponent.h"

#include "WebUIHostActor.h"
#include "WebUIRuntime.h"
#include "WebUIRuntimeSubsystem.h"
#include "WebUIRuntimeSettings.h"

namespace
{
	FString StripWebUIOrderPrefix(const FString& RawLabel)
	{
		int32 SeparatorIndex = INDEX_NONE;
		for (int32 Index = 0; Index < RawLabel.Len(); ++Index)
		{
			const TCHAR Char = RawLabel[Index];
			if (Char == TEXT('_') || Char == TEXT(' '))
			{
				SeparatorIndex = Index;
				break;
			}
			if (!FChar::IsDigit(Char))
			{
				return RawLabel;
			}
		}

		if (SeparatorIndex > 0 && SeparatorIndex < RawLabel.Len())
		{
			const FString Prefix = RawLabel.Left(SeparatorIndex);
			int64 ParsedOrder = 0;
			if (LexTryParseString(ParsedOrder, *Prefix) && ParsedOrder >= 0)
			{
				const FString DisplayLabel = RawLabel.Mid(SeparatorIndex + 1);
				if (!DisplayLabel.IsEmpty())
				{
					return DisplayLabel;
				}
			}
		}

		return RawLabel;
	}

	bool IsWebUIButtonFunction(const UFunction* Function)
	{
		if (!Function || Function->NumParms != 0 || !Function->HasMetaData(TEXT("Category")))
		{
			return false;
		}

		FString Normalized = Function->GetMetaData(TEXT("Category"));
		Normalized.ReplaceInline(TEXT(" "), TEXT(""));
		return Normalized.Equals(TEXT("WebUI"), ESearchCase::IgnoreCase);
	}

	FName ResolveWebUIButtonId(const TArray<FName>& Buttons, const FName RequestedButtonId)
	{
		if (RequestedButtonId.IsNone())
		{
			return NAME_None;
		}

		if (Buttons.Contains(RequestedButtonId))
		{
			return RequestedButtonId;
		}

		const FString RequestedLabel = StripWebUIOrderPrefix(RequestedButtonId.ToString());
		for (const FName& Button : Buttons)
		{
			if (StripWebUIOrderPrefix(Button.ToString()).Equals(RequestedLabel, ESearchCase::IgnoreCase))
			{
				return Button;
			}
		}

		return NAME_None;
	}

	FName ResolveWebUIButtonFunctionId(const UObject* Owner, const FName RequestedButtonId)
	{
		if (!IsValid(Owner) || RequestedButtonId.IsNone())
		{
			return NAME_None;
		}

		UFunction* Function = Owner->FindFunction(RequestedButtonId);
		if (IsWebUIButtonFunction(Function))
		{
			return Function->GetFName();
		}

		const FString RequestedLabel = StripWebUIOrderPrefix(RequestedButtonId.ToString());
		for (TFieldIterator<UFunction> It(Owner->GetClass()); It; ++It)
		{
			UFunction* Candidate = *It;
			if (!IsWebUIButtonFunction(Candidate))
			{
				continue;
			}

			if (StripWebUIOrderPrefix(Candidate->GetName()).Equals(RequestedLabel, ESearchCase::IgnoreCase))
			{
				return Candidate->GetFName();
			}
		}

		return NAME_None;
	}
}

UWebUIHostComponent::UWebUIHostComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWebUIHostComponent::PostInitProperties()
{
	Super::PostInitProperties();
}

void UWebUIHostComponent::PostLoad()
{
	Super::PostLoad();
}

void UWebUIHostComponent::OnRegister()
{
	if (AActor* Owner = GetOwner())
	{
		if (!Owner->IsTemplate())
		{
			TInlineComponentArray<UWebUIHostComponent*> HostComponents;
			Owner->GetComponents(HostComponents);
			if (HostComponents.Num() > 1 && HostComponents[0] != this)
			{
				UE_LOG(LogWebUIRuntime, Warning, TEXT("Duplicate WebUIHostComponent on '%s' was removed."), *Owner->GetName());
				DestroyComponent();
				return;
			}
		}
	}

	Super::OnRegister();
}

void UWebUIHostComponent::BeginPlay()
{
	Super::BeginPlay();

	if (WebUIId.IsEmpty() && GetOwner())
	{
		WebUIId = GetOwner()->GetName();
	}

	if (UWebUIRuntimeSubsystem* Runtime = GetRuntimeSubsystem())
	{
		Runtime->RegisterHost(this);
		if (IsAutoSaveChangedValuesEnabled() && ShouldAutoLoadSavedValues())
		{
			Runtime->LoadPersistedState(this);
		}
		if (bAutoStartServer)
		{
			Runtime->StartServerFromSettings();
		}
	}
}

void UWebUIHostComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWebUIRuntimeSubsystem* Runtime = GetRuntimeSubsystem())
	{
		if (IsAutoSaveChangedValuesEnabled())
		{
			Runtime->SavePersistedState(this);
		}
		Runtime->UnregisterHost(this);
	}

	Super::EndPlay(EndPlayReason);
}

bool UWebUIHostComponent::StartWebUIServer()
{
	if (UWebUIRuntimeSubsystem* Runtime = GetRuntimeSubsystem())
	{
		Runtime->RegisterHost(this);
		if (IsAutoSaveChangedValuesEnabled() && ShouldAutoLoadSavedValues())
		{
			Runtime->LoadPersistedState(this);
		}
		return Runtime->StartServerFromSettings();
	}
	return false;
}

void UWebUIHostComponent::StopWebUIServer()
{
	if (UWebUIRuntimeSubsystem* Runtime = GetRuntimeSubsystem())
	{
		Runtime->StopServer();
	}
}

FString UWebUIHostComponent::GetWebUIId() const
{
	if (!WebUIId.IsEmpty())
	{
		return WebUIId;
	}
	return GetOwner() ? GetOwner()->GetName() : GetName();
}

int32 UWebUIHostComponent::GetWebUIPort() const
{
	return GetDefault<UWebUIRuntimeSettings>()->Port;
}

FString UWebUIHostComponent::GetDescription() const
{
	if (!Description.IsEmpty())
	{
		return Description;
	}
	return GetOwner() ? GetOwner()->GetName() : GetName();
}

void UWebUIHostComponent::RegisterWebUIButton(FName ButtonId)
{
	if (!ButtonId.IsNone())
	{
		WebUIButtons.AddUnique(ButtonId);
	}
}

void UWebUIHostComponent::UnregisterWebUIButton(FName ButtonId)
{
	ButtonId = ResolveWebUIButtonId(WebUIButtons, ButtonId);
	if (ButtonId.IsNone())
	{
		return;
	}

	WebUIButtons.Remove(ButtonId);
	WebUIButtonEnabledStates.Remove(ButtonId);
}

void UWebUIHostComponent::ClearWebUIButtons()
{
	WebUIButtons.Reset();
	WebUIButtonEnabledStates.Reset();
}

void UWebUIHostComponent::SetWebUIButtonEnabled(FName ButtonId, bool bEnabled)
{
	const FName RequestedButtonId = ButtonId;
	ButtonId = ResolveWebUIButtonId(WebUIButtons, RequestedButtonId);
	if (ButtonId.IsNone())
	{
		ButtonId = ResolveWebUIButtonFunctionId(GetOwner(), RequestedButtonId);
	}

	if (!ButtonId.IsNone())
	{
		const bool* CurrentEnabled = WebUIButtonEnabledStates.Find(ButtonId);
		if (CurrentEnabled && *CurrentEnabled == bEnabled)
		{
			return;
		}
		WebUIButtonEnabledStates.Add(ButtonId, bEnabled);

		if (UWorld* World = GetWorld())
		{
			if (UWebUIRuntimeSubsystem* Runtime = World->GetSubsystem<UWebUIRuntimeSubsystem>())
			{
				Runtime->NotifyWebUIComponentStateChanged(this);
			}
		}
	}
}

bool UWebUIHostComponent::IsWebUIButtonEnabled(FName ButtonId) const
{
	const FName RequestedButtonId = ButtonId;
	ButtonId = ResolveWebUIButtonId(WebUIButtons, RequestedButtonId);
	if (ButtonId.IsNone())
	{
		ButtonId = ResolveWebUIButtonFunctionId(GetOwner(), RequestedButtonId);
	}
	if (ButtonId.IsNone())
	{
		return false;
	}

	if (const bool* bEnabled = WebUIButtonEnabledStates.Find(ButtonId))
	{
		return *bEnabled;
	}

	return true;
}

const TArray<FName>& UWebUIHostComponent::GetWebUIButtons() const
{
	return WebUIButtons;
}

void UWebUIHostComponent::NotifyWebUIPropertyChanged(FName PropertyName)
{
	OnWebUIPropertyChanged.Broadcast(FName(*StripWebUIOrderPrefix(PropertyName.ToString())));
}

void UWebUIHostComponent::NotifyWebUIBoolChanged(FName PropertyName, bool Value)
{
	OnWebUIBoolChanged.Broadcast(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIHostComponent::NotifyWebUIFloatChanged(FName PropertyName, double Value)
{
	OnWebUIFloatChanged.Broadcast(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIHostComponent::NotifyWebUIStringChanged(FName PropertyName, const FString& Value)
{
	OnWebUIStringChanged.Broadcast(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIHostComponent::NotifyWebUIVectorChanged(FName PropertyName, FVector Value)
{
	OnWebUIVectorChanged.Broadcast(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIHostComponent::NotifyWebUIRotatorChanged(FName PropertyName, FRotator Value)
{
	OnWebUIRotatorChanged.Broadcast(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIHostComponent::NotifyWebUIColorChanged(FName PropertyName, FLinearColor Value)
{
	OnWebUIColorChanged.Broadcast(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIHostComponent::NotifyWebUIButtonClicked(FName ButtonId)
{
	OnWebUIButtonClicked.Broadcast(ButtonId);
	K2_OnWebUIButtonClicked(ButtonId);
	if (AWebUIHostActor* HostActor = Cast<AWebUIHostActor>(GetOwner()))
	{
		HostActor->NotifyWebUIButtonClicked(ButtonId);
	}
}

bool UWebUIHostComponent::IsAutoSaveChangedValuesEnabled() const
{
	return bAutoSaveChangedValues;
}

bool UWebUIHostComponent::ShouldAutoLoadSavedValues() const
{
	return bAutoLoadSavedValues;
}

UWebUIRuntimeSubsystem* UWebUIHostComponent::GetRuntimeSubsystem() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UWebUIRuntimeSubsystem>() : nullptr;
}

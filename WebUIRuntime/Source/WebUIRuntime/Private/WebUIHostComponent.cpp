#include "WebUIHostComponent.h"

#include "WebUIHostActor.h"
#include "WebUIRuntime.h"
#include "WebUIRuntimeSubsystem.h"
#include "WebUIRuntimeSettings.h"

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
	WebUIButtons.Remove(ButtonId);
}

void UWebUIHostComponent::ClearWebUIButtons()
{
	WebUIButtons.Reset();
}

const TArray<FName>& UWebUIHostComponent::GetWebUIButtons() const
{
	return WebUIButtons;
}

void UWebUIHostComponent::NotifyWebUIPropertyChanged(FName PropertyName)
{
	OnWebUIPropertyChanged.Broadcast(PropertyName);
}

void UWebUIHostComponent::NotifyWebUIBoolChanged(FName PropertyName, bool Value)
{
	OnWebUIBoolChanged.Broadcast(PropertyName, Value);
}

void UWebUIHostComponent::NotifyWebUIFloatChanged(FName PropertyName, double Value)
{
	OnWebUIFloatChanged.Broadcast(PropertyName, Value);
}

void UWebUIHostComponent::NotifyWebUIStringChanged(FName PropertyName, const FString& Value)
{
	OnWebUIStringChanged.Broadcast(PropertyName, Value);
}

void UWebUIHostComponent::NotifyWebUIVectorChanged(FName PropertyName, FVector Value)
{
	OnWebUIVectorChanged.Broadcast(PropertyName, Value);
}

void UWebUIHostComponent::NotifyWebUIRotatorChanged(FName PropertyName, FRotator Value)
{
	OnWebUIRotatorChanged.Broadcast(PropertyName, Value);
}

void UWebUIHostComponent::NotifyWebUIColorChanged(FName PropertyName, FLinearColor Value)
{
	OnWebUIColorChanged.Broadcast(PropertyName, Value);
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

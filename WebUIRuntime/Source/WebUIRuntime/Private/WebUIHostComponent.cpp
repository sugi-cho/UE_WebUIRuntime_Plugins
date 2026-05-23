#include "WebUIHostComponent.h"

#include "WebUIHostActor.h"
#include "WebUIRuntimeSubsystem.h"
#include "WebUIRuntimeSettings.h"

UWebUIHostComponent::UWebUIHostComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWebUIHostComponent::PostInitProperties()
{
	Super::PostInitProperties();
	RefreshOwnerSettingsMode();
}

void UWebUIHostComponent::PostLoad()
{
	Super::PostLoad();
	RefreshOwnerSettingsMode();
}

void UWebUIHostComponent::OnRegister()
{
	RefreshOwnerSettingsMode();
	Super::OnRegister();
}

void UWebUIHostComponent::BeginPlay()
{
	Super::BeginPlay();

	AWebUIHostActor* HostActor = Cast<AWebUIHostActor>(GetOwner());

	if (WebUIId.IsEmpty() && GetOwner() && !HostActor)
	{
		WebUIId = GetOwner()->GetName();
	}

	if (UWebUIRuntimeSubsystem* Runtime = GetRuntimeSubsystem())
	{
		Runtime->RegisterHost(this);
		const bool bShouldAutoStart = HostActor ? HostActor->ShouldAutoStartServer() : bAutoStartServer;
		if (bShouldAutoStart)
		{
			Runtime->StartServerFromSettings();
		}
	}
}

void UWebUIHostComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWebUIRuntimeSubsystem* Runtime = GetRuntimeSubsystem())
	{
		Runtime->UnregisterHost(this);
	}

	Super::EndPlay(EndPlayReason);
}

bool UWebUIHostComponent::StartWebUIServer()
{
	if (UWebUIRuntimeSubsystem* Runtime = GetRuntimeSubsystem())
	{
		Runtime->RegisterHost(this);
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
	if (const AWebUIHostActor* HostActor = Cast<AWebUIHostActor>(GetOwner()))
	{
		return HostActor->GetWebUIId();
	}
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
	if (const AWebUIHostActor* HostActor = Cast<AWebUIHostActor>(GetOwner()))
	{
		return HostActor->GetDescription();
	}
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

UWebUIRuntimeSubsystem* UWebUIHostComponent::GetRuntimeSubsystem() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UWebUIRuntimeSubsystem>() : nullptr;
}

void UWebUIHostComponent::RefreshOwnerSettingsMode()
{
	const AActor* OwnerActor = GetOwner() ? GetOwner() : GetTypedOuter<AActor>();
	bUseActorSettings = Cast<AWebUIHostActor>(OwnerActor) != nullptr;
}

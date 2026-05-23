#include "WebUIHostComponent.h"

#include "WebUIRuntimeSubsystem.h"
#include "WebUIRuntimeSettings.h"

UWebUIHostComponent::UWebUIHostComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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

UWebUIRuntimeSubsystem* UWebUIHostComponent::GetRuntimeSubsystem() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UWebUIRuntimeSubsystem>() : nullptr;
}

#include "WebUIImageComponent.h"

#include "WebUIRuntimeSubsystem.h"

UWebUIImageComponent::UWebUIImageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWebUIImageComponent::SetWebUIImageEnabled(bool bEnabled)
{
	if (bWebUIImageEnabled == bEnabled)
	{
		return;
	}

	bWebUIImageEnabled = bEnabled;
	NotifyWebUIPropertyChanged(TEXT("bWebUIImageEnabled"));

	if (UWorld* World = GetWorld())
	{
		if (UWebUIRuntimeSubsystem* Runtime = World->GetSubsystem<UWebUIRuntimeSubsystem>())
		{
			Runtime->NotifyWebUIComponentStateChanged(this);
		}
	}
}

bool UWebUIImageComponent::IsWebUIImageEnabled() const
{
	return bWebUIImageEnabled;
}

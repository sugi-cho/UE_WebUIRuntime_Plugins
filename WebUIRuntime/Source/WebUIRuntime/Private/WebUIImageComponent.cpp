#include "WebUIImageComponent.h"

UWebUIImageComponent::UWebUIImageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWebUIImageComponent::SetWebUIImageEnabled(bool bEnabled)
{
	bWebUIImageEnabled = bEnabled;
}

bool UWebUIImageComponent::IsWebUIImageEnabled() const
{
	return bWebUIImageEnabled;
}

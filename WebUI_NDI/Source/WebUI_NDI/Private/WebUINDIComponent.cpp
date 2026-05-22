#include "WebUINDIComponent.h"

UWebUINDIComponent::UWebUINDIComponent()
{
	RegisterWebUIButton(TEXT("RefreshNDISources"));
}

void UWebUINDIComponent::SetAvailableNDISources(const TArray<FString>& Sources)
{
	AvailableNDISources = Sources;
}

const TArray<FString>& UWebUINDIComponent::GetAvailableNDISources() const
{
	return AvailableNDISources;
}

void UWebUINDIComponent::SelectNDISource(const FString& SourceName)
{
	SelectedNDISource = SourceName;
	K2_OnWebUINDISourceSelected(SourceName);
}


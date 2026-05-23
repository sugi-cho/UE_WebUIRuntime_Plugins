#include "WebUINDIComponent.h"

#include "Objects/Libraries/NDIIOLibrary.h"

UWebUINDIComponent::UWebUINDIComponent()
{
	SyncWebUIButtonList();
}

void UWebUINDIComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshNDISources();
}

void UWebUINDIComponent::RefreshNDISources()
{
	AvailableNDISources.Reset();

	const TArray<FNDIConnectionInformation> Sources = UNDIIOLibrary::K2_GetNDISourceCollection();
	AvailableNDISources.Reserve(Sources.Num());
	for (const FNDIConnectionInformation& Source : Sources)
	{
		if (!Source.SourceName.IsEmpty())
		{
			AvailableNDISources.Add(Source.SourceName);
		}
	}

	SyncWebUIButtonList();
}

void UWebUINDIComponent::SetAvailableNDISources(const TArray<FString>& Sources)
{
	AvailableNDISources = Sources;
	SyncWebUIButtonList();
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

void UWebUINDIComponent::HandleWebUIButtonClicked(FName ButtonId)
{
	const FString ButtonName = ButtonId.ToString();
	if (ButtonName == TEXT("RefreshNDISources"))
	{
		RefreshNDISources();
		return;
	}

	if (AvailableNDISources.Contains(ButtonName))
	{
		SelectNDISource(ButtonName);
	}
}

void UWebUINDIComponent::SyncWebUIButtonList()
{
	ClearWebUIButtons();
	RegisterWebUIButton(TEXT("RefreshNDISources"));
	for (const FString& SourceName : AvailableNDISources)
	{
		RegisterWebUIButton(FName(*SourceName));
	}
}

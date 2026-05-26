#include "WebUINDIComponent.h"

#include "Objects/Libraries/NDIIOLibrary.h"
#include "Components/NDIReceiverComponent.h"

UWebUINDIComponent::UWebUINDIComponent()
{
	SyncWebUIButtonList();
}

void UWebUINDIComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshNDISources();
	ApplySelectedNDISource();
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

void UWebUINDIComponent::GetAvailableNDISourceOptions(TArray<FString>& OutSources) const
{
	OutSources = AvailableNDISources;
}

void UWebUINDIComponent::SelectNDISource(const FString& SourceName)
{
	SelectedNDISource = SourceName;
	ApplySelectedNDISource();
	K2_OnWebUINDISourceSelected(SourceName);
}

bool UWebUINDIComponent::ApplySelectedNDISource()
{
	if (SelectedNDISource.IsEmpty() || !IsValid(TargetNDIReceiverComponent))
	{
		return false;
	}

	FNDIConnectionInformation ConnectionInformation;
	bool bFoundSource = false;
	const TArray<FNDIConnectionInformation> Sources = UNDIIOLibrary::K2_GetNDISourceCollection();
	for (const FNDIConnectionInformation& Source : Sources)
	{
		if (Source.SourceName.Equals(SelectedNDISource, ESearchCase::IgnoreCase) ||
			Source.GetNDIName().Equals(SelectedNDISource, ESearchCase::IgnoreCase))
		{
			ConnectionInformation = Source;
			bFoundSource = true;
			break;
		}
	}

	if (!bFoundSource)
	{
		return false;
	}

	const FNDIConnectionInformation CurrentConnection = TargetNDIReceiverComponent->GetCurrentConnectionInformation();
	if (CurrentConnection.IsValid())
	{
		TargetNDIReceiverComponent->ChangeConnection(ConnectionInformation);
	}
	else
	{
		TargetNDIReceiverComponent->StartReceiver(ConnectionInformation);
	}

	return true;
}

void UWebUINDIComponent::SetTargetNDIReceiverComponent(UNDIReceiverComponent* InTargetNDIReceiverComponent)
{
	TargetNDIReceiverComponent = InTargetNDIReceiverComponent;
	ApplySelectedNDISource();
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
}

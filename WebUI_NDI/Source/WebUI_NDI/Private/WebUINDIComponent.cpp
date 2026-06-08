#include "WebUINDIComponent.h"

#include "Services/NDIFinderService.h"
#include "Objects/Media/NDIMediaReceiver.h"
#include "Objects/Media/NDIMediaTexture2D.h"

UWebUINDIComponent::UWebUINDIComponent()
{
	SyncWebUIButtonList();
}

void UWebUINDIComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureTargetNDIResources();
	RefreshNDISources();
	ApplySelectedNDISource();
}

void UWebUINDIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseTargetNDIResources();
	Super::EndPlay(EndPlayReason);
}

void UWebUINDIComponent::RefreshNDISources()
{
	AvailableNDISources.Reset();

	const TArray<FNDIConnectionInformation> Sources = FNDIFinderService::GetNetworkSourceCollection();
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
	if (ApplySelectedNDISource())
	{
		OnWebUINDISourceSelected.Broadcast(SourceName);
		K2_OnWebUINDISourceSelected(SourceName);
	}
}

bool UWebUINDIComponent::ApplySelectedNDISource()
{
	if (!EnsureTargetNDIResources() || SelectedNDISource.IsEmpty())
	{
		return false;
	}

	FNDIConnectionInformation ConnectionInformation;
	bool bFoundSource = false;
	const TArray<FNDIConnectionInformation> Sources = FNDIFinderService::GetNetworkSourceCollection();
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

	TargetNDIMediaReceiver->ConnectionSetting = ConnectionInformation;

	const FNDIConnectionInformation CurrentConnection = TargetNDIMediaReceiver->GetCurrentConnectionInformation();
	if (CurrentConnection.IsValid())
	{
		TargetNDIMediaReceiver->ChangeConnection(ConnectionInformation);
	}
	else
	{
		TargetNDIMediaReceiver->Initialize(ConnectionInformation, UNDIMediaReceiver::EUsage::Standalone);
	}

	return true;
}

void UWebUINDIComponent::SetTargetNDIMediaReceiver(UNDIMediaReceiver* InTargetNDIMediaReceiver)
{
	bOwnsTargetNDIResources = false;
	TargetNDIMediaReceiver = InTargetNDIMediaReceiver;
	ApplySelectedNDISource();
}

bool UWebUINDIComponent::EnsureTargetNDIResources()
{
	if (IsValid(TargetNDIMediaReceiver))
	{
		return true;
	}

	TargetNDIMediaReceiver = NewObject<UNDIMediaReceiver>(this, UNDIMediaReceiver::StaticClass(), NAME_None, RF_Transient);
	if (!IsValid(TargetNDIMediaReceiver))
	{
		return false;
	}

	UNDIMediaTexture2D* TargetNDIMediaTexture = NewObject<UNDIMediaTexture2D>(
		TargetNDIMediaReceiver, UNDIMediaTexture2D::StaticClass(), NAME_None, RF_Transient);
	if (!IsValid(TargetNDIMediaTexture))
	{
		return false;
	}

	TargetNDIMediaTexture->UpdateResource();
	TargetNDIMediaReceiver->ChangeVideoTexture(TargetNDIMediaTexture);
	bOwnsTargetNDIResources = true;
	return true;
}

void UWebUINDIComponent::ReleaseTargetNDIResources()
{
	if (IsValid(TargetNDIMediaReceiver) && bOwnsTargetNDIResources)
	{
		TargetNDIMediaReceiver->ChangeVideoTexture(nullptr);
		TargetNDIMediaReceiver->Shutdown();
	}

	TargetNDIMediaReceiver = nullptr;
	bOwnsTargetNDIResources = false;
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

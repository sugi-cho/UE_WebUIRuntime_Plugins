#include "WebUINDIComponent.h"

#include "Async/Async.h"
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
	BindTargetNDIEvents();
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
	NotifyWebUIStateChanged();
}

void UWebUINDIComponent::SetAvailableNDISources(const TArray<FString>& Sources)
{
	AvailableNDISources = Sources;
	SyncWebUIButtonList();
	NotifyWebUIStateChanged();
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
	const bool bApplied = ApplySelectedNDISource();
	OnWebUINDISourceSelected.Broadcast(SourceName);
	K2_OnWebUINDISourceSelected(SourceName);
	if (!bApplied)
	{
		UE_LOG(LogTemp, Warning, TEXT("WebUINDI: selection broadcasted before NDI connection was fully applied: %s"), *SourceName);
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
	TargetNDIMediaReceiver->Shutdown();
	EnsureTargetNDIVideoTexture();
	BindTargetNDIEvents();

	if (!TargetNDIMediaReceiver->Initialize(ConnectionInformation, UNDIMediaReceiver::EUsage::Standalone))
	{
		TargetNDIMediaReceiver->ChangeConnection(ConnectionInformation);
		TargetNDIMediaReceiver->StartConnection();
	}

	TargetNDIMediaReceiver->CaptureConnectedVideo();

	return true;
}

void UWebUINDIComponent::SetTargetNDIMediaReceiver(UNDIMediaReceiver* InTargetNDIMediaReceiver)
{
	UnbindTargetNDIEvents();
	bOwnsTargetNDIResources = false;
	TargetNDIMediaReceiver = InTargetNDIMediaReceiver;
	EnsureTargetNDIVideoTexture();
	BindTargetNDIEvents();
	ApplySelectedNDISource();
}

bool UWebUINDIComponent::EnsureTargetNDIResources()
{
	if (IsValid(TargetNDIMediaReceiver))
	{
		return EnsureTargetNDIVideoTexture();
	}

	TargetNDIMediaReceiver = NewObject<UNDIMediaReceiver>(this, UNDIMediaReceiver::StaticClass(), NAME_None, RF_Transient);
	if (!IsValid(TargetNDIMediaReceiver))
	{
		return false;
	}

	UNDIMediaTexture2D* NewTargetNDIMediaTexture = NewObject<UNDIMediaTexture2D>(
		TargetNDIMediaReceiver, UNDIMediaTexture2D::StaticClass(), NAME_None, RF_Transient);
	if (!IsValid(NewTargetNDIMediaTexture))
	{
		return false;
	}

	NewTargetNDIMediaTexture->UpdateResource();
	TargetNDIMediaReceiver->ChangeVideoTexture(NewTargetNDIMediaTexture);
	this->TargetNDIMediaTexture = NewTargetNDIMediaTexture;
	bOwnsTargetNDIResources = true;
	BindTargetNDIEvents();
	return true;
}

bool UWebUINDIComponent::EnsureTargetNDIVideoTexture()
{
	if (!IsValid(TargetNDIMediaReceiver))
	{
		return false;
	}

	if (IsValid(TargetNDIMediaTexture))
	{
		return true;
	}

	TargetNDIMediaTexture = NewObject<UNDIMediaTexture2D>(
		TargetNDIMediaReceiver, UNDIMediaTexture2D::StaticClass(), NAME_None, RF_Transient);
	if (!IsValid(TargetNDIMediaTexture))
	{
		return false;
	}

	TargetNDIMediaTexture->UpdateResource();
	TargetNDIMediaReceiver->ChangeVideoTexture(TargetNDIMediaTexture);
	return true;
}

void UWebUINDIComponent::BindTargetNDIEvents()
{
	if (IsValid(TargetNDIMediaReceiver))
	{
		if (TargetNDIVideoCaptureEventHandle.IsValid())
		{
			TargetNDIMediaReceiver->OnNDIReceiverVideoCaptureEvent.Remove(TargetNDIVideoCaptureEventHandle);
			TargetNDIVideoCaptureEventHandle.Reset();
		}

		TargetNDIVideoCaptureEventHandle = TargetNDIMediaReceiver->OnNDIReceiverVideoCaptureEvent.AddUObject(
			this, &UWebUINDIComponent::HandleTargetNDIVideoCaptured);
	}
}

void UWebUINDIComponent::UnbindTargetNDIEvents()
{
	if (IsValid(TargetNDIMediaReceiver) && TargetNDIVideoCaptureEventHandle.IsValid())
	{
		TargetNDIMediaReceiver->OnNDIReceiverVideoCaptureEvent.Remove(TargetNDIVideoCaptureEventHandle);
		TargetNDIVideoCaptureEventHandle.Reset();
	}
}

void UWebUINDIComponent::ReleaseTargetNDIResources()
{
	UnbindTargetNDIEvents();
	if (IsValid(TargetNDIMediaReceiver) && bOwnsTargetNDIResources)
	{
		TargetNDIMediaReceiver->ChangeVideoTexture(nullptr);
		TargetNDIMediaReceiver->Shutdown();
	}

	TargetNDIMediaReceiver = nullptr;
	TargetNDIMediaTexture = nullptr;
	bOwnsTargetNDIResources = false;
}

void UWebUINDIComponent::HandleWebUIButtonClicked(FName ButtonId)
{
	const FString ButtonName = ButtonId.ToString();
	if (ButtonName == TEXT("00_Refresh") || ButtonName == TEXT("Refresh") || ButtonName == TEXT("RefreshNDISources"))
	{
		RefreshNDISources();
		return;
	}

	if (AvailableNDISources.Contains(ButtonName))
	{
		SelectNDISource(ButtonName);
	}
}

void UWebUINDIComponent::HandleTargetNDIVideoCaptured(UNDIMediaReceiver* Receiver, const NDIlib_video_frame_v2_t& VideoFrame)
{
	if (!VideoFrame.p_data)
	{
		return;
	}

	if (!IsValid(Receiver) || Receiver != TargetNDIMediaReceiver)
	{
		return;
	}

	if (!IsValid(TargetNDIMediaTexture))
	{
		return;
	}

	const FNDIConnectionInformation& ConnectionInfo = Receiver->GetCurrentConnectionInformation();
	const FString SourceName = !ConnectionInfo.GetNDIName().IsEmpty() ? ConnectionInfo.GetNDIName() : SelectedNDISource;
	TWeakObjectPtr<UWebUINDIComponent> WeakThis(this);
	TWeakObjectPtr<UNDIMediaTexture2D> WeakTexture(TargetNDIMediaTexture);
	AsyncTask(ENamedThreads::GameThread, [WeakThis, SourceName, WeakTexture]()
	{
		if (!WeakThis.IsValid() || !WeakTexture.IsValid())
		{
			return;
		}

		WeakThis->OnWebUINDIVideoFrameReceived.Broadcast(SourceName, WeakTexture.Get());
		WeakThis->K2_OnWebUINDIVideoFrameReceived(SourceName, WeakTexture.Get());
	});
}

void UWebUINDIComponent::SyncWebUIButtonList()
{
	ClearWebUIButtons();
	RegisterWebUIButton(TEXT("00_Refresh"));
}

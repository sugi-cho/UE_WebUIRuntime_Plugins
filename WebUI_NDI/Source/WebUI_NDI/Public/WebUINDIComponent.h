#pragma once

#include "CoreMinimal.h"
#include "WebUIComponentBase.h"
#include "WebUINDIComponent.generated.h"

class UNDIMediaReceiver;
class UNDIMediaTexture2D;
struct FNDIConnectionInformation;
struct NDIlib_video_frame_v2_t;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWebUINDISourceSelected, const FString&, SourceName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWebUINDIVideoFrameReceived, const FString&, SourceName, UNDIMediaTexture2D*, VideoTexture);

UCLASS(BlueprintType, Blueprintable, ClassGroup=(WebUI), meta=(BlueprintSpawnableComponent))
class WEBUI_NDI_API UWebUINDIComponent : public UWebUIComponentBase
{
	GENERATED_BODY()

public:
	UWebUINDIComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="WebUI|NDI", DisplayName="Refresh")
	void RefreshNDISources();

	UFUNCTION(BlueprintCallable, Category="WebUI|NDI")
	void SetAvailableNDISources(const TArray<FString>& Sources);

	UFUNCTION(BlueprintPure, Category="WebUI|NDI")
	const TArray<FString>& GetAvailableNDISources() const;

	UFUNCTION(BlueprintPure, Category="WebUI|NDI")
	void GetAvailableNDISourceOptions(TArray<FString>& OutSources) const;

	UFUNCTION(BlueprintCallable, Category="WebUI|NDI")
	void SelectNDISource(const FString& SourceName);

	UFUNCTION(BlueprintCallable, Category="WebUI|NDI")
	bool ApplySelectedNDISource();

	UFUNCTION(BlueprintCallable, Category="WebUI|NDI")
	void SetTargetNDIMediaReceiver(UNDIMediaReceiver* InTargetNDIMediaReceiver);

	UFUNCTION(BlueprintImplementableEvent, Category="WebUI|NDI", DisplayName="OnWebUINDISourceSelected")
	void K2_OnWebUINDISourceSelected(const FString& SourceName);

	UFUNCTION(BlueprintImplementableEvent, Category="WebUI|NDI", DisplayName="OnWebUINDIVideoFrameReceived")
	void K2_OnWebUINDIVideoFrameReceived(const FString& SourceName, UNDIMediaTexture2D* VideoTexture);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI", meta=(WebUI, WebUIOptions="GetAvailableNDISourceOptions", WebUIOnChanged="SelectNDISource"))
	FString SelectedNDISource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NDI")
	TObjectPtr<UNDIMediaReceiver> TargetNDIMediaReceiver = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NDI")
	bool bAutoRefreshSources = false;

	UPROPERTY(BlueprintAssignable, Category="WebUI|NDI", meta=(DisplayName="On WebUI NDI Source Selected"))
	FOnWebUINDISourceSelected OnWebUINDISourceSelected;

	UPROPERTY(BlueprintAssignable, Category="WebUI|NDI", meta=(DisplayName="On WebUI NDI Video Frame Received"))
	FOnWebUINDIVideoFrameReceived OnWebUINDIVideoFrameReceived;

protected:
	virtual void HandleWebUIButtonClicked(FName ButtonId) override;

private:
	UPROPERTY(VisibleAnywhere, Category="WebUI|NDI")
	TArray<FString> AvailableNDISources;

	bool EnsureTargetNDIResources();
	bool EnsureTargetNDIVideoTexture();
	void BindTargetNDIEvents();
	void UnbindTargetNDIEvents();
	void ReleaseTargetNDIResources();
	void SyncWebUIButtonList();

	void HandleTargetNDIVideoCaptured(UNDIMediaReceiver* Receiver, const NDIlib_video_frame_v2_t& VideoFrame);

	FDelegateHandle TargetNDIVideoCaptureEventHandle;
	UPROPERTY(Transient)
	TObjectPtr<UNDIMediaTexture2D> TargetNDIMediaTexture = nullptr;

	bool bOwnsTargetNDIResources = false;
};

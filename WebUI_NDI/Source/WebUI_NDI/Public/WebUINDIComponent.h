#pragma once

#include "CoreMinimal.h"
#include "WebUIComponentBase.h"
#include "WebUINDIComponent.generated.h"

class UNDIReceiverComponent;
struct FNDIConnectionInformation;

UCLASS(BlueprintType, Blueprintable, ClassGroup=(WebUI), meta=(BlueprintSpawnableComponent))
class WEBUI_NDI_API UWebUINDIComponent : public UWebUIComponentBase
{
	GENERATED_BODY()

public:
	UWebUINDIComponent();
	virtual void BeginPlay() override;

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
	void SetTargetNDIReceiverComponent(UNDIReceiverComponent* InTargetNDIReceiverComponent);

	UFUNCTION(BlueprintImplementableEvent, Category="WebUI|NDI", DisplayName="OnWebUINDISourceSelected")
	void K2_OnWebUINDISourceSelected(const FString& SourceName);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI", meta=(WebUI, WebUIOptions="GetAvailableNDISourceOptions", WebUIOnChanged="SelectNDISource", WebUIActions="RefreshNDISources"))
	FString SelectedNDISource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NDI")
	TObjectPtr<UNDIReceiverComponent> TargetNDIReceiverComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NDI")
	bool bAutoRefreshSources = false;

protected:
	virtual void HandleWebUIButtonClicked(FName ButtonId) override;

private:
	UPROPERTY(VisibleAnywhere, Category="WebUI|NDI")
	TArray<FString> AvailableNDISources;

	void SyncWebUIButtonList();
};

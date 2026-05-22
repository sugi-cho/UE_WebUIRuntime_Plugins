#pragma once

#include "CoreMinimal.h"
#include "WebUIComponentBase.h"
#include "WebUINDIComponent.generated.h"

UCLASS(BlueprintType, Blueprintable, ClassGroup=(WebUI), meta=(BlueprintSpawnableComponent))
class WEBUI_NDI_API UWebUINDIComponent : public UWebUIComponentBase
{
	GENERATED_BODY()

public:
	UWebUINDIComponent();

	UFUNCTION(BlueprintCallable, Category="WebUI|NDI")
	void SetAvailableNDISources(const TArray<FString>& Sources);

	UFUNCTION(BlueprintPure, Category="WebUI|NDI")
	const TArray<FString>& GetAvailableNDISources() const;

	UFUNCTION(BlueprintCallable, Category="WebUI|NDI")
	void SelectNDISource(const FString& SourceName);

	UFUNCTION(BlueprintImplementableEvent, Category="WebUI|NDI", DisplayName="OnWebUINDISourceSelected")
	void K2_OnWebUINDISourceSelected(const FString& SourceName);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI")
	FString SelectedNDISource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI")
	bool bAutoRefreshSources = false;

private:
	UPROPERTY(VisibleAnywhere, Category="WebUI|NDI")
	TArray<FString> AvailableNDISources;
};


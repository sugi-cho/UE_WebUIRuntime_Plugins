#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "WebUIPresentationTypes.h"
#include "WebUIComponentBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWebUIButtonClicked, FName, ButtonId);

UCLASS(BlueprintType, Blueprintable, ClassGroup=(WebUI), meta=(BlueprintSpawnableComponent))
class WEBUIRUNTIME_API UWebUIComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:
	UWebUIComponentBase();

	UFUNCTION(BlueprintPure, Category="WebUI")
	bool IsWebUIExpandedByDefault() const { return bWebUIExpandedByDefault; }

	UFUNCTION(BlueprintCallable, Category="WebUI")
	void RegisterWebUIButton(FName ButtonId);

	UFUNCTION(BlueprintCallable, Category="WebUI")
	void UnregisterWebUIButton(FName ButtonId);

	UFUNCTION(BlueprintCallable, Category="WebUI")
	void ClearWebUIButtons();

	UFUNCTION(BlueprintCallable, Category="WebUI")
	void SetWebUIButtonEnabled(FName ButtonId, bool bEnabled);

	UFUNCTION(BlueprintPure, Category="WebUI")
	bool IsWebUIButtonEnabled(FName ButtonId) const;

	UFUNCTION(BlueprintPure, Category="WebUI")
	FString GetWebUIDisplayName() const;

	UFUNCTION(BlueprintCallable, Category="WebUI")
	void SetWebUIDisplayName(const FString& InDisplayName);

	const TArray<FName>& GetWebUIButtons() const;

	UPROPERTY(EditAnywhere, Category="WebUI")
	TMap<FName, FWebUIPropertyPresentation> PropertyPresentations;

	UPROPERTY(EditAnywhere, Category="WebUI")
	TMap<FName, FWebUIButtonPresentation> ButtonPresentations;

	void NotifyWebUIPropertyChanged(FName PropertyName);
	void NotifyWebUIBoolChanged(FName PropertyName, bool Value);
	void NotifyWebUIFloatChanged(FName PropertyName, double Value);
	void NotifyWebUIStringChanged(FName PropertyName, const FString& Value);
	void NotifyWebUIVectorChanged(FName PropertyName, FVector Value);
	void NotifyWebUIRotatorChanged(FName PropertyName, FRotator Value);
	void NotifyWebUIColorChanged(FName PropertyName, FLinearColor Value);
	void NotifyWebUIButtonClicked(FName ButtonId);

protected:
	virtual void HandleWebUIButtonClicked(FName ButtonId);

	UFUNCTION(BlueprintImplementableEvent, Category="WebUI", DisplayName="OnWebUIPropertyChanged")
	void K2_OnWebUIPropertyChanged(FName PropertyName);

	UFUNCTION(BlueprintImplementableEvent, Category="WebUI", DisplayName="OnWebUIBoolChanged")
	void K2_OnWebUIBoolChanged(FName PropertyName, bool Value);

	UFUNCTION(BlueprintImplementableEvent, Category="WebUI", DisplayName="OnWebUIFloatChanged")
	void K2_OnWebUIFloatChanged(FName PropertyName, double Value);

	UFUNCTION(BlueprintImplementableEvent, Category="WebUI", DisplayName="OnWebUIStringChanged")
	void K2_OnWebUIStringChanged(FName PropertyName, const FString& Value);

	UFUNCTION(BlueprintImplementableEvent, Category="WebUI", DisplayName="OnWebUIVectorChanged")
	void K2_OnWebUIVectorChanged(FName PropertyName, FVector Value);

	UFUNCTION(BlueprintImplementableEvent, Category="WebUI", DisplayName="OnWebUIRotatorChanged")
	void K2_OnWebUIRotatorChanged(FName PropertyName, FRotator Value);

	UFUNCTION(BlueprintImplementableEvent, Category="WebUI", DisplayName="OnWebUIColorChanged")
	void K2_OnWebUIColorChanged(FName PropertyName, FLinearColor Value);

	UFUNCTION(BlueprintImplementableEvent, Category="WebUI", DisplayName="OnWebUIButtonClicked")
	void K2_OnWebUIButtonClicked(FName ButtonId);

	UPROPERTY(BlueprintAssignable, Category="WebUI")
	FOnWebUIButtonClicked OnWebUIButtonClicked;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Internal", meta=(AllowPrivateAccess="true"))
	FString WebUIDisplayName;

	UPROPERTY(EditAnywhere, Category="WebUI Internal", meta=(DisplayName="Expanded By Default"))
	bool bWebUIExpandedByDefault = true;

	UPROPERTY(EditAnywhere, Category="WebUI")
	TArray<FName> WebUIButtons;

	UPROPERTY(EditAnywhere, Category="WebUI")
	TMap<FName, bool> WebUIButtonEnabledStates;
};

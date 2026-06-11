#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "WebUIPresentationTypes.h"
#include "WebUIHostComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWebUIHostPropertyChanged, FName, PropertyName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWebUIHostBoolChanged, FName, PropertyName, bool, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWebUIHostFloatChanged, FName, PropertyName, double, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWebUIHostStringChanged, FName, PropertyName, const FString&, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWebUIHostVectorChanged, FName, PropertyName, FVector, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWebUIHostRotatorChanged, FName, PropertyName, FRotator, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWebUIHostColorChanged, FName, PropertyName, FLinearColor, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWebUIHostButtonClicked, FName, ButtonId);

class UWebUIRuntimeSubsystem;
class AWebUIHostActor;
class UTexture2D;

UENUM(BlueprintType)
enum class EWebUIHostVisibility : uint8
{
	BrowserOnly UMETA(DisplayName="Browser Only"),
	WidgetOnly UMETA(DisplayName="Widget Only"),
	Both UMETA(DisplayName="Both")
};

UCLASS(BlueprintType, Blueprintable, ClassGroup=(WebUI), meta=(BlueprintSpawnableComponent))
class WEBUIRUNTIME_API UWebUIHostComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWebUIHostComponent();

	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	bool StartWebUIServer();

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	void StopWebUIServer();

	UFUNCTION(BlueprintPure, Category="WebUI Host")
	FString GetWebUIId() const;

	UFUNCTION(BlueprintPure, Category="WebUI Host")
	int32 GetWebUIPort() const;

	UFUNCTION(BlueprintPure, Category="WebUI Host")
	FString GetDescription() const;

	UFUNCTION(BlueprintPure, Category="WebUI Host")
	FString GetBrowserURL() const;

	UFUNCTION(BlueprintPure, Category="WebUI Host")
	FString GetEmbeddedURL() const;

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	UTexture2D* CreateURLQRCodeTexture(const FString& URL, int32 PixelsPerModule = 8, int32 QuietZoneModules = 4) const;

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	UTexture2D* CreateBrowserURLQRCodeTexture(int32 PixelsPerModule = 8, int32 QuietZoneModules = 4) const;

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	UTexture2D* CreateEmbeddedURLQRCodeTexture(int32 PixelsPerModule = 8, int32 QuietZoneModules = 4) const;

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	UTexture2D* CreateBrowserURLQRCodeTextureWithControlToken(int32 PixelsPerModule = 8, int32 QuietZoneModules = 4) const;

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	UTexture2D* CreateEmbeddedURLQRCodeTextureWithControlToken(int32 PixelsPerModule = 8, int32 QuietZoneModules = 4) const;

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	void RegisterWebUIButton(FName ButtonId);

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	void UnregisterWebUIButton(FName ButtonId);

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	void ClearWebUIButtons();

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	void SetWebUIButtonEnabled(FName ButtonId, bool bEnabled);

	UFUNCTION(BlueprintPure, Category="WebUI Host")
	bool IsWebUIButtonEnabled(FName ButtonId) const;

	UFUNCTION(BlueprintPure, Category="WebUI Host")
	const TArray<FName>& GetWebUIButtons() const;

	UPROPERTY(EditAnywhere, Category="WebUI Host")
	TMap<FName, FWebUIPropertyPresentation> ActorPropertyPresentations;

	UPROPERTY(EditAnywhere, Category="WebUI Host")
	TMap<FName, FWebUIButtonPresentation> ButtonPresentations;

	void NotifyWebUIPropertyChanged(FName PropertyName);
	void NotifyWebUIBoolChanged(FName PropertyName, bool Value);
	void NotifyWebUIFloatChanged(FName PropertyName, double Value);
	void NotifyWebUIStringChanged(FName PropertyName, const FString& Value);
	void NotifyWebUIVectorChanged(FName PropertyName, FVector Value);
	void NotifyWebUIRotatorChanged(FName PropertyName, FRotator Value);
	void NotifyWebUIColorChanged(FName PropertyName, FLinearColor Value);
	void NotifyWebUIButtonClicked(FName ButtonId);

	bool IsAutoSaveChangedValuesEnabled() const;
	bool ShouldAutoLoadSavedValues() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Host")
	bool bAutoStartServer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Host")
	FString WebUIId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Host", meta=(MultiLine=true))
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Host")
	bool bAutoSaveChangedValues = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Host")
	bool bAutoLoadSavedValues = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Host")
	EWebUIHostVisibility DisplayTarget = EWebUIHostVisibility::Both;

	UPROPERTY(BlueprintAssignable, Category="WebUI")
	FOnWebUIHostPropertyChanged OnWebUIPropertyChanged;

	UPROPERTY(BlueprintAssignable, Category="WebUI")
	FOnWebUIHostBoolChanged OnWebUIBoolChanged;

	UPROPERTY(BlueprintAssignable, Category="WebUI")
	FOnWebUIHostFloatChanged OnWebUIFloatChanged;

	UPROPERTY(BlueprintAssignable, Category="WebUI")
	FOnWebUIHostStringChanged OnWebUIStringChanged;

	UPROPERTY(BlueprintAssignable, Category="WebUI")
	FOnWebUIHostVectorChanged OnWebUIVectorChanged;

	UPROPERTY(BlueprintAssignable, Category="WebUI")
	FOnWebUIHostRotatorChanged OnWebUIRotatorChanged;

	UPROPERTY(BlueprintAssignable, Category="WebUI")
	FOnWebUIHostColorChanged OnWebUIColorChanged;

	UPROPERTY(BlueprintAssignable, Category="WebUI")
	FOnWebUIHostButtonClicked OnWebUIButtonClicked;

	UFUNCTION(BlueprintImplementableEvent, Category="WebUI", DisplayName="OnWebUIButtonClicked")
	void K2_OnWebUIButtonClicked(FName ButtonId);

private:
	FString BuildControlTokenQRCodeURL(bool bEmbed) const;
	UWebUIRuntimeSubsystem* GetRuntimeSubsystem() const;

	UPROPERTY(EditAnywhere, Category="WebUI Host")
	TArray<FName> WebUIButtons;

	UPROPERTY(EditAnywhere, Category="WebUI Host")
	TMap<FName, bool> WebUIButtonEnabledStates;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WebUIHostActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWebUIHostActorButtonClicked, FName, ButtonId);

class UWebUIHostComponent;
class USceneComponent;
class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class WEBUIRUNTIME_API AWebUIHostActor : public AActor
{
	GENERATED_BODY()

public:
	AWebUIHostActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	bool StartWebUIServer();

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	void StopWebUIServer();

	UFUNCTION(BlueprintPure, Category="WebUI Host")
	UWebUIHostComponent* GetWebUIHostComponent() const;

	UFUNCTION(BlueprintPure, Category="WebUI Host")
	FString GetWebUIId() const;

	UFUNCTION(BlueprintPure, Category="WebUI Host")
	FString GetDescription() const;

	UFUNCTION(BlueprintPure, Category="WebUI Host")
	bool ShouldAutoStartServer() const;

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	void RegisterWebUIButton(FName ButtonId);

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	void UnregisterWebUIButton(FName ButtonId);

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	void ClearWebUIButtons();

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

	UFUNCTION(BlueprintPure, Category="WebUI Host")
	const TArray<FName>& GetWebUIButtons() const;

	void NotifyWebUIButtonClicked(FName ButtonId);

	UPROPERTY(BlueprintAssignable, Category="WebUI")
	FOnWebUIHostActorButtonClicked OnWebUIButtonClicked;

	UFUNCTION(BlueprintImplementableEvent, Category="WebUI", DisplayName="OnWebUIButtonClicked")
	void K2_OnWebUIButtonClicked(FName ButtonId);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="WebUI Host", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> SceneRootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="WebUI Host", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UWebUIHostComponent> WebUIHostComponent;
};

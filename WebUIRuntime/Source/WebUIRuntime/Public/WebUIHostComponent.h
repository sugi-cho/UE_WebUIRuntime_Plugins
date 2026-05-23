#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "WebUIHostComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWebUIHostPropertyChanged, FName, PropertyName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWebUIHostBoolChanged, FName, PropertyName, bool, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWebUIHostFloatChanged, FName, PropertyName, double, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWebUIHostStringChanged, FName, PropertyName, const FString&, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWebUIHostVectorChanged, FName, PropertyName, FVector, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWebUIHostRotatorChanged, FName, PropertyName, FRotator, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWebUIHostColorChanged, FName, PropertyName, FLinearColor, Value);

class UWebUIRuntimeSubsystem;

UCLASS(BlueprintType, Blueprintable, ClassGroup=(WebUI), meta=(BlueprintSpawnableComponent))
class WEBUIRUNTIME_API UWebUIHostComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWebUIHostComponent();

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

	void NotifyWebUIPropertyChanged(FName PropertyName);
	void NotifyWebUIBoolChanged(FName PropertyName, bool Value);
	void NotifyWebUIFloatChanged(FName PropertyName, double Value);
	void NotifyWebUIStringChanged(FName PropertyName, const FString& Value);
	void NotifyWebUIVectorChanged(FName PropertyName, FVector Value);
	void NotifyWebUIRotatorChanged(FName PropertyName, FRotator Value);
	void NotifyWebUIColorChanged(FName PropertyName, FLinearColor Value);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Host")
	bool bAutoStartServer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Host")
	FString WebUIId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Host", meta=(MultiLine=true))
	FString Description;

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

private:
	UWebUIRuntimeSubsystem* GetRuntimeSubsystem() const;
};

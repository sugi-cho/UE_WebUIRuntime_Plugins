#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "WebUIHostComponent.generated.h"

class UWebUIRuntimeSubsystem;

UCLASS(BlueprintType, Blueprintable, ClassGroup=(WebUI), meta=(BlueprintSpawnableComponent))
class WEBUIRUNTIME_API UWebUIHostComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWebUIHostComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="WebUI")
	bool StartWebUIServer();

	UFUNCTION(BlueprintCallable, Category="WebUI")
	void StopWebUIServer();

	UFUNCTION(BlueprintPure, Category="WebUI")
	FString GetWebUIId() const;

	UFUNCTION(BlueprintPure, Category="WebUI")
	int32 GetWebUIPort() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI")
	bool bAutoStartServer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI")
	FString WebUIId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI", meta=(ClampMin="1", ClampMax="65535"))
	int32 Port = 8765;

private:
	UWebUIRuntimeSubsystem* GetRuntimeSubsystem() const;
};


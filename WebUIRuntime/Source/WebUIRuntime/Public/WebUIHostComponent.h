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

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	bool StartWebUIServer();

	UFUNCTION(BlueprintCallable, Category="WebUI Host")
	void StopWebUIServer();

	UFUNCTION(BlueprintPure, Category="WebUI Host")
	FString GetWebUIId() const;

	UFUNCTION(BlueprintPure, Category="WebUI Host")
	int32 GetWebUIPort() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Host")
	bool bAutoStartServer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Host")
	FString WebUIId;

private:
	UWebUIRuntimeSubsystem* GetRuntimeSubsystem() const;
};

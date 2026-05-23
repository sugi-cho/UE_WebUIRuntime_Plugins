#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WebUIRuntimeSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Web UI Runtime"))
class WEBUIRUNTIME_API UWebUIRuntimeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;

	UPROPERTY(EditAnywhere, Config, Category="Web UI", meta=(ClampMin="1", ClampMax="65535"))
	int32 Port = 8765;
};

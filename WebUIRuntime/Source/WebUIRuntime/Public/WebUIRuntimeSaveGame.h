#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WebUIRuntimeSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FWebUIRuntimeSavedProperty
{
	GENERATED_BODY()

	UPROPERTY()
	FString OwnerType;

	UPROPERTY()
	FString ComponentId;

	UPROPERTY()
	FString PropertyName;

	UPROPERTY()
	FString ValueJson;
};

USTRUCT(BlueprintType)
struct FWebUIRuntimeSavedHostState
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FWebUIRuntimeSavedProperty> Properties;
};

UCLASS()
class WEBUIRUNTIME_API UWebUIRuntimeSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TMap<FString, FWebUIRuntimeSavedHostState> HostStates;
};

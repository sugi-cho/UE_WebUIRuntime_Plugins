#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WebUIPresentationSyncLibrary.generated.h"

class UBlueprint;

UCLASS()
class UWebUIPresentationSyncLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor, Category="WebUI")
	static bool SyncWebUIPresentationForBlueprint(UBlueprint* Blueprint);

	UFUNCTION(BlueprintCallable, CallInEditor, Category="WebUI")
	static int32 SyncAllLoadedWebUIPresentations();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="WebUI")
	static int32 SyncAllBlueprintWebUIPresentations();
};

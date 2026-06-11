#pragma once

#include "CoreMinimal.h"
#include "WebUIPresentationTypes.generated.h"

USTRUCT(BlueprintType)
struct WEBUIRUNTIME_API FWebUIPropertyPresentation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI", meta=(MultiLine=true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI")
	bool bUseSlider = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI", meta=(EditCondition="bUseSlider"))
	double Min = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI", meta=(EditCondition="bUseSlider"))
	double Max = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI", meta=(EditCondition="bUseSlider"))
	double Step = 0.0;
};

USTRUCT(BlueprintType)
struct WEBUIRUNTIME_API FWebUIButtonPresentation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI", meta=(MultiLine=true))
	FText Description;
};

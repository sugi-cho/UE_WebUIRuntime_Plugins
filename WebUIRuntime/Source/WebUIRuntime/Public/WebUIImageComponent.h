#pragma once

#include "WebUIComponentBase.h"
#include "WebUIImageComponent.generated.h"

class UTexture;

UENUM(BlueprintType)
enum class EWebUIImageSlot : uint8
{
	Preview UMETA(DisplayName="Preview"),
	Icon UMETA(DisplayName="Icon"),
	Inline UMETA(DisplayName="Inline")
};

UCLASS(BlueprintType, Blueprintable, ClassGroup=(WebUI), meta=(BlueprintSpawnableComponent))
class WEBUIRUNTIME_API UWebUIImageComponent : public UWebUIComponentBase
{
	GENERATED_BODY()

public:
	UWebUIImageComponent();

	UFUNCTION(BlueprintCallable, Category="WebUI Image")
	void SetWebUIImageEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category="WebUI Image")
	bool IsWebUIImageEnabled() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Image")
	EWebUIImageSlot WebUIImageSlot = EWebUIImageSlot::Preview;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Image")
	TObjectPtr<UTexture> SourceTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Image", meta=(ToolTip="Force RenderTarget alpha to fully opaque for this image. Disable this to preserve the RenderTarget alpha channel."))
	bool bForceOpaqueRenderTargetImage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Internal")
	bool bWebUIImageEnabled = true;
};

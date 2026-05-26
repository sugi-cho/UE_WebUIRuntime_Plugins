#pragma once

#include "CoreMinimal.h"

class UTexture;

class WEBUIRUNTIME_API IWebUIRuntimeTextureReader
{
public:
	virtual ~IWebUIRuntimeTextureReader() = default;

	virtual bool CanReadTexture(const UTexture* Texture) const = 0;
	virtual bool TryReadTexturePixels(UTexture* Texture, bool bForceOpaqueRenderTargetImage, TArray<FColor>& OutPixels, int32& OutWidth, int32& OutHeight, FString& OutError) const = 0;
};

class WEBUIRUNTIME_API FWebUIRuntimeTextureReaderRegistry
{
public:
	static void RegisterTextureReader(FName ReaderName, TSharedRef<IWebUIRuntimeTextureReader> Reader);
	static void UnregisterTextureReader(FName ReaderName);
	static bool CanReadTexture(UTexture* Texture);
	static bool TryReadTexturePixels(UTexture* Texture, bool bForceOpaqueRenderTargetImage, TArray<FColor>& OutPixels, int32& OutWidth, int32& OutHeight, FString& OutError);
};

#include "WebUIRuntimeTextureReader.h"

namespace
{
	FCriticalSection GTextureReaderRegistryLock;
	TMap<FName, TSharedRef<IWebUIRuntimeTextureReader>> GTextureReaders;
}

void FWebUIRuntimeTextureReaderRegistry::RegisterTextureReader(FName ReaderName, TSharedRef<IWebUIRuntimeTextureReader> Reader)
{
	if (ReaderName.IsNone())
	{
		return;
	}

	FScopeLock Lock(&GTextureReaderRegistryLock);
	GTextureReaders.Add(ReaderName, MoveTemp(Reader));
}

void FWebUIRuntimeTextureReaderRegistry::UnregisterTextureReader(FName ReaderName)
{
	if (ReaderName.IsNone())
	{
		return;
	}

	FScopeLock Lock(&GTextureReaderRegistryLock);
	GTextureReaders.Remove(ReaderName);
}

bool FWebUIRuntimeTextureReaderRegistry::CanReadTexture(UTexture* Texture)
{
	FScopeLock Lock(&GTextureReaderRegistryLock);
	for (const TPair<FName, TSharedRef<IWebUIRuntimeTextureReader>>& Entry : GTextureReaders)
	{
		const TSharedRef<IWebUIRuntimeTextureReader>& Reader = Entry.Value;
		if (Reader->CanReadTexture(Texture))
		{
			return true;
		}
	}

	return false;
}

bool FWebUIRuntimeTextureReaderRegistry::TryReadTexturePixels(UTexture* Texture, bool bForceOpaqueRenderTargetImage, TArray<FColor>& OutPixels, int32& OutWidth, int32& OutHeight, FString& OutError)
{
	FScopeLock Lock(&GTextureReaderRegistryLock);
	for (const TPair<FName, TSharedRef<IWebUIRuntimeTextureReader>>& Entry : GTextureReaders)
	{
		const TSharedRef<IWebUIRuntimeTextureReader>& Reader = Entry.Value;
		if (!Reader->CanReadTexture(Texture))
		{
			continue;
		}

		return Reader->TryReadTexturePixels(Texture, bForceOpaqueRenderTargetImage, OutPixels, OutWidth, OutHeight, OutError);
	}

	return false;
}

#include "WebUIRuntime.h"

#include "MediaTexture.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "TextureResource.h"
#include "WebUIRuntimeTextureReader.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogWebUIRuntime);

namespace
{
	class FWebUIMediaTextureReader final : public IWebUIRuntimeTextureReader
	{
	public:
		virtual bool CanReadTexture(const UTexture* Texture) const override
		{
			return Texture != nullptr && Texture->IsA<UMediaTexture>();
		}

		virtual bool TryReadTexturePixels(UTexture* Texture, bool /*bForceOpaqueRenderTargetImage*/, TArray<FColor>& OutPixels, int32& OutWidth, int32& OutHeight, FString& OutError) const override
		{
			OutPixels.Reset();
			OutWidth = 0;
			OutHeight = 0;

			UMediaTexture* MediaTexture = Cast<UMediaTexture>(Texture);
			if (!MediaTexture)
			{
				OutError = TEXT("Unsupported media texture");
				return false;
			}

			FTextureResource* Resource = MediaTexture->GetResource();
			if (!Resource || !Resource->TextureRHI.IsValid())
			{
				OutError = TEXT("Media texture resource is not ready");
				return false;
			}

			const int32 Width = FMath::Max(0, FMath::RoundToInt(MediaTexture->GetSurfaceWidth()));
			const int32 Height = FMath::Max(0, FMath::RoundToInt(MediaTexture->GetSurfaceHeight()));
			if (Width <= 0 || Height <= 0)
			{
				OutError = TEXT("Media texture has invalid size");
				return false;
			}

			FTextureRHIRef TextureRHI = Resource->TextureRHI;
			bool bReadSucceeded = false;
			ENQUEUE_RENDER_COMMAND(WebUIMediaTextureReadPixels)(
				[TextureRHI, Width, Height, &OutPixels, &bReadSucceeded](FRHICommandListImmediate& RHICmdList)
				{
					TArray<FColor> LocalPixels;
					LocalPixels.SetNumUninitialized(Width * Height);

					FReadSurfaceDataFlags Flags(RCM_UNorm);
					Flags.SetLinearToGamma(false);
					RHICmdList.ReadSurfaceData(TextureRHI.GetReference(), FIntRect(0, 0, Width, Height), LocalPixels, Flags);

					OutPixels = MoveTemp(LocalPixels);
					bReadSucceeded = true;
				});

			FlushRenderingCommands();

			if (!bReadSucceeded)
			{
				OutError = TEXT("Failed to read media texture pixels");
				return false;
			}

			OutWidth = Width;
			OutHeight = Height;
			return true;
		}
	};

	TSharedPtr<FWebUIMediaTextureReader> GWebUIMediaTextureReader;
}

void FWebUIRuntimeModule::StartupModule()
{
	UE_LOG(LogWebUIRuntime, Log, TEXT("WebUIRuntime module started"));
	GWebUIMediaTextureReader = MakeShared<FWebUIMediaTextureReader>();
	FWebUIRuntimeTextureReaderRegistry::RegisterTextureReader(TEXT("WebUI_MediaTexture"), GWebUIMediaTextureReader.ToSharedRef());
}

void FWebUIRuntimeModule::ShutdownModule()
{
	FWebUIRuntimeTextureReaderRegistry::UnregisterTextureReader(TEXT("WebUI_MediaTexture"));
	GWebUIMediaTextureReader.Reset();
	UE_LOG(LogWebUIRuntime, Log, TEXT("WebUIRuntime module stopped"));
}

IMPLEMENT_MODULE(FWebUIRuntimeModule, WebUIRuntime)


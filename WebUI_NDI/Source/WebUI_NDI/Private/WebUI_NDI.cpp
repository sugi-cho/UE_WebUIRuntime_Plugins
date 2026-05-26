#include "WebUI_NDI.h"

#include "WebUIRuntimeTextureReader.h"
#include "Objects/Media/NDIMediaTexture2D.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "TextureResource.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogWebUINDI);

namespace
{
	class FWebUINDITextureReader final : public IWebUIRuntimeTextureReader
	{
	public:
		virtual bool CanReadTexture(const UTexture* Texture) const override
		{
			return Texture != nullptr && Texture->IsA<UNDIMediaTexture2D>();
		}

		virtual bool TryReadTexturePixels(UTexture* Texture, bool /*bForceOpaqueRenderTargetImage*/, TArray<FColor>& OutPixels, int32& OutWidth, int32& OutHeight, FString& OutError) const override
		{
			OutPixels.Reset();
			OutWidth = 0;
			OutHeight = 0;

			UNDIMediaTexture2D* NDITexture = Cast<UNDIMediaTexture2D>(Texture);
			if (!NDITexture)
			{
				OutError = TEXT("Unsupported NDI texture");
				return false;
			}

			FTextureResource* Resource = NDITexture->GetResource();
			if (!Resource || !Resource->TextureRHI.IsValid())
			{
				OutError = TEXT("NDI texture resource is not ready");
				return false;
			}

			const int32 Width = FMath::Max(0, FMath::RoundToInt(NDITexture->GetSurfaceWidth()));
			const int32 Height = FMath::Max(0, FMath::RoundToInt(NDITexture->GetSurfaceHeight()));
			if (Width <= 0 || Height <= 0)
			{
				OutError = TEXT("NDI texture has invalid size");
				return false;
			}

			FTextureRHIRef TextureRHI = Resource->TextureRHI;
			bool bReadSucceeded = false;
			ENQUEUE_RENDER_COMMAND(WebUINDIReadTexturePixels)(
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
				OutError = TEXT("Failed to read NDI texture pixels");
				return false;
			}

			OutWidth = Width;
			OutHeight = Height;
			return true;
		}
	};

	TSharedPtr<FWebUINDITextureReader> GWebUINDITextureReader;
}

void FWebUINDIModule::StartupModule()
{
	UE_LOG(LogWebUINDI, Log, TEXT("WebUI_NDI module started"));
	GWebUINDITextureReader = MakeShared<FWebUINDITextureReader>();
	FWebUIRuntimeTextureReaderRegistry::RegisterTextureReader(TEXT("WebUI_NDI"), GWebUINDITextureReader.ToSharedRef());
}

void FWebUINDIModule::ShutdownModule()
{
	FWebUIRuntimeTextureReaderRegistry::UnregisterTextureReader(TEXT("WebUI_NDI"));
	GWebUINDITextureReader.Reset();
	UE_LOG(LogWebUINDI, Log, TEXT("WebUI_NDI module stopped"));
}

IMPLEMENT_MODULE(FWebUINDIModule, WebUI_NDI)

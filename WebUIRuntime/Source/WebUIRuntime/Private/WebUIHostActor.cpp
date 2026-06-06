#include "WebUIHostActor.h"

#include "Components/SceneComponent.h"
#include "WebUIHostComponent.h"

AWebUIHostActor::AWebUIHostActor()
{
	SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRootComponent);

	WebUIHostComponent = CreateDefaultSubobject<UWebUIHostComponent>(TEXT("WebUIHostComponent"));
}

void AWebUIHostActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AWebUIHostActor::BeginPlay()
{
	Super::BeginPlay();
}

bool AWebUIHostActor::StartWebUIServer()
{
	if (WebUIHostComponent)
	{
		return WebUIHostComponent->StartWebUIServer();
	}
	return false;
}

void AWebUIHostActor::StopWebUIServer()
{
	if (WebUIHostComponent)
	{
		WebUIHostComponent->StopWebUIServer();
	}
}

UWebUIHostComponent* AWebUIHostActor::GetWebUIHostComponent() const
{
	return WebUIHostComponent;
}

FString AWebUIHostActor::GetWebUIId() const
{
	return WebUIHostComponent ? WebUIHostComponent->GetWebUIId() : GetName();
}

FString AWebUIHostActor::GetDescription() const
{
	return WebUIHostComponent ? WebUIHostComponent->GetDescription() : GetName();
}

bool AWebUIHostActor::ShouldAutoStartServer() const
{
	return WebUIHostComponent ? WebUIHostComponent->bAutoStartServer : true;
}

void AWebUIHostActor::RegisterWebUIButton(FName ButtonId)
{
	if (WebUIHostComponent && !ButtonId.IsNone())
	{
		WebUIHostComponent->RegisterWebUIButton(ButtonId);
	}
}

void AWebUIHostActor::UnregisterWebUIButton(FName ButtonId)
{
	if (WebUIHostComponent)
	{
		WebUIHostComponent->UnregisterWebUIButton(ButtonId);
	}
}

void AWebUIHostActor::ClearWebUIButtons()
{
	if (WebUIHostComponent)
	{
		WebUIHostComponent->ClearWebUIButtons();
	}
}

const TArray<FName>& AWebUIHostActor::GetWebUIButtons() const
{
	static const TArray<FName> EmptyButtons;
	return WebUIHostComponent ? WebUIHostComponent->GetWebUIButtons() : EmptyButtons;
}

void AWebUIHostActor::NotifyWebUIButtonClicked(FName ButtonId)
{
	OnWebUIButtonClicked.Broadcast(ButtonId);
	K2_OnWebUIButtonClicked(ButtonId);
}

FString AWebUIHostActor::GetBrowserURL() const
{
	return WebUIHostComponent ? WebUIHostComponent->GetBrowserURL() : FString();
}

FString AWebUIHostActor::GetEmbeddedURL() const
{
	return WebUIHostComponent ? WebUIHostComponent->GetEmbeddedURL() : FString();
}

UTexture2D* AWebUIHostActor::CreateURLQRCodeTexture(const FString& URL, const int32 PixelsPerModule, const int32 QuietZoneModules) const
{
	return WebUIHostComponent ? WebUIHostComponent->CreateURLQRCodeTexture(URL, PixelsPerModule, QuietZoneModules) : nullptr;
}

UTexture2D* AWebUIHostActor::CreateBrowserURLQRCodeTexture(const int32 PixelsPerModule, const int32 QuietZoneModules) const
{
	return WebUIHostComponent ? WebUIHostComponent->CreateBrowserURLQRCodeTexture(PixelsPerModule, QuietZoneModules) : nullptr;
}

UTexture2D* AWebUIHostActor::CreateEmbeddedURLQRCodeTexture(const int32 PixelsPerModule, const int32 QuietZoneModules) const
{
	return WebUIHostComponent ? WebUIHostComponent->CreateEmbeddedURLQRCodeTexture(PixelsPerModule, QuietZoneModules) : nullptr;
}

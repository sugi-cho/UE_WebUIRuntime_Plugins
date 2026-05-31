#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "HttpRouteHandle.h"
#include "Subsystems/WorldSubsystem.h"
#include "WebUIRuntimeSubsystem.generated.h"

class FJsonObject;
class FJsonValue;
class IHttpRouter;
class AActor;
class UActorComponent;
class UWebUIHostComponent;
class UWebUIImageComponent;
class UTexture;
struct FHttpServerRequest;

UCLASS()
class WEBUIRUNTIME_API UWebUIRuntimeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;

	void RegisterHost(UWebUIHostComponent* Host);
	void UnregisterHost(UWebUIHostComponent* Host);
	void NotifyWebUIComponentStateChanged(UActorComponent* Component);

	UFUNCTION(BlueprintCallable, Category="WebUI")
	bool StartServerFromSettings();

	UFUNCTION(BlueprintCallable, Category="WebUI")
	void StopServer();

	UFUNCTION(BlueprintPure, Category="WebUI")
	bool IsServerRunning() const;

	UFUNCTION(BlueprintPure, Category="WebUI")
	int32 GetServerPort() const;

	void SavePersistedState(UWebUIHostComponent* Host);
	void LoadPersistedState(UWebUIHostComponent* Host);

private:
	FString GetWebUITitle() const;
	bool StartServer(int32 Port);
	bool HandleWebUI(const FHttpServerRequest& Request, const TFunction<void(TUniquePtr<struct FHttpServerResponse>&&)>& OnComplete);
	bool HandleSchema(const FHttpServerRequest& Request, const TFunction<void(TUniquePtr<struct FHttpServerResponse>&&)>& OnComplete);
	bool HandleImage(const FHttpServerRequest& Request, const TFunction<void(TUniquePtr<struct FHttpServerResponse>&&)>& OnComplete);
	bool HandleProperty(const FHttpServerRequest& Request, const TFunction<void(TUniquePtr<struct FHttpServerResponse>&&)>& OnComplete);
	bool HandleButton(const FHttpServerRequest& Request, const TFunction<void(TUniquePtr<struct FHttpServerResponse>&&)>& OnComplete);
	bool HandleAction(const FHttpServerRequest& Request, const TFunction<void(TUniquePtr<struct FHttpServerResponse>&&)>& OnComplete);

	TSharedRef<FJsonObject> BuildSchema() const;
	TSharedPtr<FJsonObject> BuildActorSchema(AActor* Actor, UWebUIHostComponent* Host) const;
	TSharedPtr<FJsonObject> BuildComponentSchema(UActorComponent* Component, const FString& WebUIId) const;
	UObject* FindPropertyOwner(const FString& WebUIId, const FString& OwnerType, const FString& ComponentId, UWebUIHostComponent*& OutHost) const;
	UActorComponent* FindComponent(const FString& WebUIId, const FString& ComponentId) const;
	FString BuildImageUrl(const FString& WebUIId, const FString& ComponentId) const;
	TUniquePtr<struct FHttpServerResponse> MakeImageResponse(const UWebUIImageComponent* ImageComponent, FString& OutError) const;
	bool TryGetTexturePixels(UTexture* Texture, bool bForceOpaqueRenderTargetImage, TArray<FColor>& OutPixels, int32& OutWidth, int32& OutHeight, FString& OutError) const;
	bool EncodeTextureToPNG(UTexture* Texture, bool bForceOpaqueRenderTargetImage, TArray<uint8>& OutCompressedBytes, int32& OutWidth, int32& OutHeight, FString& OutError) const;
	bool StartWebSocketServer(int32 Port);
	void StopWebSocketServer();
	bool TickWebSocketStreaming(float DeltaTime);

	bool SetPropertyFromJson(UObject* Owner, UWebUIHostComponent* Host, const FString& PropertyName, const TSharedPtr<FJsonValue>& Value, FString& OutError, bool bPersistAfterChange = true);
	TSharedPtr<FJsonValue> PropertyToJsonValue(FProperty* Property, const void* Container) const;
	FString GetPropertyWebUIType(FProperty* Property) const;
	bool IsWebUIProperty(FProperty* Property) const;
	void ApplyHttpServerBindAddressSetting() const;
	bool SerializeJsonValue(const TSharedPtr<FJsonValue>& Value, FString& OutJson) const;
	TSharedPtr<FJsonValue> DeserializeJsonValue(const FString& Json, FString& OutError) const;

	TUniquePtr<struct FHttpServerResponse> MakeJsonResponse(const TSharedRef<FJsonObject>& Object) const;
	TSharedPtr<FJsonObject> ParseRequestJson(const FHttpServerRequest& Request, FString& OutError) const;
	void AddNoCacheHeaders(struct FHttpServerResponse& Response) const;

private:
	UPROPERTY()
	TArray<TObjectPtr<UWebUIHostComponent>> Hosts;

	TSharedPtr<IHttpRouter> Router;
	TArray<FHttpRouteHandle> RouteHandles;
	int32 ActivePort = 0;
	int32 ActiveWebSocketPort = 0;
	int32 SchemaRevision = 0;
	FTSTicker::FDelegateHandle WebSocketTickHandle;
	class FWebUIRenderTargetWebSocketServer* WebSocketServer = nullptr;
};

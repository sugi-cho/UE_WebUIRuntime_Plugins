#pragma once

#include "CoreMinimal.h"
#include "HttpRouteHandle.h"
#include "Subsystems/WorldSubsystem.h"
#include "WebUIRuntimeSubsystem.generated.h"

class FJsonObject;
class FJsonValue;
class IHttpRouter;
class UActorComponent;
class UWebUIHostComponent;
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

	UFUNCTION(BlueprintCallable, Category="WebUI")
	bool StartServer(int32 Port = 8765);

	UFUNCTION(BlueprintCallable, Category="WebUI")
	void StopServer();

	UFUNCTION(BlueprintPure, Category="WebUI")
	bool IsServerRunning() const;

	UFUNCTION(BlueprintPure, Category="WebUI")
	int32 GetServerPort() const;

private:
	bool HandleWebUI(const FHttpServerRequest& Request, const TFunction<void(TUniquePtr<struct FHttpServerResponse>&&)>& OnComplete);
	bool HandleSchema(const FHttpServerRequest& Request, const TFunction<void(TUniquePtr<struct FHttpServerResponse>&&)>& OnComplete);
	bool HandleProperty(const FHttpServerRequest& Request, const TFunction<void(TUniquePtr<struct FHttpServerResponse>&&)>& OnComplete);
	bool HandleButton(const FHttpServerRequest& Request, const TFunction<void(TUniquePtr<struct FHttpServerResponse>&&)>& OnComplete);

	TSharedRef<FJsonObject> BuildSchema() const;
	TSharedPtr<FJsonObject> BuildComponentSchema(UActorComponent* Component) const;
	UActorComponent* FindComponent(const FString& WebUIId, const FString& ComponentId) const;

	bool SetPropertyFromJson(UActorComponent* Component, const FString& PropertyName, const TSharedPtr<FJsonValue>& Value, FString& OutError);
	TSharedPtr<FJsonValue> PropertyToJsonValue(FProperty* Property, const void* Container) const;
	FString GetPropertyWebUIType(FProperty* Property) const;
	bool IsWebUIProperty(FProperty* Property) const;

	TUniquePtr<struct FHttpServerResponse> MakeJsonResponse(const TSharedRef<FJsonObject>& Object) const;
	TSharedPtr<FJsonObject> ParseRequestJson(const FHttpServerRequest& Request, FString& OutError) const;

private:
	UPROPERTY()
	TArray<TObjectPtr<UWebUIHostComponent>> Hosts;

	TSharedPtr<IHttpRouter> Router;
	TArray<FHttpRouteHandle> RouteHandles;
	int32 ActivePort = 0;
};

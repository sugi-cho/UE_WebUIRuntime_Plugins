#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WebUIRuntimeSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Web UI Runtime"))
class WEBUIRUNTIME_API UWebUIRuntimeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;

	UPROPERTY(EditAnywhere, Config, Category="Web UI", meta=(ClampMin="1", ClampMax="65535", ToolTip="TCP port used by the Web UI HTTP server."))
	int32 Port = 8765;

	UPROPERTY(EditAnywhere, Config, Category="Web UI", meta=(ClampMin="0", ClampMax="65535", ToolTip="TCP port used by the WebSocket stream server. Set 0 to disable streaming."))
	int32 WebSocketPort = 8766;

	UPROPERTY(EditAnywhere, Config, Category="Web UI", meta=(ToolTip="Allow LAN devices to access the Web UI by binding the HTTP server to any address."))
	bool bAllowRemoteAccess = false;
};

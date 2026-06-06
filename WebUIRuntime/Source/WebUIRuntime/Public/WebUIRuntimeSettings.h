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

	UPROPERTY(EditAnywhere, Config, Category="Web UI", meta=(ClampMin="64", ClampMax="4096", ToolTip="Maximum width or height for streamed images. Larger images are downscaled before being sent."))
	int32 MaxStreamingImageDimension = 512;

	UPROPERTY(EditAnywhere, Config, Category="Web UI", meta=(ToolTip="Allow LAN devices to access the Web UI by binding the HTTP server to any address."))
	bool bAllowRemoteAccess = false;

	UPROPERTY(EditAnywhere, Config, Category="Mobile Controller", meta=(ToolTip="Enable the landscape mobile touch controller in the Web UI."))
	bool bEnableMobileController = false;

	UPROPERTY(EditAnywhere, Config, Category="Mobile Controller", meta=(ClampMin="0", ToolTip="PlayerController index controlled by the mobile controller."))
	int32 TargetPlayerIndex = 0;

	UPROPERTY(EditAnywhere, Config, Category="Mobile Controller", meta=(ClampMin="0.0", ClampMax="1.0", ToolTip="Dead zone applied to the mobile movement pad."))
	float MoveDeadZone = 0.12f;

	UPROPERTY(EditAnywhere, Config, Category="Mobile Controller", meta=(ClampMin="0.0", ToolTip="Scale applied to mobile movement input."))
	float MoveScale = 1.0f;

	UPROPERTY(EditAnywhere, Config, Category="Mobile Controller", meta=(ClampMin="0.0", ToolTip="Horizontal look sensitivity for mobile control."))
	float LookSensitivityX = 0.08f;

	UPROPERTY(EditAnywhere, Config, Category="Mobile Controller", meta=(ClampMin="0.0", ToolTip="Vertical look sensitivity for mobile control."))
	float LookSensitivityY = 0.08f;

	UPROPERTY(EditAnywhere, Config, Category="Mobile Controller", meta=(ToolTip="Invert the mobile controller look Y axis."))
	bool bInvertLookY = false;

	UPROPERTY(EditAnywhere, Config, Category="Mobile Controller", meta=(ClampMin="0.05", ToolTip="Seconds before stale mobile control input is cleared."))
	float ControlTimeoutSeconds = 0.2f;

	UPROPERTY(EditAnywhere, Config, Category="Mobile Controller", meta=(ToolTip="Require a control token query parameter before accepting mobile control input."))
	bool bRequireControlToken = true;

	UPROPERTY(EditAnywhere, Config, Category="Mobile Controller", meta=(ToolTip="Token expected from the Web UI query parameter controlToken."))
	FString ControlToken;
};

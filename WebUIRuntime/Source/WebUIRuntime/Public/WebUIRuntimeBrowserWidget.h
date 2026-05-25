#pragma once

#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "WebUIRuntimeBrowserWidget.generated.h"

class UOverlay;
class UWebBrowser;
class UWebUIRuntimeSubsystem;
class SWidget;

UCLASS(BlueprintType, Blueprintable)
class WEBUIRUNTIME_API UWebUIRuntimeBrowserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void SynchronizeProperties() override;

	UFUNCTION(BlueprintCallable, Category="WebUI Runtime")
	void LoadWebUI();

	UFUNCTION(BlueprintCallable, Category="WebUI Runtime")
	void ReloadWebUI();

	UFUNCTION(BlueprintCallable, Category="WebUI Runtime")
	void SetWebUIId(const FString& InWebUIId);

	UFUNCTION(BlueprintCallable, Category="WebUI Runtime")
	void SetUseEmbedMode(bool bInUseEmbedMode);

	UFUNCTION(BlueprintCallable, Category="WebUI Runtime")
	void SetOverrideURL(const FString& InOverrideURL);

	UFUNCTION(BlueprintPure, Category="WebUI Runtime")
	FString GetWebUIURL() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Runtime")
	bool bAutoLoadOnConstruct = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Runtime")
	bool bUseEmbedMode = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Runtime")
	bool bSupportsTransparency = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Runtime")
	FString WebUIId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Runtime", meta=(MultiLine=true))
	FString AdditionalQueryString;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Runtime", meta=(MultiLine=true))
	FString OverrideURL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI Runtime")
	bool bPreferLocalhost = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="WebUI Runtime", meta=(BindWidgetOptional, AllowPrivateAccess="true"))
	TObjectPtr<UWebBrowser> WebBrowser;

private:
	void EnsureBrowserWidget();
	void ApplyBrowserSettings();
	void TryLoadWebUI(int32 RetryCount);
	void ScheduleLoadRetry(int32 RetryCount, float DelaySeconds);
	void ClearLoadRetryTimer();
	void OnWidgetStateChanged();

	UWebUIRuntimeSubsystem* GetRuntimeSubsystem() const;
	FString BuildWebUIURL() const;
	FString BuildBaseWebUIURL() const;
	static FString NormalizeQueryString(const FString& InQueryString);
	static void AppendQueryParameter(FString& InURL, const FString& Key, const FString& Value);
	static void AppendRawQueryString(FString& InURL, const FString& RawQueryString);

private:
	UPROPERTY(Transient)
	TObjectPtr<UOverlay> NativeRootOverlay;

	FTimerHandle LoadRetryTimerHandle;
};

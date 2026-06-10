#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "WebUIComponentBase.h"
#include "WebUIFileBrowserComponent.generated.h"

class FJsonObject;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnWebUIFileSelected,
	const FString&,
	FilePath,
	const FString&,
	RelativePath
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnWebUIFolderSelected,
	const FString&,
	FolderPath
);

struct FWebUIFileBrowserEntry
{
	FString Id;
	FString Name;
	FString RelativePath;
	FString AbsolutePath;
	FString Extension;
	bool bDirectory = false;
	bool bHasChildren = false;
	bool bSelected = false;
	bool bAllowed = true;
	int64 Size = 0;
	FDateTime ModifiedUtc;

	TSharedRef<FJsonObject> ToJsonObject() const;
};

UCLASS(BlueprintType, Blueprintable, ClassGroup=(WebUI), meta=(BlueprintSpawnableComponent))
class WEBUIRUNTIME_API UWebUIFileBrowserComponent : public UWebUIComponentBase
{
	GENERATED_BODY()

public:
	UWebUIFileBrowserComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="WebUI File Browser")
	void RefreshFileBrowser();

	UFUNCTION(BlueprintCallable, Category="WebUI File Browser")
	bool SelectFileByRelativePath(const FString& RelativePath);

	UFUNCTION(BlueprintCallable, Category="WebUI File Browser")
	bool ClearSelection();

	UFUNCTION(BlueprintCallable, Category="WebUI File Browser")
	bool OpenFolderDialogFromWebUI(FString& OutSelectedFolder, FString& OutError);

	UFUNCTION(BlueprintCallable, Category="WebUI File Browser")
	bool SetTargetFolderFromPersistedPath(const FString& PersistedFolderPath, FString& OutError);

	UFUNCTION(BlueprintPure, Category="WebUI File Browser")
	FString GetResolvedRootPath() const;

	UFUNCTION(BlueprintPure, Category="WebUI File Browser")
	FString GetSelectedFilePath() const { return SelectedFilePath; }

	UFUNCTION(BlueprintPure, Category="WebUI File Browser")
	FString GetRootLabelForWebUI() const;

	UFUNCTION(BlueprintPure, Category="WebUI File Browser")
	FString GetSelectedRelativePath() const { return SelectedRelativePath; }

	bool TryResolveRelativePath(const FString& RelativePath, FString& OutAbsolutePath, FString& OutError) const;

	bool SelectFileFromWebUI(
		const FString& RelativePath,
		FString& OutSelectedAbsolutePath,
		FString& OutSelectedRelativePath,
		FString& OutError
	);

	bool ListDirectoryForWebUI(
		const FString& RelativePath,
		TArray<FWebUIFileBrowserEntry>& OutEntries,
		bool& bOutTruncated,
		FString& OutNormalizedRelativePath,
		FString& OutError
	) const;

	TSharedRef<FJsonObject> BuildWebUICustomView(const FString& WebUIId) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI File Browser", meta=(RelativeToGameDir))
	FDirectoryPath TargetFolder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI File Browser")
	FString RootLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI File Browser|Folder Selection")
	bool bAllowWebUIFolderSelection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI File Browser|Folder Selection")
	FString FolderDialogTitle = TEXT("Select Folder");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI File Browser|Folder Selection", meta=(RelativeToGameDir))
	FDirectoryPath FolderDialogInitialPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI File Browser|Folder Selection")
	bool bClearSelectionWhenFolderChanged = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI File Browser")
	bool bLazyLoadSubFolders = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI File Browser")
	bool bFoldersFirst = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI File Browser")
	bool bShowHiddenFiles = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI File Browser", meta=(ClampMin="1"))
	int32 MaxEntriesPerDirectory = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI File Browser", meta=(ClampMin="0"))
	int32 MaxScanDepth = 32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI File Browser")
	TArray<FString> AllowedExtensions;

	UPROPERTY(BlueprintAssignable, Category="WebUI File Browser")
	FOnWebUIFileSelected OnFileSelected;

	UPROPERTY(BlueprintAssignable, Category="WebUI File Browser")
	FOnWebUIFolderSelected OnFolderSelected;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category="WebUI File Browser", DisplayName="OnFileSelected")
	void K2_OnFileSelected(const FString& FilePath, const FString& RelativePath);

	UFUNCTION(BlueprintImplementableEvent, Category="WebUI File Browser", DisplayName="OnFolderSelected")
	void K2_OnFolderSelected(const FString& FolderPath);

private:
	bool TryResolveRelativePathInternal(
		const FString& RelativePath,
		FString& OutAbsolutePath,
		FString& OutNormalizedRelativePath,
		FString& OutError
	) const;

	bool IsAllowedFileName(const FString& FileName) const;
	bool ShouldHideEntry(const FString& Name) const;
	bool DirectoryHasDisplayableChildren(const FString& AbsoluteDirectory, int32 Depth) const;
	void NotifyFileBrowserStateChanged();
	FString NormalizeAllowedExtension(const FString& Extension) const;
	bool TryFindOwningWebUIHost(class UWebUIHostComponent*& OutHost) const;
	void SavePersistedFolderIfEnabled() const;
	void LoadPersistedFolderIfEnabled();
	FString GetFolderPersistenceSection(const class UWebUIHostComponent* Host) const;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="WebUI File Browser", meta=(AllowPrivateAccess="true"))
	FString SelectedFilePath;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="WebUI File Browser", meta=(AllowPrivateAccess="true"))
	FString SelectedRelativePath;
};

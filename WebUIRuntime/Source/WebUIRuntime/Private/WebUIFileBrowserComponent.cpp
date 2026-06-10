#include "WebUIFileBrowserComponent.h"

#include "HAL/FileManager.h"
#include "Json.h"
#include "Misc/Paths.h"
#include "WebUIRuntimeSubsystem.h"

#if WITH_EDITOR
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "IDesktopPlatform.h"
#endif

namespace
{
	FString NormalizePathForCompare(FString Path)
	{
		Path.ReplaceInline(TEXT("\\"), TEXT("/"));
		FPaths::NormalizeFilename(Path);
		FPaths::CollapseRelativeDirectories(Path);
		while (Path.Len() > 1 && Path.EndsWith(TEXT("/")))
		{
			Path.LeftChopInline(1);
		}
		return Path;
	}

	FString NormalizeRelativePath(FString RelativePath)
	{
		RelativePath.TrimStartAndEndInline();
		RelativePath.ReplaceInline(TEXT("\\"), TEXT("/"));
		while (RelativePath.StartsWith(TEXT("/")))
		{
			RelativePath.RightChopInline(1);
		}
		FPaths::NormalizeFilename(RelativePath);
		FPaths::CollapseRelativeDirectories(RelativePath);
		while (RelativePath.StartsWith(TEXT("./")))
		{
			RelativePath.RightChopInline(2);
		}
		if (RelativePath == TEXT("."))
		{
			RelativePath.Reset();
		}
		while (RelativePath.Len() > 1 && RelativePath.EndsWith(TEXT("/")))
		{
			RelativePath.LeftChopInline(1);
		}
		return RelativePath;
	}

	int32 GetRelativeDepth(const FString& RelativePath)
	{
		TArray<FString> Segments;
		RelativePath.ParseIntoArray(Segments, TEXT("/"), true);
		return Segments.Num();
	}

	FString CombineRelativePath(const FString& ParentRelativePath, const FString& Name)
	{
		if (ParentRelativePath.IsEmpty())
		{
			return Name;
		}
		return ParentRelativePath / Name;
	}

	bool IsDotDirectoryName(const FString& Name)
	{
		return Name == TEXT(".") || Name == TEXT("..");
	}

	bool IsFileBrowserAbsolutePath(const FString& Path)
	{
		if (Path.IsEmpty())
		{
			return false;
		}
		return !FPaths::IsRelative(Path) || Path.StartsWith(TEXT("/")) || Path.StartsWith(TEXT("\\")); 
	}

	FString MakeEntryId(const FString& RelativePath, bool bDirectory)
	{
		return FString::Printf(TEXT("%s:%s"), bDirectory ? TEXT("directory") : TEXT("file"), *RelativePath);
	}

	void AddStringArrayField(TSharedRef<FJsonObject> Object, const FString& FieldName, const TArray<FString>& Values)
	{
		TArray<TSharedPtr<FJsonValue>> JsonValues;
		JsonValues.Reserve(Values.Num());
		for (const FString& Value : Values)
		{
			JsonValues.Add(MakeShared<FJsonValueString>(Value));
		}
		Object->SetArrayField(FieldName, JsonValues);
	}
}

TSharedRef<FJsonObject> FWebUIFileBrowserEntry::ToJsonObject() const
{
	TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
	Object->SetStringField(TEXT("id"), Id);
	Object->SetStringField(TEXT("type"), bDirectory ? TEXT("directory") : TEXT("file"));
	Object->SetStringField(TEXT("name"), Name);
	Object->SetStringField(TEXT("relativePath"), RelativePath);
	Object->SetBoolField(TEXT("hasChildren"), bHasChildren);
	Object->SetBoolField(TEXT("selected"), bSelected);
	Object->SetBoolField(TEXT("allowed"), bAllowed);

	if (!bDirectory)
	{
		Object->SetStringField(TEXT("extension"), Extension);
		Object->SetNumberField(TEXT("size"), static_cast<double>(Size));
	}

	if (ModifiedUtc.GetTicks() > 0)
	{
		Object->SetStringField(TEXT("modifiedUtc"), ModifiedUtc.ToIso8601());
	}

	return Object;
}

UWebUIFileBrowserComponent::UWebUIFileBrowserComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetWebUIDisplayName(TEXT("File Browser"));
}

void UWebUIFileBrowserComponent::RefreshFileBrowser()
{
	if (!SelectedRelativePath.IsEmpty())
	{
		FString AbsolutePath;
		FString Error;
		if (!TryResolveRelativePath(SelectedRelativePath, AbsolutePath, Error) || !FPaths::FileExists(AbsolutePath))
		{
			SelectedRelativePath.Reset();
			SelectedFilePath.Reset();
		}
	}

	NotifyFileBrowserStateChanged();
}

bool UWebUIFileBrowserComponent::SelectFileByRelativePath(const FString& RelativePath)
{
	FString AbsolutePath;
	FString NormalizedRelativePath;
	FString Error;
	return SelectFileFromWebUI(RelativePath, AbsolutePath, NormalizedRelativePath, Error);
}

bool UWebUIFileBrowserComponent::ClearSelection()
{
	if (SelectedFilePath.IsEmpty() && SelectedRelativePath.IsEmpty())
	{
		return false;
	}

	SelectedFilePath.Reset();
	SelectedRelativePath.Reset();
	NotifyFileBrowserStateChanged();
	return true;
}


bool UWebUIFileBrowserComponent::OpenFolderDialogFromWebUI(FString& OutSelectedFolder, FString& OutError)
{
	OutSelectedFolder.Reset();
	OutError.Reset();

	if (!bAllowWebUIFolderSelection)
	{
		OutError = TEXT("Folder selection from WebUI is disabled.");
		return false;
	}

#if WITH_EDITOR
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		OutError = TEXT("DesktopPlatform is not available.");
		return false;
	}

	const void* ParentWindowHandle = nullptr;
	if (FSlateApplication::IsInitialized())
	{
		ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
	}

	FString DefaultPath = FolderDialogInitialPath.Path;
	DefaultPath.TrimStartAndEndInline();
	DefaultPath.ReplaceInline(TEXT("\\"), TEXT("/"));

	if (DefaultPath.IsEmpty())
	{
		DefaultPath = GetResolvedRootPath();
	}
	if (DefaultPath.IsEmpty())
	{
		DefaultPath = FPaths::ProjectDir();
	}

	if (DefaultPath.Equals(TEXT("/Game"), ESearchCase::IgnoreCase))
	{
		DefaultPath = FPaths::ProjectContentDir();
	}
	else if (DefaultPath.StartsWith(TEXT("/Game/"), ESearchCase::IgnoreCase))
	{
		DefaultPath = FPaths::Combine(FPaths::ProjectContentDir(), DefaultPath.RightChop(6));
	}
	else if (FPaths::IsRelative(DefaultPath))
	{
		DefaultPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), DefaultPath);
	}

	DefaultPath = NormalizePathForCompare(DefaultPath);
	if (!FPaths::DirectoryExists(DefaultPath))
	{
		DefaultPath = NormalizePathForCompare(FPaths::ProjectDir());
	}

	FString SelectedFolder;
	const bool bSelected = DesktopPlatform->OpenDirectoryDialog(
		ParentWindowHandle,
		FolderDialogTitle.IsEmpty() ? TEXT("Select Folder") : FolderDialogTitle,
		DefaultPath,
		SelectedFolder
	);

	if (!bSelected)
	{
		OutError = TEXT("Folder selection was cancelled.");
		return false;
	}

	SelectedFolder = NormalizePathForCompare(SelectedFolder);
	if (SelectedFolder.IsEmpty() || !FPaths::DirectoryExists(SelectedFolder))
	{
		OutError = TEXT("Selected folder does not exist.");
		return false;
	}

	TargetFolder.Path = SelectedFolder;
	if (bClearSelectionWhenFolderChanged)
	{
		SelectedFilePath.Reset();
		SelectedRelativePath.Reset();
	}

	OutSelectedFolder = SelectedFolder;
	OnFolderSelected.Broadcast(SelectedFolder);
	K2_OnFolderSelected(SelectedFolder);
	NotifyFileBrowserStateChanged();
	return true;
#else
	OutError = TEXT("Folder dialog is only available in editor builds.");
	return false;
#endif
}

FString UWebUIFileBrowserComponent::GetRootLabelForWebUI() const
{
	if (!RootLabel.IsEmpty())
	{
		return RootLabel;
	}

	const FString RootPath = GetResolvedRootPath();
	if (!RootPath.IsEmpty())
	{
		return FPaths::GetCleanFilename(RootPath);
	}

	return GetWebUIDisplayName();
}

FString UWebUIFileBrowserComponent::GetResolvedRootPath() const
{
	FString Path = TargetFolder.Path;
	Path.TrimStartAndEndInline();
	Path.ReplaceInline(TEXT("\\"), TEXT("/"));

	if (Path.IsEmpty())
	{
		return FString();
	}

	if (Path.Equals(TEXT("/Game"), ESearchCase::IgnoreCase))
	{
		Path = FPaths::ProjectContentDir();
	}
	else if (Path.StartsWith(TEXT("/Game/"), ESearchCase::IgnoreCase))
	{
		Path = FPaths::Combine(FPaths::ProjectContentDir(), Path.RightChop(6));
	}
	else if (FPaths::IsRelative(Path))
	{
		Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), Path);
	}

	return NormalizePathForCompare(Path);
}

bool UWebUIFileBrowserComponent::TryResolveRelativePath(const FString& RelativePath, FString& OutAbsolutePath, FString& OutError) const
{
	FString NormalizedRelativePath;
	return TryResolveRelativePathInternal(RelativePath, OutAbsolutePath, NormalizedRelativePath, OutError);
}

bool UWebUIFileBrowserComponent::TryResolveRelativePathInternal(
	const FString& RelativePath,
	FString& OutAbsolutePath,
	FString& OutNormalizedRelativePath,
	FString& OutError
) const
{
	OutAbsolutePath.Reset();
	OutNormalizedRelativePath.Reset();
	OutError.Reset();

	const FString RootPath = GetResolvedRootPath();
	if (RootPath.IsEmpty())
	{
		OutError = TEXT("TargetFolder is empty.");
		return false;
	}

	if (!FPaths::DirectoryExists(RootPath))
	{
		OutError = FString::Printf(TEXT("TargetFolder does not exist: %s"), *RootPath);
		return false;
	}

	FString RawRelativePath = RelativePath;
	RawRelativePath.TrimStartAndEndInline();
	RawRelativePath.ReplaceInline(TEXT("\\"), TEXT("/"));
	if (IsFileBrowserAbsolutePath(RawRelativePath))
	{
		OutError = TEXT("Absolute path is not allowed.");
		return false;
	}

	FString NormalizedRelativePath = NormalizeRelativePath(RelativePath);

	if (NormalizedRelativePath == TEXT("..") || NormalizedRelativePath.StartsWith(TEXT("../")) || NormalizedRelativePath.Contains(TEXT("/../")))
	{
		OutError = TEXT("Parent directory traversal is not allowed.");
		return false;
	}

	const int32 Depth = GetRelativeDepth(NormalizedRelativePath);
	if (MaxScanDepth >= 0 && Depth > MaxScanDepth)
	{
		OutError = TEXT("Path is deeper than MaxScanDepth.");
		return false;
	}

	FString Candidate = NormalizedRelativePath.IsEmpty()
		? RootPath
		: FPaths::Combine(RootPath, NormalizedRelativePath);
	Candidate = NormalizePathForCompare(Candidate);

	FString RootWithSlash = RootPath;
	if (!RootWithSlash.EndsWith(TEXT("/")))
	{
		RootWithSlash.AppendChar(TEXT('/'));
	}

	const bool bInsideRoot = Candidate.Equals(RootPath, ESearchCase::IgnoreCase) || Candidate.StartsWith(RootWithSlash, ESearchCase::IgnoreCase);
	if (!bInsideRoot)
	{
		OutError = TEXT("Path is outside of TargetFolder.");
		return false;
	}

	OutAbsolutePath = Candidate;
	OutNormalizedRelativePath = NormalizedRelativePath;
	return true;
}

bool UWebUIFileBrowserComponent::SelectFileFromWebUI(
	const FString& RelativePath,
	FString& OutSelectedAbsolutePath,
	FString& OutSelectedRelativePath,
	FString& OutError
)
{
	OutSelectedAbsolutePath.Reset();
	OutSelectedRelativePath.Reset();
	OutError.Reset();

	FString AbsolutePath;
	FString NormalizedRelativePath;
	if (!TryResolveRelativePathInternal(RelativePath, AbsolutePath, NormalizedRelativePath, OutError))
	{
		return false;
	}

	if (!FPaths::FileExists(AbsolutePath))
	{
		if (FPaths::DirectoryExists(AbsolutePath))
		{
			OutError = TEXT("Directory selection is not allowed.");
		}
		else
		{
			OutError = TEXT("File does not exist.");
		}
		return false;
	}

	if (!IsAllowedFileName(FPaths::GetCleanFilename(AbsolutePath)))
	{
		OutError = TEXT("File extension is not allowed.");
		return false;
	}

	SelectedFilePath = AbsolutePath;
	SelectedRelativePath = NormalizedRelativePath;
	OutSelectedAbsolutePath = SelectedFilePath;
	OutSelectedRelativePath = SelectedRelativePath;

	OnFileSelected.Broadcast(SelectedFilePath, SelectedRelativePath);
	K2_OnFileSelected(SelectedFilePath, SelectedRelativePath);
	NotifyFileBrowserStateChanged();
	return true;
}

bool UWebUIFileBrowserComponent::ListDirectoryForWebUI(
	const FString& RelativePath,
	TArray<FWebUIFileBrowserEntry>& OutEntries,
	bool& bOutTruncated,
	FString& OutNormalizedRelativePath,
	FString& OutError
) const
{
	OutEntries.Reset();
	bOutTruncated = false;
	OutNormalizedRelativePath.Reset();
	OutError.Reset();

	FString AbsoluteDirectory;
	FString NormalizedRelativePath;
	if (!TryResolveRelativePathInternal(RelativePath, AbsoluteDirectory, NormalizedRelativePath, OutError))
	{
		return false;
	}

	if (!FPaths::DirectoryExists(AbsoluteDirectory))
	{
		OutError = TEXT("Directory does not exist.");
		return false;
	}

	OutNormalizedRelativePath = NormalizedRelativePath;

	const int32 CurrentDepth = GetRelativeDepth(NormalizedRelativePath);
	if (MaxScanDepth >= 0 && CurrentDepth > MaxScanDepth)
	{
		OutError = TEXT("Directory is deeper than MaxScanDepth.");
		return false;
	}

	const FString SearchPattern = FPaths::Combine(AbsoluteDirectory, TEXT("*"));
	TArray<FString> DirectoryNames;
	TArray<FString> FileNames;
	IFileManager::Get().FindFiles(DirectoryNames, *SearchPattern, false, true);
	IFileManager::Get().FindFiles(FileNames, *SearchPattern, true, false);

	DirectoryNames.Sort([](const FString& A, const FString& B)
	{
		return A.Compare(B, ESearchCase::IgnoreCase) < 0;
	});

	FileNames.Sort([](const FString& A, const FString& B)
	{
		return A.Compare(B, ESearchCase::IgnoreCase) < 0;
	});

	auto CanAppendEntry = [&]()
	{
		if (MaxEntriesPerDirectory > 0 && OutEntries.Num() >= MaxEntriesPerDirectory)
		{
			bOutTruncated = true;
			return false;
		}
		return true;
	};

	auto AppendDirectory = [&](const FString& DirectoryName)
	{
		if (!CanAppendEntry() || IsDotDirectoryName(DirectoryName) || ShouldHideEntry(DirectoryName))
		{
			return;
		}

		if (MaxScanDepth >= 0 && CurrentDepth >= MaxScanDepth)
		{
			return;
		}

		const FString ChildRelativePath = CombineRelativePath(NormalizedRelativePath, DirectoryName);
		const FString ChildAbsolutePath = NormalizePathForCompare(FPaths::Combine(AbsoluteDirectory, DirectoryName));

		FWebUIFileBrowserEntry Entry;
		Entry.bDirectory = true;
		Entry.Name = DirectoryName;
		Entry.RelativePath = ChildRelativePath;
		Entry.AbsolutePath = ChildAbsolutePath;
		Entry.Id = MakeEntryId(ChildRelativePath, true);
		Entry.bHasChildren = DirectoryHasDisplayableChildren(ChildAbsolutePath, CurrentDepth + 1);
		Entry.ModifiedUtc = IFileManager::Get().GetTimeStamp(*ChildAbsolutePath);
		OutEntries.Add(MoveTemp(Entry));
	};

	auto AppendFile = [&](const FString& FileName)
	{
		if (!CanAppendEntry() || ShouldHideEntry(FileName) || !IsAllowedFileName(FileName))
		{
			return;
		}

		const FString ChildRelativePath = CombineRelativePath(NormalizedRelativePath, FileName);
		const FString ChildAbsolutePath = NormalizePathForCompare(FPaths::Combine(AbsoluteDirectory, FileName));

		FWebUIFileBrowserEntry Entry;
		Entry.bDirectory = false;
		Entry.Name = FileName;
		Entry.RelativePath = ChildRelativePath;
		Entry.AbsolutePath = ChildAbsolutePath;
		Entry.Extension = FPaths::GetExtension(FileName, true).ToLower();
		Entry.Id = MakeEntryId(ChildRelativePath, false);
		Entry.bSelected = !SelectedRelativePath.IsEmpty() && ChildRelativePath.Equals(SelectedRelativePath, ESearchCase::IgnoreCase);
		Entry.Size = IFileManager::Get().FileSize(*ChildAbsolutePath);
		Entry.ModifiedUtc = IFileManager::Get().GetTimeStamp(*ChildAbsolutePath);
		OutEntries.Add(MoveTemp(Entry));
	};

	if (bFoldersFirst)
	{
		for (const FString& DirectoryName : DirectoryNames)
		{
			AppendDirectory(DirectoryName);
		}
		for (const FString& FileName : FileNames)
		{
			AppendFile(FileName);
		}
	}
	else
	{
		for (const FString& DirectoryName : DirectoryNames)
		{
			AppendDirectory(DirectoryName);
		}
		for (const FString& FileName : FileNames)
		{
			AppendFile(FileName);
		}
		OutEntries.Sort([](const FWebUIFileBrowserEntry& A, const FWebUIFileBrowserEntry& B)
		{
			return A.Name.Compare(B.Name, ESearchCase::IgnoreCase) < 0;
		});
	}

	return true;
}

TSharedRef<FJsonObject> UWebUIFileBrowserComponent::BuildWebUICustomView(const FString& WebUIId) const
{
	TSharedRef<FJsonObject> CustomView = MakeShared<FJsonObject>();
	CustomView->SetStringField(TEXT("type"), TEXT("fileBrowser"));
	CustomView->SetStringField(TEXT("webUIId"), WebUIId);
	CustomView->SetStringField(TEXT("rootLabel"), GetRootLabelForWebUI());
	CustomView->SetStringField(TEXT("rootRelativePath"), FString());
	CustomView->SetBoolField(TEXT("allowFolderSelection"), bAllowWebUIFolderSelection);
	CustomView->SetStringField(TEXT("openFolderDialogEndpoint"), TEXT("/api/webui/file-browser/open-folder-dialog"));
	CustomView->SetStringField(TEXT("selectedRelativePath"), SelectedRelativePath);
	CustomView->SetBoolField(TEXT("supportsLazyLoad"), bLazyLoadSubFolders);
	CustomView->SetBoolField(TEXT("foldersFirst"), bFoldersFirst);
	CustomView->SetNumberField(TEXT("maxEntriesPerDirectory"), MaxEntriesPerDirectory);
	CustomView->SetStringField(TEXT("listEndpoint"), TEXT("/api/webui/file-browser/list"));
	CustomView->SetStringField(TEXT("selectEndpoint"), TEXT("/api/webui/file-browser/select"));
	CustomView->SetBoolField(TEXT("targetFolderConfigured"), !GetResolvedRootPath().IsEmpty());
	CustomView->SetBoolField(TEXT("targetFolderExists"), FPaths::DirectoryExists(GetResolvedRootPath()));
	AddStringArrayField(CustomView, TEXT("allowedExtensions"), AllowedExtensions);
	return CustomView;
}

bool UWebUIFileBrowserComponent::IsAllowedFileName(const FString& FileName) const
{
	if (AllowedExtensions.IsEmpty())
	{
		return true;
	}

	const FString Extension = NormalizeAllowedExtension(FPaths::GetExtension(FileName, true));
	for (const FString& AllowedExtension : AllowedExtensions)
	{
		const FString NormalizedAllowedExtension = NormalizeAllowedExtension(AllowedExtension);
		if (NormalizedAllowedExtension == TEXT(".*") || NormalizedAllowedExtension == TEXT("*"))
		{
			return true;
		}
		if (Extension.Equals(NormalizedAllowedExtension, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}

bool UWebUIFileBrowserComponent::ShouldHideEntry(const FString& Name) const
{
	return !bShowHiddenFiles && Name.StartsWith(TEXT("."));
}

bool UWebUIFileBrowserComponent::DirectoryHasDisplayableChildren(const FString& AbsoluteDirectory, int32 Depth) const
{
	if (MaxScanDepth >= 0 && Depth > MaxScanDepth)
	{
		return false;
	}

	const FString SearchPattern = FPaths::Combine(AbsoluteDirectory, TEXT("*"));
	TArray<FString> DirectoryNames;
	IFileManager::Get().FindFiles(DirectoryNames, *SearchPattern, false, true);
	for (const FString& DirectoryName : DirectoryNames)
	{
		if (!IsDotDirectoryName(DirectoryName) && !ShouldHideEntry(DirectoryName))
		{
			return true;
		}
	}

	TArray<FString> FileNames;
	IFileManager::Get().FindFiles(FileNames, *SearchPattern, true, false);
	for (const FString& FileName : FileNames)
	{
		if (!ShouldHideEntry(FileName) && IsAllowedFileName(FileName))
		{
			return true;
		}
	}

	return false;
}

void UWebUIFileBrowserComponent::NotifyFileBrowserStateChanged()
{
	if (UWorld* World = GetWorld())
	{
		if (UWebUIRuntimeSubsystem* Runtime = World->GetSubsystem<UWebUIRuntimeSubsystem>())
		{
			Runtime->NotifyWebUIComponentStateChanged(this);
		}
	}
}

FString UWebUIFileBrowserComponent::NormalizeAllowedExtension(const FString& Extension) const
{
	FString Normalized = Extension;
	Normalized.TrimStartAndEndInline();
	Normalized.ToLowerInline();
	if (Normalized.IsEmpty())
	{
		return Normalized;
	}
	if (Normalized == TEXT("*"))
	{
		return Normalized;
	}
	if (!Normalized.StartsWith(TEXT(".")))
	{
		Normalized.InsertAt(0, TEXT('.'));
	}
	return Normalized;
}

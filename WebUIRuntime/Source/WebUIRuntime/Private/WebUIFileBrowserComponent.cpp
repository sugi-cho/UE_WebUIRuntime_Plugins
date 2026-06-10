#include "WebUIFileBrowserComponent.h"

#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "Json.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "WebUIHostComponent.h"
#include "WebUIRuntimeSubsystem.h"

#if PLATFORM_WINDOWS
#include <thread>
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
#include <ShObjIdl.h>
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"
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

	FString SanitizeConfigSegment(FString Segment)
	{
		Segment.TrimStartAndEndInline();
		Segment.ReplaceInline(TEXT("["), TEXT("_"));
		Segment.ReplaceInline(TEXT("]"), TEXT("_"));
		Segment.ReplaceInline(TEXT("\r"), TEXT("_"));
		Segment.ReplaceInline(TEXT("\n"), TEXT("_"));
		return Segment.IsEmpty() ? TEXT("Default") : Segment;
	}

	FString ResolveFileBrowserDirectoryPath(FString Path, const FString& FallbackPath)
	{
		Path.TrimStartAndEndInline();
		Path.ReplaceInline(TEXT("\\"), TEXT("/"));

		if (Path.IsEmpty())
		{
			Path = FallbackPath;
		}

		if (Path.IsEmpty())
		{
			Path = FPaths::ProjectDir();
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

#if PLATFORM_WINDOWS
	FString MakeWindowsDialogError(const TCHAR* Context, HRESULT Result)
	{
		return FString::Printf(TEXT("%s failed. HRESULT=0x%08x"), Context, static_cast<uint32>(Result));
	}

	bool OpenWindowsFolderPicker(const FString& Title, const FString& DefaultPath, FString& OutSelectedFolder, FString& OutError)
	{
		OutSelectedFolder.Reset();
		OutError.Reset();

		bool bSelected = false;
		FString ThreadSelectedFolder;
		FString ThreadError;

		std::thread DialogThread([Title, DefaultPath, &bSelected, &ThreadSelectedFolder, &ThreadError]()
		{
			HRESULT CoInitResult = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
			const bool bDidInitializeCom = SUCCEEDED(CoInitResult);
			if (FAILED(CoInitResult))
			{
				ThreadError = MakeWindowsDialogError(TEXT("CoInitializeEx"), CoInitResult);
				return;
			}

			IFileOpenDialog* Dialog = nullptr;
			HRESULT Result = ::CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&Dialog));
			if (FAILED(Result) || !Dialog)
			{
				ThreadError = MakeWindowsDialogError(TEXT("CoCreateInstance(CLSID_FileOpenDialog)"), Result);
				if (bDidInitializeCom)
				{
					::CoUninitialize();
				}
				return;
			}

			DWORD Options = 0;
			if (SUCCEEDED(Dialog->GetOptions(&Options)))
			{
				Dialog->SetOptions(Options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR);
			}

			const FString DialogTitle = Title.IsEmpty() ? TEXT("Select Folder") : Title;
			Dialog->SetTitle(*DialogTitle);

			if (!DefaultPath.IsEmpty() && FPaths::DirectoryExists(DefaultPath))
			{
				IShellItem* DefaultFolderItem = nullptr;
				if (SUCCEEDED(::SHCreateItemFromParsingName(*DefaultPath, nullptr, IID_PPV_ARGS(&DefaultFolderItem))) && DefaultFolderItem)
				{
					Dialog->SetFolder(DefaultFolderItem);
					DefaultFolderItem->Release();
				}
			}

			Result = Dialog->Show(nullptr);
			if (Result == HRESULT_FROM_WIN32(ERROR_CANCELLED))
			{
				ThreadError = TEXT("Folder selection was cancelled.");
				Dialog->Release();
				if (bDidInitializeCom)
				{
					::CoUninitialize();
				}
				return;
			}

			if (FAILED(Result))
			{
				ThreadError = MakeWindowsDialogError(TEXT("IFileOpenDialog::Show"), Result);
				Dialog->Release();
				if (bDidInitializeCom)
				{
					::CoUninitialize();
				}
				return;
			}

			IShellItem* SelectedItem = nullptr;
			Result = Dialog->GetResult(&SelectedItem);
			if (FAILED(Result) || !SelectedItem)
			{
				ThreadError = MakeWindowsDialogError(TEXT("IFileOpenDialog::GetResult"), Result);
				Dialog->Release();
				if (bDidInitializeCom)
				{
					::CoUninitialize();
				}
				return;
			}

			PWSTR SelectedPath = nullptr;
			Result = SelectedItem->GetDisplayName(SIGDN_FILESYSPATH, &SelectedPath);
			if (FAILED(Result) || !SelectedPath)
			{
				ThreadError = MakeWindowsDialogError(TEXT("IShellItem::GetDisplayName"), Result);
				SelectedItem->Release();
				Dialog->Release();
				if (bDidInitializeCom)
				{
					::CoUninitialize();
				}
				return;
			}

			ThreadSelectedFolder = FString(SelectedPath);
			::CoTaskMemFree(SelectedPath);
			SelectedItem->Release();
			Dialog->Release();

			bSelected = true;
			if (bDidInitializeCom)
			{
				::CoUninitialize();
			}
		});

		DialogThread.join();

		OutSelectedFolder = ThreadSelectedFolder;
		OutError = ThreadError;
		return bSelected;
	}
#endif

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

void UWebUIFileBrowserComponent::BeginPlay()
{
	Super::BeginPlay();
	LoadPersistedFolderIfEnabled();
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

#if PLATFORM_WINDOWS
	FString DefaultPath = ResolveFileBrowserDirectoryPath(FolderDialogInitialPath.Path, GetResolvedRootPath());
	if (!FPaths::DirectoryExists(DefaultPath))
	{
		DefaultPath = NormalizePathForCompare(FPaths::ProjectDir());
	}

	FString SelectedFolder;
	if (!OpenWindowsFolderPicker(FolderDialogTitle, DefaultPath, SelectedFolder, OutError))
	{
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
	SavePersistedFolderIfEnabled();
	return true;
#else
	OutError = TEXT("Folder dialog is only available on Windows builds.");
	return false;
#endif
}

bool UWebUIFileBrowserComponent::SetTargetFolderFromPersistedPath(const FString& PersistedFolderPath, FString& OutError)
{
	OutError.Reset();

	FString NormalizedFolder = ResolveFileBrowserDirectoryPath(PersistedFolderPath, FString());
	if (NormalizedFolder.IsEmpty())
	{
		OutError = TEXT("Persisted folder path is empty.");
		return false;
	}

	if (!FPaths::DirectoryExists(NormalizedFolder))
	{
		OutError = FString::Printf(TEXT("Persisted folder does not exist: %s"), *NormalizedFolder);
		return false;
	}

	TargetFolder.Path = NormalizedFolder;
	if (bClearSelectionWhenFolderChanged)
	{
		SelectedFilePath.Reset();
		SelectedRelativePath.Reset();
	}

	NotifyFileBrowserStateChanged();
	return true;
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

bool UWebUIFileBrowserComponent::TryFindOwningWebUIHost(UWebUIHostComponent*& OutHost) const
{
	OutHost = nullptr;
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return false;
	}

	TArray<UWebUIHostComponent*> Hosts;
	Owner->GetComponents<UWebUIHostComponent>(Hosts);
	for (UWebUIHostComponent* Host : Hosts)
	{
		if (IsValid(Host))
		{
			OutHost = Host;
			return true;
		}
	}

	return false;
}

FString UWebUIFileBrowserComponent::GetFolderPersistenceSection(const UWebUIHostComponent* Host) const
{
	const FString HostId = IsValid(Host) ? Host->GetWebUIId() : FString();
	return FString::Printf(TEXT("WebUIRuntime.FileBrowserFolders.%s"), *SanitizeConfigSegment(HostId));
}

void UWebUIFileBrowserComponent::SavePersistedFolderIfEnabled() const
{
	UWebUIHostComponent* Host = nullptr;
	if (!TryFindOwningWebUIHost(Host) || !Host || !Host->IsAutoSaveChangedValuesEnabled())
	{
		return;
	}

	if (TargetFolder.Path.IsEmpty())
	{
		return;
	}

	if (GConfig)
	{
		GConfig->SetString(*GetFolderPersistenceSection(Host), *GetName(), *TargetFolder.Path, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void UWebUIFileBrowserComponent::LoadPersistedFolderIfEnabled()
{
	UWebUIHostComponent* Host = nullptr;
	if (!TryFindOwningWebUIHost(Host) || !Host || !Host->ShouldAutoLoadSavedValues())
	{
		return;
	}

	FString PersistedFolder;
	if (!GConfig || !GConfig->GetString(*GetFolderPersistenceSection(Host), *GetName(), PersistedFolder, GGameUserSettingsIni))
	{
		return;
	}

	FString Error;
	SetTargetFolderFromPersistedPath(PersistedFolder, Error);
}


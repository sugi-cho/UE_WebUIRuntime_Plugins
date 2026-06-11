#include "WebUIPresentationSyncLibrary.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Misc/ScopedSlowTask.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IPluginManager.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "WebUIComponentBase.h"
#include "WebUIHostComponent.h"
#include "WebUIPresentationTypes.h"

namespace
{
	bool IsWebUIOrderedName(const FString& Name)
	{
		int32 PrefixStart = Name.StartsWith(TEXT("WUI"), ESearchCase::IgnoreCase) ? 3 : 0;
		int32 PrefixEnd = PrefixStart;
		while (PrefixEnd < Name.Len() && FChar::IsDigit(Name[PrefixEnd]))
		{
			++PrefixEnd;
		}
		return PrefixEnd > PrefixStart && PrefixEnd < Name.Len() && (Name[PrefixEnd] == TEXT('_') || Name[PrefixEnd] == TEXT(' '));
	}

	bool HasPresentationChanged(const FWebUIPropertyPresentation& A, const FWebUIPropertyPresentation& B)
	{
		return A.DisplayName.ToString() != B.DisplayName.ToString()
			|| A.Description.ToString() != B.Description.ToString()
			|| A.bUseSlider != B.bUseSlider
			|| !FMath::IsNearlyEqual(A.Min, B.Min)
			|| !FMath::IsNearlyEqual(A.Max, B.Max)
			|| !FMath::IsNearlyEqual(A.Step, B.Step);
	}

	bool HasPresentationChanged(const FWebUIButtonPresentation& A, const FWebUIButtonPresentation& B)
	{
		return A.DisplayName.ToString() != B.DisplayName.ToString()
			|| A.Description.ToString() != B.Description.ToString();
	}

	FText ReadTextMeta(const FField* Field, const TCHAR* Key)
	{
		return Field && Field->HasMetaData(Key) ? FText::FromString(Field->GetMetaData(Key)) : FText::GetEmpty();
	}

	FText ReadTextMeta(const UFunction* Function, const TCHAR* Key)
	{
		return Function && Function->HasMetaData(Key) ? FText::FromString(Function->GetMetaData(Key)) : FText::GetEmpty();
	}

	bool TryReadDoubleMeta(const FField* Field, const TCHAR* Key, double& OutValue)
	{
		if (!Field || !Field->HasMetaData(Key))
		{
			return false;
		}
		return LexTryParseString(OutValue, *Field->GetMetaData(Key));
	}

	bool BuildPropertyPresentation(const FProperty* Property, FWebUIPropertyPresentation& OutPresentation)
	{
		if (!Property || !IsWebUIOrderedName(Property->GetName()))
		{
			return false;
		}

		OutPresentation.DisplayName = ReadTextMeta(Property, TEXT("DisplayName"));
		OutPresentation.Description = ReadTextMeta(Property, TEXT("ToolTip"));

		double Min = 0.0;
		double Max = 0.0;
		const bool bHasMin = TryReadDoubleMeta(Property, TEXT("UIMin"), Min) || TryReadDoubleMeta(Property, TEXT("ClampMin"), Min);
		const bool bHasMax = TryReadDoubleMeta(Property, TEXT("UIMax"), Max) || TryReadDoubleMeta(Property, TEXT("ClampMax"), Max);
		if (Property->IsA<FNumericProperty>() && bHasMin && bHasMax && Min < Max)
		{
			OutPresentation.bUseSlider = true;
			OutPresentation.Min = Min;
			OutPresentation.Max = Max;
			OutPresentation.Step = Property->IsA<FIntProperty>() ? 1.0 : FMath::Max((Max - Min) / 100.0, 0.001);
		}

		return OutPresentation.bUseSlider || !OutPresentation.DisplayName.IsEmptyOrWhitespace() || !OutPresentation.Description.IsEmptyOrWhitespace();
	}

	bool BuildButtonPresentation(const UFunction* Function, FWebUIButtonPresentation& OutPresentation)
	{
		if (!Function || Function->NumParms != 0 || Function->HasAnyFunctionFlags(FUNC_BlueprintPure | FUNC_Event) || !IsWebUIOrderedName(Function->GetName()))
		{
			return false;
		}

		OutPresentation.DisplayName = ReadTextMeta(Function, TEXT("DisplayName"));
		OutPresentation.Description = ReadTextMeta(Function, TEXT("ToolTip"));
		return !OutPresentation.DisplayName.IsEmptyOrWhitespace() || !OutPresentation.Description.IsEmptyOrWhitespace();
	}

	bool SyncPropertyPresentations(UObject* Object, TMap<FName, FWebUIPropertyPresentation>& Presentations)
	{
		if (!Object)
		{
			return false;
		}

		bool bChanged = false;
		for (TFieldIterator<FProperty> It(Object->GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
		{
			FProperty* Property = *It;
			FWebUIPropertyPresentation Presentation;
			if (!BuildPropertyPresentation(Property, Presentation))
			{
				continue;
			}

			FWebUIPropertyPresentation* Existing = Presentations.Find(Property->GetFName());
			if (!Existing || HasPresentationChanged(*Existing, Presentation))
			{
				Presentations.Add(Property->GetFName(), Presentation);
				bChanged = true;
			}
		}
		return bChanged;
	}

	bool SyncButtonPresentations(UObject* Object, TMap<FName, FWebUIButtonPresentation>& Presentations)
	{
		if (!Object)
		{
			return false;
		}

		bool bChanged = false;
		for (TFieldIterator<UFunction> It(Object->GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
		{
			UFunction* Function = *It;
			FWebUIButtonPresentation Presentation;
			if (!BuildButtonPresentation(Function, Presentation))
			{
				continue;
			}

			FWebUIButtonPresentation* Existing = Presentations.Find(Function->GetFName());
			if (!Existing || HasPresentationChanged(*Existing, Presentation))
			{
				Presentations.Add(Function->GetFName(), Presentation);
				bChanged = true;
			}
		}
		return bChanged;
	}
}

bool UWebUIPresentationSyncLibrary::SyncWebUIPresentationForBlueprint(UBlueprint* Blueprint)
{
	if (!Blueprint || !Blueprint->GeneratedClass)
	{
		return false;
	}

	UObject* CDO = Blueprint->GeneratedClass->GetDefaultObject();
	if (!CDO)
	{
		return false;
	}

	bool bChanged = false;
	if (AActor* ActorCDO = Cast<AActor>(CDO))
	{
		if (UWebUIHostComponent* Host = ActorCDO->FindComponentByClass<UWebUIHostComponent>())
		{
			Host->Modify();
			bChanged |= SyncPropertyPresentations(ActorCDO, Host->ActorPropertyPresentations);
			bChanged |= SyncButtonPresentations(ActorCDO, Host->ButtonPresentations);
		}

		TArray<UActorComponent*> Components;
		ActorCDO->GetComponents(Components);
		for (UActorComponent* Component : Components)
		{
			if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Component))
			{
				WebUIComponent->Modify();
				bChanged |= SyncPropertyPresentations(WebUIComponent, WebUIComponent->PropertyPresentations);
				bChanged |= SyncButtonPresentations(WebUIComponent, WebUIComponent->ButtonPresentations);
			}
		}
	}
	else if (UWebUIComponentBase* WebUIComponentCDO = Cast<UWebUIComponentBase>(CDO))
	{
		WebUIComponentCDO->Modify();
		bChanged |= SyncPropertyPresentations(WebUIComponentCDO, WebUIComponentCDO->PropertyPresentations);
		bChanged |= SyncButtonPresentations(WebUIComponentCDO, WebUIComponentCDO->ButtonPresentations);
	}

	if (bChanged)
	{
		Blueprint->Modify();
		Blueprint->MarkPackageDirty();
	}

	return bChanged;
}

int32 UWebUIPresentationSyncLibrary::SyncAllLoadedWebUIPresentations()
{
	int32 Count = 0;
	for (TObjectIterator<UBlueprint> It; It; ++It)
	{
		if (SyncWebUIPresentationForBlueprint(*It))
		{
			++Count;
		}
	}
	return Count;
}

int32 UWebUIPresentationSyncLibrary::SyncAllBlueprintWebUIPresentations()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FARFilter Filter;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> BlueprintAssets;
	AssetRegistry.GetAssets(Filter, BlueprintAssets);

	FScopedSlowTask SlowTask(BlueprintAssets.Num(), NSLOCTEXT("WebUIRuntimeEditor", "SyncAllBlueprintWebUIPresentations", "Syncing WebUI presentations"));
	SlowTask.MakeDialog(true);

	int32 Count = 0;
	TArray<UPackage*> PackagesToSave;
	for (const FAssetData& AssetData : BlueprintAssets)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromName(AssetData.AssetName));
		if (SlowTask.ShouldCancel())
		{
			break;
		}

		UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
		if (!Blueprint)
		{
			continue;
		}

		if (SyncWebUIPresentationForBlueprint(Blueprint))
		{
			++Count;
			if (UPackage* Package = Blueprint->GetOutermost())
			{
				PackagesToSave.AddUnique(Package);
			}
		}
	}

	if (PackagesToSave.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, true);
	}

	return Count;
}

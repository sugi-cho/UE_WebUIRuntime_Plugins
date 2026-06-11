#include "Modules/ModuleManager.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Notifications/NotificationManager.h"
#include "ToolMenus.h"
#include "WebUIPresentationSyncLibrary.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "WebUIRuntimeEditorModule"

class FWebUIRuntimeEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FWebUIRuntimeEditorModule::RegisterMenus));
	}

	virtual void ShutdownModule() override
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
	}

private:
	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
		if (!Menu)
		{
			return;
		}

		FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("WebUIRuntime"));
		Section.AddMenuEntry(
			TEXT("WebUIRuntimeSyncAllBlueprints"),
			LOCTEXT("SyncAllBlueprintsMenuLabel", "WebUI Sync All"),
			LOCTEXT("SyncAllBlueprintsMenuTooltip", "Sync WebUI presentation data for all Blueprint assets in the project and save the results."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FWebUIRuntimeEditorModule::HandleSyncAllBlueprints)));
	}

	void HandleSyncAllBlueprints()
	{
		const int32 SyncedCount = UWebUIPresentationSyncLibrary::SyncAllBlueprintWebUIPresentations();
		FNotificationInfo Info(FText::Format(
			LOCTEXT("SyncAllBlueprintsDone", "Synced WebUI presentations for {0} Blueprint(s)."),
			FText::AsNumber(SyncedCount)));
		Info.ExpireDuration = 3.0f;
		Info.bUseSuccessFailIcons = true;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
};

IMPLEMENT_MODULE(FWebUIRuntimeEditorModule, WebUIRuntimeEditor)

#undef LOCTEXT_NAMESPACE

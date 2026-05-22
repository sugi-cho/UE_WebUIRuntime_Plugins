#include "WebUIRuntime.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogWebUIRuntime);

void FWebUIRuntimeModule::StartupModule()
{
	UE_LOG(LogWebUIRuntime, Log, TEXT("WebUIRuntime module started"));
}

void FWebUIRuntimeModule::ShutdownModule()
{
	UE_LOG(LogWebUIRuntime, Log, TEXT("WebUIRuntime module stopped"));
}

IMPLEMENT_MODULE(FWebUIRuntimeModule, WebUIRuntime)


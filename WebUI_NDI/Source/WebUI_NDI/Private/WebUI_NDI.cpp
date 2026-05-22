#include "WebUI_NDI.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogWebUINDI);

void FWebUINDIModule::StartupModule()
{
	UE_LOG(LogWebUINDI, Log, TEXT("WebUI_NDI module started"));
}

void FWebUINDIModule::ShutdownModule()
{
	UE_LOG(LogWebUINDI, Log, TEXT("WebUI_NDI module stopped"));
}

IMPLEMENT_MODULE(FWebUINDIModule, WebUI_NDI)


#include "WebUIRuntimeSubsystem.h"

#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "HttpPath.h"
#include "HttpServerModule.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "IHttpRouter.h"
#include "Json.h"
#include "Math/Color.h"
#include "Serialization/JsonSerializer.h"
#include "WebUIComponentBase.h"
#include "WebUIHostComponent.h"
#include "WebUIRuntimeSettings.h"
#include "WebUIRuntime.h"

namespace
{
	const FString JsonContentType = TEXT("application/json; charset=utf-8");
	const FString HtmlContentType = TEXT("text/html; charset=utf-8");

	const TCHAR* WebUIHtml = TEXT(R"HTML(
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>UE WebUI Runtime</title>
<style>
body{font-family:system-ui,sans-serif;margin:24px;background:#101216;color:#e9edf2}
h1{margin:0 0 16px}
.tabs{display:flex;flex-wrap:wrap;gap:8px;margin:12px 0 20px}
.tab{border:1px solid #3d4654;background:#0d1015;color:#e9edf2;padding:8px 12px;border-radius:999px;cursor:pointer}
.tab.active{background:#233247;border-color:#5b7898}
.panel{display:none;border:1px solid #2d3440;border-radius:8px;padding:16px;margin:16px 0;background:#171b22}
.panel.active{display:block}
.host-meta{opacity:.78;margin-top:4px}
.component{border-top:1px solid #2d3440;padding-top:12px;margin-top:12px}
label{display:grid;grid-template-columns:180px minmax(180px,1fr);gap:12px;align-items:center;margin:10px 0}
input,button{font:inherit;padding:8px;border-radius:6px;border:1px solid #3d4654;background:#0d1015;color:#e9edf2}
button{cursor:pointer;background:#233247}
.row{margin:8px 0}
.empty{opacity:.75;padding:16px 0}
</style>
</head>
<body>
<h1>UE WebUI Runtime</h1>
<main id="app"></main>
<script>
const app=document.getElementById('app');
async function api(path,body){const r=await fetch(path,{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify(body)});return r.json();}
function makeInput(p){
 const el=document.createElement('input');
 el.value=typeof p.value==='object'?JSON.stringify(p.value):p.value ?? '';
 if(p.type==='bool'){el.type='checkbox';el.checked=!!p.value;}
 if(p.type==='float'||p.type==='int32'){el.type='number'; if(p.type==='float') el.step='0.01';}
 return el;
}
function renderComponent(host,c){
 const cs=document.createElement('section');
 cs.className='component';
 cs.innerHTML=`<h3>${c.name}</h3>`;
 for(const p of c.properties){
  const label=document.createElement('label');
  const input=makeInput(p);
  input.onchange=async()=>{
   let value=input.type==='checkbox'?input.checked:input.value;
   if(p.type==='float') value=parseFloat(value);
   if(p.type==='int32') value=parseInt(value,10);
   if(['vector','rotator','linearColor'].includes(p.type)) value=JSON.parse(input.value);
   await api('/api/webui/property',{webUIId:host.webUIId,componentId:c.componentId,propertyName:p.name,value});
  };
  label.append(p.name,input);
  cs.append(label);
 }
 for(const b of c.buttons){
 const row=document.createElement('div');
  row.className='row';
  const btn=document.createElement('button');
  btn.textContent=b.id;
  btn.onclick=async()=>{await api('/api/webui/button',{webUIId:host.webUIId,componentId:c.componentId,buttonId:b.id}); await load();};
  row.append(btn);
  cs.append(row);
 }
 return cs;
}
async function load(){
 const schema=await (await fetch('/api/webui/schema')).json();
 app.innerHTML='';
 const hosts=[...(schema.hosts||[])].sort((a,b)=>String(a.webUIId).localeCompare(String(b.webUIId)));
 if(!hosts.length){
  const empty=document.createElement('div');
  empty.className='empty';
  empty.textContent='No WebUIHostComponent was found.';
  app.append(empty);
  return;
 }
 const tabs=document.createElement('div');
 tabs.className='tabs';
 const panels=document.createElement('div');
 let activeTabId='';
 const setActive=(webUIId)=>{
  activeTabId=webUIId;
  for(const tab of tabs.querySelectorAll('[data-webui-id]')){
   tab.classList.toggle('active',tab.dataset.webuiId===webUIId);
  }
  for(const panel of panels.querySelectorAll('[data-webui-panel]')){
   panel.classList.toggle('active',panel.dataset.webuiPanel===webUIId);
  }
 };
 for(const host of hosts){
  const tab=document.createElement('button');
  tab.className='tab';
  tab.type='button';
  tab.dataset.webuiId=host.webUIId;
  tab.textContent=host.webUIId;
  tab.onclick=()=>setActive(host.webUIId);
  tabs.append(tab);

  const panel=document.createElement('section');
  panel.className='panel';
  panel.dataset.webuiPanel=host.webUIId;
  panel.innerHTML=`<h2>${host.webUIId}</h2><div class="host-meta">${host.actorName}</div>`;
  for(const c of host.components||[]){
   panel.append(renderComponent(host,c));
  }
  panels.append(panel);
 }
 app.append(tabs,panels);
 setActive(hosts[0].webUIId);
}
load();
</script>
</body>
</html>
)HTML");

	TSharedRef<FJsonObject> MakeErrorObject(const FString& Error)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetBoolField(TEXT("ok"), false);
		Object->SetStringField(TEXT("error"), Error);
		return Object;
	}
}

bool UWebUIRuntimeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

bool UWebUIRuntimeSubsystem::StartServerFromSettings()
{
	return StartServer(GetDefault<UWebUIRuntimeSettings>()->Port);
}

void UWebUIRuntimeSubsystem::Deinitialize()
{
	StopServer();
	Hosts.Reset();
	Super::Deinitialize();
}

void UWebUIRuntimeSubsystem::RegisterHost(UWebUIHostComponent* Host)
{
	if (IsValid(Host))
	{
		Hosts.AddUnique(Host);
	}
}

void UWebUIRuntimeSubsystem::UnregisterHost(UWebUIHostComponent* Host)
{
	Hosts.Remove(Host);
}

bool UWebUIRuntimeSubsystem::StartServer(int32 Port)
{
	if (Router.IsValid() && ActivePort == Port)
	{
		return true;
	}

	StopServer();

	FHttpServerModule& HttpServerModule = FHttpServerModule::Get();
	Router = HttpServerModule.GetHttpRouter(static_cast<uint32>(Port), true);
	if (!Router.IsValid())
	{
		UE_LOG(LogWebUIRuntime, Error, TEXT("Failed to bind WebUI HTTP server on port %d"), Port);
		return false;
	}

	RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/webui")), EHttpServerRequestVerbs::VERB_GET, FHttpRequestHandler::CreateUObject(this, &UWebUIRuntimeSubsystem::HandleWebUI)));
	RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/api/webui/schema")), EHttpServerRequestVerbs::VERB_GET, FHttpRequestHandler::CreateUObject(this, &UWebUIRuntimeSubsystem::HandleSchema)));
	RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/api/webui/property")), EHttpServerRequestVerbs::VERB_POST, FHttpRequestHandler::CreateUObject(this, &UWebUIRuntimeSubsystem::HandleProperty)));
	RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/api/webui/button")), EHttpServerRequestVerbs::VERB_POST, FHttpRequestHandler::CreateUObject(this, &UWebUIRuntimeSubsystem::HandleButton)));

	HttpServerModule.StartAllListeners();
	ActivePort = Port;
	UE_LOG(LogWebUIRuntime, Log, TEXT("WebUI HTTP server listening on http://localhost:%d/webui"), ActivePort);
	return true;
}

void UWebUIRuntimeSubsystem::StopServer()
{
	if (Router.IsValid())
	{
		for (const FHttpRouteHandle& Handle : RouteHandles)
		{
			Router->UnbindRoute(Handle);
		}
	}

	RouteHandles.Reset();
	Router.Reset();
	ActivePort = 0;
	FHttpServerModule::Get().StopAllListeners();
}

bool UWebUIRuntimeSubsystem::IsServerRunning() const
{
	return Router.IsValid() && ActivePort > 0;
}

int32 UWebUIRuntimeSubsystem::GetServerPort() const
{
	return ActivePort;
}

bool UWebUIRuntimeSubsystem::HandleWebUI(const FHttpServerRequest& Request, const TFunction<void(TUniquePtr<FHttpServerResponse>&&)>& OnComplete)
{
	OnComplete(FHttpServerResponse::Create(FString(WebUIHtml), HtmlContentType));
	return true;
}

bool UWebUIRuntimeSubsystem::HandleSchema(const FHttpServerRequest& Request, const TFunction<void(TUniquePtr<FHttpServerResponse>&&)>& OnComplete)
{
	OnComplete(MakeJsonResponse(BuildSchema()));
	return true;
}

bool UWebUIRuntimeSubsystem::HandleProperty(const FHttpServerRequest& Request, const TFunction<void(TUniquePtr<FHttpServerResponse>&&)>& OnComplete)
{
	FString Error;
	TSharedPtr<FJsonObject> Body = ParseRequestJson(Request, Error);
	if (!Body.IsValid())
	{
		OnComplete(MakeJsonResponse(MakeErrorObject(Error)));
		return true;
	}

	const FString WebUIId = Body->GetStringField(TEXT("webUIId"));
	const FString ComponentId = Body->GetStringField(TEXT("componentId"));
	const FString PropertyName = Body->GetStringField(TEXT("propertyName"));
	const TSharedPtr<FJsonValue> Value = Body->TryGetField(TEXT("value"));
	UActorComponent* Component = FindComponent(WebUIId, ComponentId);

	if (!Component || !Value.IsValid() || !SetPropertyFromJson(Component, PropertyName, Value, Error))
	{
		OnComplete(MakeJsonResponse(MakeErrorObject(Error.IsEmpty() ? TEXT("Failed to set property") : Error)));
		return true;
	}

	TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("ok"), true);
	OnComplete(MakeJsonResponse(Response));
	return true;
}

bool UWebUIRuntimeSubsystem::HandleButton(const FHttpServerRequest& Request, const TFunction<void(TUniquePtr<FHttpServerResponse>&&)>& OnComplete)
{
	FString Error;
	TSharedPtr<FJsonObject> Body = ParseRequestJson(Request, Error);
	if (!Body.IsValid())
	{
		OnComplete(MakeJsonResponse(MakeErrorObject(Error)));
		return true;
	}

	const FString WebUIId = Body->GetStringField(TEXT("webUIId"));
	const FString ComponentId = Body->GetStringField(TEXT("componentId"));
	const FName ButtonId(*Body->GetStringField(TEXT("buttonId")));
	UWebUIComponentBase* Component = Cast<UWebUIComponentBase>(FindComponent(WebUIId, ComponentId));
	if (!Component || ButtonId.IsNone() || !Component->GetWebUIButtons().Contains(ButtonId))
	{
		OnComplete(MakeJsonResponse(MakeErrorObject(TEXT("Button not found"))));
		return true;
	}

	Component->NotifyWebUIButtonClicked(ButtonId);

	TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("ok"), true);
	OnComplete(MakeJsonResponse(Response));
	return true;
}

TSharedRef<FJsonObject> UWebUIRuntimeSubsystem::BuildSchema() const
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> HostValues;

	for (UWebUIHostComponent* Host : Hosts)
	{
		if (!IsValid(Host) || !IsValid(Host->GetOwner()))
		{
			continue;
		}

		TSharedRef<FJsonObject> HostObject = MakeShared<FJsonObject>();
		HostObject->SetStringField(TEXT("webUIId"), Host->GetWebUIId());
		HostObject->SetStringField(TEXT("actorName"), Host->GetOwner()->GetName());

		TArray<TSharedPtr<FJsonValue>> ComponentValues;
		TArray<UActorComponent*> Components;
		Host->GetOwner()->GetComponents(Components);
		for (UActorComponent* Component : Components)
		{
			if (TSharedPtr<FJsonObject> ComponentObject = BuildComponentSchema(Component))
			{
				ComponentValues.Add(MakeShared<FJsonValueObject>(ComponentObject));
			}
		}

		HostObject->SetArrayField(TEXT("components"), ComponentValues);
		HostValues.Add(MakeShared<FJsonValueObject>(HostObject));
	}

	Root->SetArrayField(TEXT("hosts"), HostValues);
	return Root;
}

TSharedPtr<FJsonObject> UWebUIRuntimeSubsystem::BuildComponentSchema(UActorComponent* Component) const
{
	if (!IsValid(Component))
	{
		return nullptr;
	}

	TArray<TSharedPtr<FJsonValue>> PropertyValues;
	for (TFieldIterator<FProperty> It(Component->GetClass()); It; ++It)
	{
		FProperty* Property = *It;
		if (!IsWebUIProperty(Property))
		{
			continue;
		}

		TSharedRef<FJsonObject> PropertyObject = MakeShared<FJsonObject>();
		PropertyObject->SetStringField(TEXT("name"), Property->GetName());
		PropertyObject->SetStringField(TEXT("type"), GetPropertyWebUIType(Property));
		PropertyObject->SetField(TEXT("value"), PropertyToJsonValue(Property, Component));
		PropertyValues.Add(MakeShared<FJsonValueObject>(PropertyObject));
	}

	TArray<TSharedPtr<FJsonValue>> ButtonValues;
	if (const UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Component))
	{
		for (const FName Button : WebUIComponent->GetWebUIButtons())
		{
			TSharedRef<FJsonObject> ButtonObject = MakeShared<FJsonObject>();
			ButtonObject->SetStringField(TEXT("id"), Button.ToString());
			ButtonValues.Add(MakeShared<FJsonValueObject>(ButtonObject));
		}
	}

	if (PropertyValues.IsEmpty() && ButtonValues.IsEmpty())
	{
		return nullptr;
	}

	TSharedRef<FJsonObject> ComponentObject = MakeShared<FJsonObject>();
	ComponentObject->SetStringField(TEXT("componentId"), Component->GetName());
	ComponentObject->SetStringField(TEXT("name"), Component->GetName());
	ComponentObject->SetStringField(TEXT("className"), Component->GetClass()->GetName());
	ComponentObject->SetArrayField(TEXT("properties"), PropertyValues);
	ComponentObject->SetArrayField(TEXT("buttons"), ButtonValues);
	return ComponentObject;
}

UActorComponent* UWebUIRuntimeSubsystem::FindComponent(const FString& WebUIId, const FString& ComponentId) const
{
	for (UWebUIHostComponent* Host : Hosts)
	{
		if (!IsValid(Host) || Host->GetWebUIId() != WebUIId || !IsValid(Host->GetOwner()))
		{
			continue;
		}

		TArray<UActorComponent*> Components;
		Host->GetOwner()->GetComponents(Components);
		for (UActorComponent* Component : Components)
		{
			if (IsValid(Component) && Component->GetName() == ComponentId)
			{
				return Component;
			}
		}
	}
	return nullptr;
}

bool UWebUIRuntimeSubsystem::SetPropertyFromJson(UActorComponent* Component, const FString& PropertyName, const TSharedPtr<FJsonValue>& Value, FString& OutError)
{
	FProperty* Property = FindFProperty<FProperty>(Component->GetClass(), *PropertyName);
	if (!Property || !IsWebUIProperty(Property))
	{
		OutError = TEXT("Property not found or not exposed to WebUI");
		return false;
	}

	void* PropertyPtr = Property->ContainerPtrToValuePtr<void>(Component);
	if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
	{
		BoolProperty->SetPropertyValue(PropertyPtr, Value->AsBool());
		if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Component))
		{
			WebUIComponent->NotifyWebUIBoolChanged(*PropertyName, Value->AsBool());
		}
	}
	else if (FIntProperty* IntProperty = CastField<FIntProperty>(Property))
	{
		IntProperty->SetPropertyValue(PropertyPtr, static_cast<int32>(Value->AsNumber()));
	}
	else if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
	{
		const double Number = Value->AsNumber();
		FloatProperty->SetPropertyValue(PropertyPtr, static_cast<float>(Number));
		if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Component))
		{
			WebUIComponent->NotifyWebUIFloatChanged(*PropertyName, Number);
		}
	}
	else if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
	{
		const double Number = Value->AsNumber();
		DoubleProperty->SetPropertyValue(PropertyPtr, Number);
		if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Component))
		{
			WebUIComponent->NotifyWebUIFloatChanged(*PropertyName, Number);
		}
	}
	else if (FStrProperty* StringProperty = CastField<FStrProperty>(Property))
	{
		const FString StringValue = Value->AsString();
		StringProperty->SetPropertyValue(PropertyPtr, StringValue);
		if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Component))
		{
			WebUIComponent->NotifyWebUIStringChanged(*PropertyName, StringValue);
		}
	}
	else if (FNameProperty* NameProperty = CastField<FNameProperty>(Property))
	{
		const FString StringValue = Value->AsString();
		NameProperty->SetPropertyValue(PropertyPtr, FName(*StringValue));
		if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Component))
		{
			WebUIComponent->NotifyWebUIStringChanged(*PropertyName, StringValue);
		}
	}
	else if (FTextProperty* TextProperty = CastField<FTextProperty>(Property))
	{
		const FString StringValue = Value->AsString();
		TextProperty->SetPropertyValue(PropertyPtr, FText::FromString(StringValue));
		if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Component))
		{
			WebUIComponent->NotifyWebUIStringChanged(*PropertyName, StringValue);
		}
	}
	else if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		const UEnum* Enum = EnumProperty->GetEnum();
		const int64 EnumValue = Enum ? Enum->GetValueByNameString(Value->AsString()) : INDEX_NONE;
		if (EnumValue == INDEX_NONE)
		{
			OutError = TEXT("Invalid enum value");
			return false;
		}
		EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(PropertyPtr, EnumValue);
	}
	else if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
	{
		if (UEnum* Enum = ByteProperty->Enum)
		{
			const int64 EnumValue = Enum->GetValueByNameString(Value->AsString());
			if (EnumValue == INDEX_NONE)
			{
				OutError = TEXT("Invalid enum value");
				return false;
			}
			ByteProperty->SetPropertyValue(PropertyPtr, static_cast<uint8>(EnumValue));
		}
		else
		{
			ByteProperty->SetPropertyValue(PropertyPtr, static_cast<uint8>(Value->AsNumber()));
		}
	}
	else if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		TSharedPtr<FJsonObject> ObjectValue = Value->AsObject();
		if (!ObjectValue.IsValid())
		{
			OutError = TEXT("Struct value must be an object");
			return false;
		}

		if (StructProperty->Struct == TBaseStructure<FVector>::Get())
		{
			FVector Vector(ObjectValue->GetNumberField(TEXT("x")), ObjectValue->GetNumberField(TEXT("y")), ObjectValue->GetNumberField(TEXT("z")));
			*static_cast<FVector*>(PropertyPtr) = Vector;
			if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Component))
			{
				WebUIComponent->NotifyWebUIVectorChanged(*PropertyName, Vector);
			}
		}
		else if (StructProperty->Struct == TBaseStructure<FRotator>::Get())
		{
			FRotator Rotator(ObjectValue->GetNumberField(TEXT("pitch")), ObjectValue->GetNumberField(TEXT("yaw")), ObjectValue->GetNumberField(TEXT("roll")));
			*static_cast<FRotator*>(PropertyPtr) = Rotator;
			if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Component))
			{
				WebUIComponent->NotifyWebUIRotatorChanged(*PropertyName, Rotator);
			}
		}
		else if (StructProperty->Struct == TBaseStructure<FLinearColor>::Get())
		{
			FLinearColor Color(ObjectValue->GetNumberField(TEXT("r")), ObjectValue->GetNumberField(TEXT("g")), ObjectValue->GetNumberField(TEXT("b")), ObjectValue->GetNumberField(TEXT("a")));
			*static_cast<FLinearColor*>(PropertyPtr) = Color;
			if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Component))
			{
				WebUIComponent->NotifyWebUIColorChanged(*PropertyName, Color);
			}
		}
		else
		{
			OutError = TEXT("Unsupported struct type");
			return false;
		}
	}
	else
	{
		OutError = TEXT("Unsupported property type");
		return false;
	}

	if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Component))
	{
		WebUIComponent->NotifyWebUIPropertyChanged(*PropertyName);
	}

	return true;
}

TSharedPtr<FJsonValue> UWebUIRuntimeSubsystem::PropertyToJsonValue(FProperty* Property, const void* Container) const
{
	const void* PropertyPtr = Property->ContainerPtrToValuePtr<void>(Container);
	if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
	{
		return MakeShared<FJsonValueBoolean>(BoolProperty->GetPropertyValue(PropertyPtr));
	}
	if (const FIntProperty* IntProperty = CastField<FIntProperty>(Property))
	{
		return MakeShared<FJsonValueNumber>(IntProperty->GetPropertyValue(PropertyPtr));
	}
	if (const FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
	{
		return MakeShared<FJsonValueNumber>(FloatProperty->GetPropertyValue(PropertyPtr));
	}
	if (const FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
	{
		return MakeShared<FJsonValueNumber>(DoubleProperty->GetPropertyValue(PropertyPtr));
	}
	if (const FStrProperty* StringProperty = CastField<FStrProperty>(Property))
	{
		return MakeShared<FJsonValueString>(StringProperty->GetPropertyValue(PropertyPtr));
	}
	if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
	{
		return MakeShared<FJsonValueString>(NameProperty->GetPropertyValue(PropertyPtr).ToString());
	}
	if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
	{
		return MakeShared<FJsonValueString>(TextProperty->GetPropertyValue(PropertyPtr).ToString());
	}
	if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		const int64 Value = EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(PropertyPtr);
		return MakeShared<FJsonValueString>(EnumProperty->GetEnum()->GetNameStringByValue(Value));
	}
	if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
	{
		if (ByteProperty->Enum)
		{
			return MakeShared<FJsonValueString>(ByteProperty->Enum->GetNameStringByValue(ByteProperty->GetPropertyValue(PropertyPtr)));
		}
		return MakeShared<FJsonValueNumber>(ByteProperty->GetPropertyValue(PropertyPtr));
	}
	if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		if (StructProperty->Struct == TBaseStructure<FVector>::Get())
		{
			const FVector& Value = *static_cast<const FVector*>(PropertyPtr);
			Object->SetNumberField(TEXT("x"), Value.X);
			Object->SetNumberField(TEXT("y"), Value.Y);
			Object->SetNumberField(TEXT("z"), Value.Z);
			return MakeShared<FJsonValueObject>(Object);
		}
		if (StructProperty->Struct == TBaseStructure<FRotator>::Get())
		{
			const FRotator& Value = *static_cast<const FRotator*>(PropertyPtr);
			Object->SetNumberField(TEXT("pitch"), Value.Pitch);
			Object->SetNumberField(TEXT("yaw"), Value.Yaw);
			Object->SetNumberField(TEXT("roll"), Value.Roll);
			return MakeShared<FJsonValueObject>(Object);
		}
		if (StructProperty->Struct == TBaseStructure<FLinearColor>::Get())
		{
			const FLinearColor& Value = *static_cast<const FLinearColor*>(PropertyPtr);
			Object->SetNumberField(TEXT("r"), Value.R);
			Object->SetNumberField(TEXT("g"), Value.G);
			Object->SetNumberField(TEXT("b"), Value.B);
			Object->SetNumberField(TEXT("a"), Value.A);
			return MakeShared<FJsonValueObject>(Object);
		}
	}
	return MakeShared<FJsonValueNull>();
}

FString UWebUIRuntimeSubsystem::GetPropertyWebUIType(FProperty* Property) const
{
	if (Property->IsA<FBoolProperty>()) return TEXT("bool");
	if (Property->IsA<FIntProperty>()) return TEXT("int32");
	if (Property->IsA<FFloatProperty>() || Property->IsA<FDoubleProperty>()) return TEXT("float");
	if (Property->IsA<FStrProperty>() || Property->IsA<FNameProperty>() || Property->IsA<FTextProperty>()) return TEXT("string");
	if (Property->IsA<FEnumProperty>()) return TEXT("enum");
	if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
	{
		return ByteProperty->Enum ? TEXT("enum") : TEXT("int32");
	}
	if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		if (StructProperty->Struct == TBaseStructure<FVector>::Get()) return TEXT("vector");
		if (StructProperty->Struct == TBaseStructure<FRotator>::Get()) return TEXT("rotator");
		if (StructProperty->Struct == TBaseStructure<FLinearColor>::Get()) return TEXT("linearColor");
	}
	return TEXT("unsupported");
}

bool UWebUIRuntimeSubsystem::IsWebUIProperty(FProperty* Property) const
{
	return Property
		&& GetPropertyWebUIType(Property) != TEXT("unsupported")
		&& (Property->HasMetaData(TEXT("WebUI")) || Property->GetMetaData(TEXT("Category")) == TEXT("WebUI"));
}

TUniquePtr<FHttpServerResponse> UWebUIRuntimeSubsystem::MakeJsonResponse(const TSharedRef<FJsonObject>& Object) const
{
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Object, Writer);
	return FHttpServerResponse::Create(JsonString, JsonContentType);
}

TSharedPtr<FJsonObject> UWebUIRuntimeSubsystem::ParseRequestJson(const FHttpServerRequest& Request, FString& OutError) const
{
	FString BodyString;
	if (Request.Body.Num() > 0)
	{
		FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Request.Body.GetData()), Request.Body.Num());
		BodyString = FString(Converted.Length(), Converted.Get());
	}

	TSharedPtr<FJsonObject> BodyObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(BodyString);
	if (!FJsonSerializer::Deserialize(Reader, BodyObject) || !BodyObject.IsValid())
	{
		OutError = TEXT("Invalid JSON body");
		return nullptr;
	}
	return BodyObject;
}

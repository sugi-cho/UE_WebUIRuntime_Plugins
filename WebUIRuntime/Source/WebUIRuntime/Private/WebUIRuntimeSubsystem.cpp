#include "WebUIRuntimeSubsystem.h"

#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GeneralProjectSettings.h"
#include "Misc/PackageName.h"
#include "HttpPath.h"
#include "HttpServerModule.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "IHttpRouter.h"
#include "Json.h"
#include "Math/Color.h"
#include "Misc/ConfigCacheIni.h"
#include "Serialization/JsonSerializer.h"
#if WITH_EDITOR
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_EditablePinBase.h"
#include "Engine/Blueprint.h"
#endif
#include "WebUIComponentBase.h"
#include "WebUIHostActor.h"
#include "WebUIHostComponent.h"
#include "WebUIRuntimeSaveGame.h"
#include "WebUIRuntimeSettings.h"
#include "WebUIRuntime.h"

namespace
{
	const FString JsonContentType = TEXT("application/json; charset=utf-8");
	const FString HtmlContentType = TEXT("text/html; charset=utf-8");

	FString BuildWebUIHtml()
	{
		FString Html;
		Html.Reserve(65536);
		Html += TEXT(R"HTML(
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>__WEBUI_TITLE__</title>
<style>
:root{
 --page-bg: rgba(16,18,22,0.98);
 --shell-bg: #101216;
 --panel-bg: #171b22;
 --panel-bg-embed: rgba(23,27,34,0.72);
 --border: #2d3440;
 --border-strong: #5b7898;
 --button-bg: #233247;
 --button-bg-pressed: #4b6f95;
 --text: #e9edf2;
 --muted: #cfd6df;
 --page-padding: 24px;
 --content-gap: 16px;
 --panel-radius: 8px;
}
 html,body{margin:0;height:100%;background:transparent;overflow:hidden}
 body{height:100%;font-family:system-ui,sans-serif;color:var(--text)}
 body.theme-light{--page-bg:rgba(248,250,252,0.98);--shell-bg:#f8fafc;--panel-bg:#ffffff;--panel-bg-embed:rgba(255,255,255,0.74);--border:#d8e0ea;--border-strong:#7aa3d8;--button-bg:#e6edf6;--button-bg-pressed:#bfd6ef;--text:#101216;--muted:#4b5563}
 body.theme-dark{--page-bg:rgba(16,18,22,0.98);--shell-bg:#101216;--panel-bg:#171b22;--panel-bg-embed:rgba(23,27,34,0.72);--border:#2d3440;--border-strong:#5b7898;--button-bg:#233247;--button-bg-pressed:#4b6f95;--text:#e9edf2;--muted:#cfd6df}
 body.embed{overflow:hidden}
 body.compact{--page-padding:12px;--content-gap:10px}
 .page-shell{height:100vh;box-sizing:border-box;padding:var(--page-padding);background:var(--shell-bg);display:flex;flex-direction:column;min-height:0}
 body.embed .page-shell{height:100vh;padding:0;background:transparent}
 .page-header{display:flex;justify-content:space-between;align-items:flex-start;gap:12px;margin:0 0 var(--content-gap)}
 .page-title{margin:0}
 .external-link{display:inline-flex;align-items:center;gap:8px;text-decoration:none;color:var(--muted);border:1px solid var(--border);background:rgba(255,255,255,.02);padding:8px 12px;border-radius:999px}
 .external-link:hover{color:var(--text);border-color:var(--border-strong)}
 body.embed .page-header{display:none}
 .app-shell{display:flex;flex-direction:column;flex:1;min-height:0}
 .tabs{display:flex;flex-wrap:wrap;gap:6px;align-items:flex-end;padding:0 0 0 8px;margin:12px 0 0;border-bottom:1px solid var(--border);flex:0 0 auto}
 .tab{border:1px solid var(--border);border-bottom:none;background:var(--button-bg);color:var(--muted);padding:10px 16px 11px;border-radius:10px 10px 0 0;cursor:pointer;position:relative;top:1px}
 .tab.active{background:var(--panel-bg);color:var(--text);border-color:var(--border-strong);border-bottom:1px solid var(--panel-bg);z-index:1}
 body.embed .tab.active{border-bottom-color:transparent}
 .tab:not(.active){opacity:.9}
 .panel-scroll{flex:1;min-height:0;overflow:auto;padding:0 8px 0 0;scrollbar-width:thin;scrollbar-color:transparent transparent;overscroll-behavior:contain}
 body.scrolling .panel-scroll{scrollbar-color:rgba(255,255,255,.36) transparent}
 body.theme-light.scrolling .panel-scroll{scrollbar-color:rgba(16,18,22,.34) transparent}
 .panel-scroll::-webkit-scrollbar{width:10px;height:10px;background:transparent}
 .panel-scroll::-webkit-scrollbar-track{background:transparent}
 .panel-scroll::-webkit-scrollbar-thumb{background:transparent;border-radius:999px;border:2px solid transparent;background-clip:padding-box;transition:background-color .15s ease}
 body.scrolling .panel-scroll::-webkit-scrollbar-thumb{background:rgba(255,255,255,.32)}
 body.theme-light.scrolling .panel-scroll::-webkit-scrollbar-thumb{background:rgba(16,18,22,.28)}
 .panel{display:none;border:1px solid var(--border);border-top:none;border-radius:0 var(--panel-radius) var(--panel-radius) var(--panel-radius);padding:16px;margin:0 0 var(--content-gap);background:var(--panel-bg)}
 body.embed .panel{background:var(--panel-bg-embed);backdrop-filter:saturate(120%) blur(4px)}
 .panel.active{display:block}
.host-meta{opacity:.78;margin-top:4px;white-space:pre-wrap}
.component{border-top:1px solid var(--border);padding-top:12px;margin-top:12px}
.property-row{display:grid;grid-template-columns:180px minmax(0,1fr);column-gap:12px;row-gap:6px;align-items:center;margin:10px 0}
.property-name{padding-top:2px;min-width:0}
.property-control{display:flex;flex-wrap:wrap;gap:8px;align-items:center;justify-content:flex-start;min-width:0;width:100%}
input,button,select{font:inherit;padding:8px;border-radius:6px;border:1px solid var(--border);background:#0d1015;color:var(--text);box-sizing:border-box}
body.theme-light input,body.theme-light button,body.theme-light select{background:#ffffff;color:var(--text)}
input[type="text"],input[type="number"],select{width:100%;min-width:0}
input[type="range"]{flex:1 1 240px;min-width:180px;padding:8px 0}
.numeric-field{width:120px}
.option-picker{position:relative;flex:1 1 260px;min-width:180px;max-width:100%}
.option-picker-button{width:100%;display:flex;justify-content:space-between;gap:8px;text-align:left}
.option-picker-button::after{content:'v';opacity:.65}
.option-picker-menu{position:absolute;z-index:20;left:0;right:0;top:calc(100% + 4px);max-height:240px;overflow:auto;border:1px solid var(--border-strong);border-radius:6px;background:#0d1015;box-shadow:0 12px 28px rgba(0,0,0,.35)}
.option-picker-menu[hidden]{display:none}
.option-picker-option{display:block;width:100%;border:0;border-radius:0;background:transparent;text-align:left}
.option-picker-option:hover,.option-picker-option.selected{background:var(--button-bg-pressed)}
body.theme-light .option-picker-menu{background:#ffffff}
.vector-group,.rotator-group,.color-group,.linear-color-group{display:grid;width:100%;align-items:center}
.vector-group,.rotator-group{grid-template-columns:repeat(3,minmax(0,1fr));gap:10px}
.vector-group input,.rotator-group input{width:100%;min-width:0}
.color-group{grid-template-columns:1fr}
.linear-color-group{grid-template-columns:140px minmax(0,1fr)}
.color-swatch,.linear-color-swatch{padding:8px;border-radius:6px;background:#0d1015}
body.theme-light .color-swatch,body.theme-light .linear-color-swatch{background:#ffffff}
.hex-field{font-family:ui-monospace,Consolas,monospace}
.alpha-field{width:90px}
.checkbox-field{margin-left:0}
.range-field{flex:1 1 240px}
.button-list{display:flex;flex-wrap:wrap;gap:8px;margin:8px 0}
button{cursor:pointer;background:var(--button-bg)}
button.webui-button{min-width:120px;transition:transform .08s ease, background-color .12s ease, border-color .12s ease, opacity .12s ease}
button.webui-button.pressed{background:var(--button-bg-pressed);border-color:#8ab1dc;transform:translateY(1px)}
button.webui-button:disabled{opacity:.72;cursor:default}
.row{margin:8px 0}
.empty{opacity:.75;padding:16px 0}
 body.compact .tabs{margin-top:0}
 body.compact .panel{padding:12px}
body.compact .property-row{margin:8px 0;grid-template-columns:150px minmax(0,1fr)}
.linear-color-group{grid-template-columns:minmax(0,180px) minmax(0,1fr)}
.linear-color-group>input[type="color"]{width:44px;min-width:44px;height:39px;padding:4px}
body:not(.embed) .linear-color-group{grid-template-columns:44px minmax(0,180px) minmax(0,1fr)}
body.embed .linear-color-group{grid-template-columns:minmax(0,180px) minmax(0,1fr)}
.color-picker-group{display:grid;grid-template-columns:44px minmax(0,1fr);gap:8px;align-items:center;min-width:0}
.color-picker-group input[type="color"]{width:44px;min-width:44px;height:39px;padding:4px}
.color-picker-group .hex-field{width:100%;min-width:0}
body.embed .color-picker-group{grid-template-columns:minmax(0,1fr)}
</style>
)HTML");
		Html += TEXT(R"HTML(
</head>
<body class="theme-dark">
<div class="page-shell">
<header class="page-header">
<h1 class="page-title">__WEBUI_TITLE__</h1>
<a id="externalLink" class="external-link" href="#" target="_blank" rel="noopener noreferrer">Open external browser</a>
</header>
<main id="app" class="app-shell">
<div class="tabs"></div>
<div class="panel-scroll"></div>
</main>
</div>
<script>
const app=document.getElementById('app');
const tabsHost=app.querySelector('.tabs');
const panelScrollHost=app.querySelector('.panel-scroll');
const url=new URL(window.location.href);
const params=url.searchParams;
const externalLink=document.getElementById('externalLink');
let currentWebUIId='';
const initialWebUIId=params.get('webuiId') || '';
const isEmbed=params.get('embed')==='1';
const isCompact=params.get('compact')==='1';
const theme=(params.get('theme')||'dark').toLowerCase();
let scrollContainer=null;
let scrollStateTimer=null;
document.body.classList.toggle('embed',isEmbed);
document.body.classList.toggle('compact',isCompact);
document.body.classList.toggle('theme-light',theme==='light');
document.body.classList.toggle('theme-dark',theme!=='light');
const setScrollingState=(isScrolling)=>{
 document.body.classList.toggle('scrolling',isScrolling);
 if(scrollStateTimer){
  clearTimeout(scrollStateTimer);
  scrollStateTimer=null;
 }
 if(isScrolling){
  scrollStateTimer=setTimeout(()=>document.body.classList.remove('scrolling'),160);
 }
};
if(externalLink){
 const externalUrl=new URL(window.location.href);
 externalUrl.searchParams.delete('embed');
 externalLink.href=externalUrl.toString();
}
async function api(path,body){const r=await fetch(path,{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify(body)});return r.json();}
function numberOrNull(v){return typeof v==='number'&&Number.isFinite(v)?v:null;}
function resolveNumericBounds(p){
 const bounds=[
  ['uiMin','uiMax'],
  ['sliderMin','sliderMax'],
  ['clampMin','clampMax']
 ];
 for(const [minKey,maxKey] of bounds){
  const min=numberOrNull(p[minKey]);
  const max=numberOrNull(p[maxKey]);
  if(min!==null&&max!==null){return {min,max};}
 }
 return {min:null,max:null};
}
function resolveNumericStep(p){
 const step=numberOrNull(p.step);
 return step!==null?step:(p.type==='int32'?1:0.01);
}
function createRow(p){
 const row=document.createElement('div');
 row.className='property-row';
 const name=document.createElement('div');
 name.className='property-name';
 name.textContent=p.name;
 const control=document.createElement('div');
 control.className='property-control';
 row.append(name,control);
 return {label:row,control};
}
function renderOptionPicker(currentValue,options,onCommit){
 const picker=document.createElement('div');
 picker.className='option-picker';
 const button=document.createElement('button');
 button.type='button';
 button.className='option-picker-button';
 const menu=document.createElement('div');
 menu.className='option-picker-menu';
 menu.hidden=true;
 const normalizedOptions=options||[];
 let selectedValue=String(currentValue ?? '');
 let isChoosing=false;
 const labelFor=(value)=>{
  const match=normalizedOptions.find(option=>String(option.value ?? '')===String(value ?? ''));
  if(match){return match.label||match.value||'Select...';}
  return selectedValue || 'Select...';
 };
 const updateButton=()=>{button.textContent=labelFor(selectedValue);};
 const chooseOption=async(optionValue,optionButton)=>{
  if(isChoosing){return;}
  isChoosing=true;
  selectedValue=optionValue;
  for(const child of menu.children){child.classList.toggle('selected',child===optionButton);}
  updateButton();
  menu.hidden=true;
  try{
   await onCommit(selectedValue);
  }finally{
   setTimeout(()=>{isChoosing=false;},0);
  }
 };
 for(const option of normalizedOptions){
  const optionValue=String(option.value ?? '');
  const optionButton=document.createElement('button');
  optionButton.type='button';
  optionButton.className='option-picker-option';
  optionButton.textContent=option.label||option.value||'Select...';
  optionButton.onpointerdown=(event)=>{
   event.preventDefault();
   chooseOption(optionValue,optionButton);
  };
  optionButton.onmousedown=(event)=>{
   event.preventDefault();
   chooseOption(optionValue,optionButton);
  };
  optionButton.onclick=(event)=>{
   event.preventDefault();
  };
  optionButton.classList.toggle('selected',optionValue===selectedValue);
  menu.append(optionButton);
 }
 button.onclick=(event)=>{event.preventDefault(); menu.hidden=!menu.hidden;};
 button.onblur=()=>setTimeout(()=>{if(!isChoosing){menu.hidden=true;}},160);
	updateButton();
	picker.append(button,menu);
	return picker;
}
)HTML");
		Html += TEXT(R"HTML(
function commitProperty(host,ownerType,componentId,p,value){
 return api('/api/webui/property',{webUIId:host.webUIId,ownerType,componentId,propertyName:p.name,value});
}
function renderBoolProperty(host,ownerType,componentId,p){
 const {label,control}=createRow(p);
 const input=document.createElement('input');
 input.type='checkbox';
 input.checked=!!p.value;
 input.className='checkbox-field';
 input.onchange=()=>commitProperty(host,ownerType,componentId,p,input.checked);
 control.append(input);
 return label;
}
function renderStringProperty(host,ownerType,componentId,p){
 const {label,control}=createRow(p);
 const actions=p.actions||[];
 if((p.options||[]).length){
  control.append(renderOptionPicker(p.value,p.options,value=>commitProperty(host,ownerType,componentId,p,value).then(load)));
 } else {
  const input=document.createElement('input');
  input.type='text';
  input.value=p.value ?? '';
  input.onchange=()=>commitProperty(host,ownerType,componentId,p,input.value);
  control.append(input);
 }
 if(actions.length){
  const actionsRow=document.createElement('div');
  actionsRow.style.display='flex';
  actionsRow.style.flexWrap='wrap';
  actionsRow.style.gap='8px';
  for(const action of actions){
   const btn=document.createElement('button');
   btn.type='button';
   btn.textContent=action.label || action.id;
   btn.onclick=async()=>{
    btn.disabled=true;
    try{
     await api('/api/webui/action',{webUIId:host.webUIId,ownerType,componentId,actionId:action.id});
     await load();
    }finally{
     btn.disabled=false;
    }
   };
   actionsRow.append(btn);
  }
  control.append(actionsRow);
 }
 return label;
	}
		)HTML");
		Html += TEXT(R"HTML(
function renderNumericProperty(host,ownerType,componentId,p){
 const {label,control}=createRow(p);
 const current=numberOrNull(p.value) ?? 0;
 const {min,max}=resolveNumericBounds(p);
 const step=resolveNumericStep(p);
 if(min!==null&&max!==null){
  const range=document.createElement('input');
  range.type='range';
  range.min=String(min);
  range.max=String(max);
  range.step=String(step);
  range.value=String(current);
  range.className='range-field';
  const number=document.createElement('input');
  number.type='number';
  number.min=String(min);
  number.max=String(max);
  number.step=String(step);
  number.value=String(current);
  number.className='numeric-field';
  const sync=(value,send)=>{
   const next=String(value);
   range.value=next;
   number.value=next;
   if(send){commitProperty(host,ownerType,componentId,p,p.type==='int32'?parseInt(next,10):parseFloat(next));}
  };
  range.oninput=()=>sync(range.value,false);
  range.onchange=()=>sync(range.value,true);
  number.onchange=()=>sync(number.value,true);
  control.append(range,number);
  return label;
 }
 const input=document.createElement('input');
 input.type='number';
 if(min!==null) input.min=String(min);
 if(max!==null) input.max=String(max);
 input.step=String(step);
 input.value=String(current);
 input.className='numeric-field';
 input.onchange=()=>commitProperty(host,ownerType,componentId,p,p.type==='int32'?parseInt(input.value,10):parseFloat(input.value));
	control.append(input);
	return label;
}
	)HTML");
		Html += TEXT(R"HTML(
function renderEnumProperty(host,ownerType,componentId,p){
 const {label,control}=createRow(p);
 control.append(renderOptionPicker(p.value,p.options,value=>commitProperty(host,ownerType,componentId,p,value).then(load)));
 return label;
}
function makeNumberInput(value,placeholder){
 const input=document.createElement('input');
 input.type='number';
 input.className='numeric-field';
 input.value=String(value);
 if(placeholder){input.placeholder=placeholder;}
 return input;
}
function renderVectorProperty(host,ownerType,componentId,p){
 const {label,control}=createRow(p);
 const value=p.value||{};
 const fields=[['x','X'],['y','Y'],['z','Z']];
 const group=document.createElement('div');
 group.className='vector-group';
 const inputs={};
 for(const [key,placeholder] of fields){
  const input=makeNumberInput(numberOrNull(value[key]) ?? 0,placeholder);
  inputs[key]=input;
  group.append(input);
 }
 const commit=()=>commitProperty(host,ownerType,componentId,p,{x:parseFloat(inputs.x.value),y:parseFloat(inputs.y.value),z:parseFloat(inputs.z.value)});
 for(const input of Object.values(inputs)){input.onchange=commit;}
 control.append(group);
 return label;
}
function renderRotatorProperty(host,ownerType,componentId,p){
 const {label,control}=createRow(p);
 const value=p.value||{};
 const fields=[['pitch','Pitch'],['yaw','Yaw'],['roll','Roll']];
 const group=document.createElement('div');
 group.className='rotator-group';
 const inputs={};
 for(const [key,placeholder] of fields){
  const input=makeNumberInput(numberOrNull(value[key]) ?? 0,placeholder);
  inputs[key]=input;
  group.append(input);
 }
 const commit=()=>commitProperty(host,ownerType,componentId,p,{pitch:parseFloat(inputs.pitch.value),yaw:parseFloat(inputs.yaw.value),roll:parseFloat(inputs.roll.value)});
 for(const input of Object.values(inputs)){input.onchange=commit;}
 control.append(group);
 return label;
}
		)HTML");
		Html += TEXT(R"HTML(
function colorToHexComponent(v){
 return Math.max(0,Math.min(255,Math.round((numberOrNull(v) ?? 0)*255))).toString(16).padStart(2,'0');
}
function linearToSrgbComponent(v){
 const x=Math.max(0,numberOrNull(v) ?? 0);
 const srgb=x<=0.0031308 ? x*12.92 : 1.055*Math.pow(x,1/2.4)-0.055;
 return Math.max(0,Math.min(255,Math.round(srgb*255))).toString(16).padStart(2,'0');
}
function srgbToLinearComponent(v){
 const x=Math.max(0,Math.min(1,numberOrNull(v) ?? 0));
 return x<=0.04045 ? x/12.92 : Math.pow((x+0.055)/1.055,2.4);
}
function colorToHex(value,linearSpace=false){
 return linearSpace
  ? `#${linearToSrgbComponent(value.r)}${linearToSrgbComponent(value.g)}${linearToSrgbComponent(value.b)}`
  : `#${colorToHexComponent(value.r)}${colorToHexComponent(value.g)}${colorToHexComponent(value.b)}`;
}
function hexToColor(hex,alpha){
 const clean=String(hex||'').replace('#','');
 const r=parseInt(clean.slice(0,2)||'00',16)/255;
 const g=parseInt(clean.slice(2,4)||'00',16)/255;
 const b=parseInt(clean.slice(4,6)||'00',16)/255;
 return {r,g,b,a:alpha};
}
function normalizeHexInput(value){
 const clean=String(value||'').trim().replace('#','');
 if(/^[0-9a-fA-F]{3}$/.test(clean)){
  return `#${clean[0]}${clean[0]}${clean[1]}${clean[1]}${clean[2]}${clean[2]}`;
 }
 if(/^[0-9a-fA-F]{6}$/.test(clean)){
  return `#${clean}`;
 }
 return null;
}
function colorToCssRgba(value,linearSpace=false){
 const r=Math.max(0,Math.min(255,Math.round((linearSpace ? (numberOrNull(value.r) ?? 0) <= 0.0031308 ? (numberOrNull(value.r) ?? 0) * 12.92 : 1.055 * Math.pow(numberOrNull(value.r) ?? 0, 1 / 2.4) - 0.055 : (numberOrNull(value.r) ?? 0))*255)));
 const g=Math.max(0,Math.min(255,Math.round((linearSpace ? (numberOrNull(value.g) ?? 0) <= 0.0031308 ? (numberOrNull(value.g) ?? 0) * 12.92 : 1.055 * Math.pow(numberOrNull(value.g) ?? 0, 1 / 2.4) - 0.055 : (numberOrNull(value.g) ?? 0))*255)));
 const b=Math.max(0,Math.min(255,Math.round((linearSpace ? (numberOrNull(value.b) ?? 0) <= 0.0031308 ? (numberOrNull(value.b) ?? 0) * 12.92 : 1.055 * Math.pow(numberOrNull(value.b) ?? 0, 1 / 2.4) - 0.055 : (numberOrNull(value.b) ?? 0))*255)));
 const a=numberOrNull(value.a);
 return `rgba(${r},${g},${b},${a!==null?a:1})`;
}
function getContrastColor(value){
 const r=(numberOrNull(value.r) ?? 0);
 const g=(numberOrNull(value.g) ?? 0);
 const b=(numberOrNull(value.b) ?? 0);
 const luminance=(0.2126*r)+(0.7152*g)+(0.0722*b);
 return luminance > 0.55 ? '#101216' : '#ffffff';
}
		)HTML");
		Html += TEXT(R"HTML(
function renderColorLikeProperty(host,ownerType,componentId,p,clamp01,displayLinearSpace,pickerLinearSpace){
 const {label,control}=createRow(p);
 const value=p.value||{};
 control.classList.add(clamp01 ? 'color-swatch' : 'linear-color-swatch');
 control.style.backgroundColor=colorToCssRgba(value,displayLinearSpace);
 control.style.color=getContrastColor(value);
 const group=document.createElement('div');
 group.className='linear-color-group';
 const browserColorInput=document.createElement('input');
 browserColorInput.type='color';
 browserColorInput.value=colorToHex(value,pickerLinearSpace);
 const hexInput=document.createElement('input');
 hexInput.type='text';
 hexInput.className='hex-field';
 hexInput.value=colorToHex(value,pickerLinearSpace);
 hexInput.placeholder='#RRGGBB';
 hexInput.maxLength=7;
 const fields=[['r','R'],['g','G'],['b','B'],['a','A']];
 const inputs={};
 const numericGroup=document.createElement('div');
 numericGroup.style.display='grid';
 numericGroup.style.gridTemplateColumns='repeat(4,minmax(0,1fr))';
 numericGroup.style.gap='8px';
 for(const [key,placeholder] of fields){
  const input=document.createElement('input');
  input.type='number';
  input.step='0.01';
  if(clamp01){
   input.min='0';
   input.max='1';
  }
  input.className='numeric-field';
  input.value=String(numberOrNull(value[key]) ?? (key==='a' ? 1 : 0));
  input.placeholder=placeholder;
  inputs[key]=input;
  numericGroup.append(input);
 }
 const refreshSwatch=()=>{
  const next={
   r:clamp01 ? Math.min(1, Math.max(0, parseFloat(inputs.r.value))) : parseFloat(inputs.r.value),
   g:clamp01 ? Math.min(1, Math.max(0, parseFloat(inputs.g.value))) : parseFloat(inputs.g.value),
   b:clamp01 ? Math.min(1, Math.max(0, parseFloat(inputs.b.value))) : parseFloat(inputs.b.value),
   a:clamp01 ? Math.min(1, Math.max(0, parseFloat(inputs.a.value))) : parseFloat(inputs.a.value)
  };
  control.style.backgroundColor=colorToCssRgba(next,displayLinearSpace);
  control.style.color=getContrastColor(next);
  hexInput.value=colorToHex(next,pickerLinearSpace);
  if(!isEmbed){browserColorInput.value=hexInput.value;}
 };
 const commit=()=>{
  const next={
   r:clamp01 ? Math.min(1, Math.max(0, parseFloat(inputs.r.value))) : parseFloat(inputs.r.value),
   g:clamp01 ? Math.min(1, Math.max(0, parseFloat(inputs.g.value))) : parseFloat(inputs.g.value),
   b:clamp01 ? Math.min(1, Math.max(0, parseFloat(inputs.b.value))) : parseFloat(inputs.b.value),
   a:clamp01 ? Math.min(1, Math.max(0, parseFloat(inputs.a.value))) : parseFloat(inputs.a.value)
  };
  commitProperty(host,ownerType,componentId,p,next);
 };
 hexInput.oninput=()=>{
  const normalizedHex=normalizeHexInput(hexInput.value);
  if(!normalizedHex){return;}
  const next=hexToColor(normalizedHex,parseFloat(inputs.a.value));
  inputs.r.value=String(pickerLinearSpace ? srgbToLinearComponent(next.r) : next.r);
  inputs.g.value=String(pickerLinearSpace ? srgbToLinearComponent(next.g) : next.g);
  inputs.b.value=String(pickerLinearSpace ? srgbToLinearComponent(next.b) : next.b);
  refreshSwatch();
  commit();
 };
 if(!isEmbed){
  browserColorInput.oninput=()=>{
   const next=hexToColor(browserColorInput.value,parseFloat(inputs.a.value));
   inputs.r.value=String(pickerLinearSpace ? srgbToLinearComponent(next.r) : next.r);
   inputs.g.value=String(pickerLinearSpace ? srgbToLinearComponent(next.g) : next.g);
   inputs.b.value=String(pickerLinearSpace ? srgbToLinearComponent(next.b) : next.b);
   refreshSwatch();
   commit();
  };
 }
 for(const input of Object.values(inputs)){
  input.onchange=()=>{refreshSwatch(); commit();};
 }
 if(!isEmbed){group.append(browserColorInput);}
 group.append(hexInput,numericGroup);
 control.append(group);
 return label;
}
function renderProperty(host,ownerType,componentId,p){
 if(p.type==='bool') return renderBoolProperty(host,ownerType,componentId,p);
 if(p.type==='int32' || p.type==='float') return renderNumericProperty(host,ownerType,componentId,p);
 if(p.type==='enum') return renderEnumProperty(host,ownerType,componentId,p);
 if(p.type==='vector') return renderVectorProperty(host,ownerType,componentId,p);
 if(p.type==='rotator') return renderRotatorProperty(host,ownerType,componentId,p);
 if(p.type==='color') return renderColorLikeProperty(host,ownerType,componentId,p,true,true,false);
 if(p.type==='linearColor') return renderColorLikeProperty(host,ownerType,componentId,p,false,true,true);
 return renderStringProperty(host,ownerType,componentId,p);
}
function renderProperties(host,ownerType,componentId,properties){
 const section=document.createElement('section');
 for(const p of properties||[]){
  section.append(renderProperty(host,ownerType,componentId,p));
	}
	return section;
}
	)HTML");
		Html += TEXT(R"HTML(
function renderButtonRow(host,ownerType,componentId,buttons){
 const buttonRow=document.createElement('div');
 buttonRow.className='button-list';
 for(const b of buttons||[]){
  const btn=document.createElement('button');
  btn.className='webui-button';
  btn.textContent=b.label || b.id;
  btn.onclick=async()=>{
   btn.classList.add('pressed');
   btn.disabled=true;
   try{
    await api('/api/webui/button',{webUIId:host.webUIId,ownerType,componentId,buttonId:b.id});
    await load();
   }finally{
    btn.classList.remove('pressed');
    btn.disabled=false;
   }
  };
  buttonRow.append(btn);
 }
 return buttonRow;
}
function renderComponent(host,c){
 const cs=document.createElement('section');
 cs.className='component';
 cs.innerHTML=`<h3>${c.name}</h3>`;
 cs.append(renderProperties(host,'component',c.componentId,c.properties));
 const buttonRow=renderButtonRow(host,'component',c.componentId,c.buttons);
 if(c.buttons.length){ cs.append(buttonRow); }
 return cs;
}
async function load(){
 const restoreWebUIId=currentWebUIId;
 const restoreScrollTop=scrollContainer ? scrollContainer.scrollTop : 0;
 scrollContainer=null;
 setScrollingState(false);
 const schema=await (await fetch('/api/webui/schema')).json();
 tabsHost.innerHTML='';
 panelScrollHost.innerHTML='';
 const hosts=[...(schema.hosts||[])].sort((a,b)=>String(a.webUIId).localeCompare(String(b.webUIId)));
 if(!hosts.length){
  const empty=document.createElement('div');
  empty.className='empty';
  empty.textContent='No WebUIHostComponent was found.';
  panelScrollHost.append(empty);
  return;
 }
  const panelScroll=panelScrollHost;
  panelScroll.onscroll=()=>setScrollingState(true);
  scrollContainer=panelScroll;
 const panels=document.createElement('div');
 const requestedWebUIId=currentWebUIId || restoreWebUIId || initialWebUIId;
  const setActive=(webUIId)=>{
   currentWebUIId=webUIId;
   for(const tab of tabsHost.querySelectorAll('[data-webui-id]')){
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
   tabsHost.append(tab);

  const panel=document.createElement('section');
  panel.className='panel';
  panel.dataset.webuiPanel=host.webUIId;
  panel.innerHTML=`<h2>${host.webUIId}</h2><div class="host-meta">${host.description || host.actorName}</div>`;
  if((host.actorProperties||[]).length || (host.actorButtons||[]).length || (host.hostButtons||[]).length){
   const actorSection=document.createElement('section');
   actorSection.className='component';
   actorSection.innerHTML='<h3>Actor</h3>';
   if((host.actorProperties||[]).length){
    actorSection.append(renderProperties(host,'actor','',host.actorProperties));
   }
   if((host.actorButtons||[]).length){
    actorSection.append(renderButtonRow(host,'actor','',host.actorButtons));
   }
   if((host.hostButtons||[]).length){
    actorSection.append(renderButtonRow(host,'host','',host.hostButtons));
   }
   panel.append(actorSection);
  }
  for(const c of host.components||[]){
   panel.append(renderComponent(host,c));
  }
   panels.append(panel);
  }
  panelScroll.append(panels);
  setActive(hosts.find(host=>host.webUIId===requestedWebUIId)?.webUIId ?? hosts.find(host=>host.webUIId===restoreWebUIId)?.webUIId ?? hosts[0].webUIId);
  requestAnimationFrame(()=>{
   panelScroll.scrollTop=restoreScrollTop;
  });
 }
 load();
</script>
</body>
</html>
		)HTML");
		return Html;
	}

	TSharedRef<FJsonObject> MakeErrorObject(const FString& Error)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetBoolField(TEXT("ok"), false);
		Object->SetStringField(TEXT("error"), Error);
		return Object;
	}

	bool IsWebUICategory(const FString& Category)
	{
		FString Normalized = Category;
		Normalized.ReplaceInline(TEXT(" "), TEXT(""));
		return Normalized.Equals(TEXT("WebUI"), ESearchCase::IgnoreCase);
	}

	bool IsWebUIButtonCategory(const FString& Category)
	{
		FString Normalized = Category;
		Normalized.ReplaceInline(TEXT(" "), TEXT(""));
		return Normalized.Equals(TEXT("WebUI"), ESearchCase::IgnoreCase);
	}

	TOptional<double> ReadMetaNumber(const FProperty* Property, const TCHAR* Key)
	{
		if (!Property || !Property->HasMetaData(Key))
		{
			return {};
		}

		double Value = 0.0;
		const FString MetaValue = Property->GetMetaData(Key);
		if (!MetaValue.IsEmpty() && LexTryParseString(Value, *MetaValue))
		{
			return Value;
		}
		return {};
	}

	void AddNumericSchemaFields(const FProperty* Property, TSharedRef<FJsonObject> PropertyObject)
	{
		const TOptional<double> UIMin = ReadMetaNumber(Property, TEXT("UIMin"));
		const TOptional<double> UIMax = ReadMetaNumber(Property, TEXT("UIMax"));
		const TOptional<double> ClampMin = ReadMetaNumber(Property, TEXT("ClampMin"));
		const TOptional<double> ClampMax = ReadMetaNumber(Property, TEXT("ClampMax"));
		const TOptional<double> SliderMin = ReadMetaNumber(Property, TEXT("SliderMin"));
		const TOptional<double> SliderMax = ReadMetaNumber(Property, TEXT("SliderMax"));
		const TOptional<double> Delta = ReadMetaNumber(Property, TEXT("Delta"));

		if (UIMin.IsSet()) PropertyObject->SetNumberField(TEXT("uiMin"), UIMin.GetValue());
		if (UIMax.IsSet()) PropertyObject->SetNumberField(TEXT("uiMax"), UIMax.GetValue());
		if (ClampMin.IsSet()) PropertyObject->SetNumberField(TEXT("clampMin"), ClampMin.GetValue());
		if (ClampMax.IsSet()) PropertyObject->SetNumberField(TEXT("clampMax"), ClampMax.GetValue());
		if (SliderMin.IsSet()) PropertyObject->SetNumberField(TEXT("sliderMin"), SliderMin.GetValue());
		if (SliderMax.IsSet()) PropertyObject->SetNumberField(TEXT("sliderMax"), SliderMax.GetValue());
		if (Delta.IsSet()) PropertyObject->SetNumberField(TEXT("step"), Delta.GetValue());
	}

	TSharedRef<FJsonObject> MakeButtonObject(const FString& Id, const FString& Label, const FString& Kind)
	{
		TSharedRef<FJsonObject> ButtonObject = MakeShared<FJsonObject>();
		ButtonObject->SetStringField(TEXT("id"), Id);
		ButtonObject->SetStringField(TEXT("label"), Label);
		ButtonObject->SetStringField(TEXT("kind"), Kind);
		return ButtonObject;
	}

	TSharedRef<FJsonObject> MakeOptionObject(const FString& Value, const FString& Label)
	{
		TSharedRef<FJsonObject> OptionObject = MakeShared<FJsonObject>();
		OptionObject->SetStringField(TEXT("value"), Value);
		OptionObject->SetStringField(TEXT("label"), Label);
		return OptionObject;
	}

	bool IsWebUIButtonFunction(const UFunction* Function)
	{
		if (!Function || Function->NumParms != 0 || !Function->HasMetaData(TEXT("Category")))
		{
			return false;
		}

		if (!IsWebUIButtonCategory(Function->GetMetaData(TEXT("Category"))))
		{
			return false;
		}

		static const TSet<FName> IgnoredButtonFunctions = {
			TEXT("RegisterWebUIButton"),
			TEXT("UnregisterWebUIButton"),
			TEXT("ClearWebUIButtons"),
			TEXT("ClearAllButtons"),
			TEXT("StartWebUIServer"),
			TEXT("StopWebUIServer")
		};

		return !IgnoredButtonFunctions.Contains(Function->GetFName());
	}

	void AddButtonIfMissing(TSet<FName>& SeenButtons, TArray<TSharedPtr<FJsonValue>>& ButtonValues, const FName ButtonId, const FString& Label, const FString& Kind)
	{
		if (ButtonId.IsNone() || SeenButtons.Contains(ButtonId))
		{
			return;
		}

		SeenButtons.Add(ButtonId);
		ButtonValues.Add(MakeShared<FJsonValueObject>(MakeButtonObject(ButtonId.ToString(), Label, Kind)));
	}

	void AppendWebUIButtonFunctions(UObject* Owner, TArray<TSharedPtr<FJsonValue>>& ButtonValues)
	{
		if (!IsValid(Owner))
		{
			return;
		}

		TSet<FName> SeenButtons;
		for (TFieldIterator<UFunction> It(Owner->GetClass()); It; ++It)
		{
			UFunction* Function = *It;
			if (!IsWebUIButtonFunction(Function))
			{
				continue;
			}

			const FString Label = Function->GetDisplayNameText().ToString();
			AddButtonIfMissing(
				SeenButtons,
				ButtonValues,
				Function->GetFName(),
				Label.IsEmpty() ? Function->GetName() : Label,
				TEXT("function"));
		}

#if WITH_EDITOR
		if (const UBlueprint* Blueprint = Cast<UBlueprint>(Owner->GetClass()->ClassGeneratedBy))
		{
			for (UEdGraph* Graph : Blueprint->FunctionGraphs)
			{
				if (!Graph)
				{
					continue;
				}

				FKismetUserDeclaredFunctionMetadata* MetaData = FBlueprintEditorUtils::GetGraphFunctionMetaData(Graph);
				if (!MetaData || !IsWebUIButtonCategory(MetaData->Category.ToString()))
				{
					continue;
				}

				AddButtonIfMissing(SeenButtons, ButtonValues, Graph->GetFName(), Graph->GetName(), TEXT("blueprintFunction"));
			}
		}
#endif
	}

	bool InvokeWebUIButtonFunction(UObject* Owner, const FName ButtonId, FString& OutError)
	{
		if (!IsValid(Owner) || ButtonId.IsNone())
		{
			OutError = TEXT("Button not found");
			return false;
		}

		UFunction* Function = Owner->FindFunction(ButtonId);
		if (IsWebUIButtonFunction(Function))
		{
			Owner->ProcessEvent(Function, nullptr);
			return true;
		}

#if WITH_EDITOR
		if (const UBlueprint* Blueprint = Cast<UBlueprint>(Owner->GetClass()->ClassGeneratedBy))
		{
			for (UEdGraph* Graph : Blueprint->FunctionGraphs)
			{
				if (!Graph || Graph->GetFName() != ButtonId)
				{
					continue;
				}

				FKismetUserDeclaredFunctionMetadata* MetaData = FBlueprintEditorUtils::GetGraphFunctionMetaData(Graph);
				if (MetaData && IsWebUIButtonCategory(MetaData->Category.ToString()))
				{
					Owner->ProcessEvent(Function, nullptr);
					return true;
				}
				break;
			}
		}
#endif

		if (!IsWebUIButtonFunction(Function))
		{
			OutError = TEXT("Button not found");
			return false;
		}
		return false;
	}

	bool InvokeWebUIActionFunction(UObject* Owner, const FName ActionId, FString& OutError)
	{
		if (!IsValid(Owner) || ActionId.IsNone())
		{
			OutError = TEXT("Action not found");
			return false;
		}

		UFunction* Function = Owner->FindFunction(ActionId);
		if (!Function || Function->NumParms != 0)
		{
			OutError = TEXT("Action not found");
			return false;
		}

		Owner->ProcessEvent(Function, nullptr);
		return true;
	}

	bool InvokeWebUIStringFunction(UObject* Owner, const FName FunctionName, const FString& Value, FString& OutError)
	{
		if (!IsValid(Owner) || FunctionName.IsNone())
		{
			OutError = TEXT("Callback not found");
			return false;
		}

		UFunction* Function = Owner->FindFunction(FunctionName);
		if (!Function || Function->NumParms != 1)
		{
			OutError = TEXT("Callback not found");
			return false;
		}

		FProperty* ParamProperty = Function->PropertyLink;
		while (ParamProperty && !ParamProperty->HasAnyPropertyFlags(CPF_Parm))
		{
			ParamProperty = ParamProperty->PropertyLinkNext;
		}

		FStrProperty* StringProperty = CastField<FStrProperty>(ParamProperty);
		if (!StringProperty)
		{
			OutError = TEXT("Callback must take a string parameter");
			return false;
		}

		TArray<uint8> Params;
		Params.SetNumZeroed(Function->ParmsSize);
		Function->InitializeStruct(Params.GetData());
		StringProperty->SetPropertyValue(StringProperty->ContainerPtrToValuePtr<void>(Params.GetData()), Value);
		Owner->ProcessEvent(Function, Params.GetData());
		Function->DestroyStruct(Params.GetData());
		return true;
	}

	void AddStringOptionFields(UObject* Owner, const FProperty* Property, TSharedRef<FJsonObject> PropertyObject)
	{
		if (!IsValid(Owner) || !Property || !Property->HasMetaData(TEXT("WebUIOptions")))
		{
			return;
		}

		const FString OptionsFunctionName = Property->GetMetaData(TEXT("WebUIOptions"));
		if (OptionsFunctionName.IsEmpty())
		{
			return;
		}

		UFunction* Function = Owner->FindFunction(FName(*OptionsFunctionName));
		if (!Function)
		{
			return;
		}

		FArrayProperty* ArrayProperty = nullptr;
		for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			FProperty* ParameterProperty = *It;
			if (ParameterProperty->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				continue;
			}
			ArrayProperty = CastField<FArrayProperty>(ParameterProperty);
			break;
		}

		FStrProperty* InnerStringProperty = ArrayProperty ? CastField<FStrProperty>(ArrayProperty->Inner) : nullptr;
		if (!ArrayProperty || !InnerStringProperty)
		{
			return;
		}

		TArray<uint8> Params;
		Params.SetNumZeroed(Function->ParmsSize);
		Function->InitializeStruct(Params.GetData());
		Owner->ProcessEvent(Function, Params.GetData());

		void* ArrayPtr = ArrayProperty->ContainerPtrToValuePtr<void>(Params.GetData());
		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayPtr);
		if (ArrayHelper.Num() <= 0)
		{
			Function->DestroyStruct(Params.GetData());
			return;
		}

		TArray<TSharedPtr<FJsonValue>> Options;
		Options.Reserve(ArrayHelper.Num());
		for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
		{
			const FString OptionValue = InnerStringProperty->GetPropertyValue(ArrayHelper.GetRawPtr(Index));
			Options.Add(MakeShared<FJsonValueObject>(MakeOptionObject(OptionValue, OptionValue)));
		}

		if (!Options.IsEmpty())
		{
			PropertyObject->SetArrayField(TEXT("options"), Options);
		}

		Function->DestroyStruct(Params.GetData());
	}

	void AddStringActionFields(UObject* Owner, const FProperty* Property, TSharedRef<FJsonObject> PropertyObject)
	{
		if (!IsValid(Owner) || !Property || !Property->HasMetaData(TEXT("WebUIActions")))
		{
			return;
		}

		const FString ActionsMeta = Property->GetMetaData(TEXT("WebUIActions"));
		TArray<FString> ActionNames;
		ActionsMeta.ParseIntoArray(ActionNames, TEXT(","), true);

		TArray<TSharedPtr<FJsonValue>> Actions;
		for (FString ActionName : ActionNames)
		{
			ActionName.TrimStartAndEndInline();
			if (ActionName.IsEmpty())
			{
				continue;
			}

			const FName ActionId(*ActionName);
			UFunction* ActionFunction = Owner->FindFunction(ActionId);
			const FString Label = ActionFunction ? ActionFunction->GetDisplayNameText().ToString() : ActionName;
			Actions.Add(MakeShared<FJsonValueObject>(MakeButtonObject(ActionName, Label.IsEmpty() ? ActionName : Label, TEXT("action"))));
		}

		if (!Actions.IsEmpty())
		{
			PropertyObject->SetArrayField(TEXT("actions"), Actions);
		}
	}

	void AddEnumSchemaFields(const FProperty* Property, TSharedRef<FJsonObject> PropertyObject)
	{
		const UEnum* Enum = nullptr;
		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			Enum = EnumProperty->GetEnum();
		}
		else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
		{
			Enum = ByteProperty->Enum;
		}

		if (!Enum)
		{
			return;
		}

		TArray<TSharedPtr<FJsonValue>> Options;
		for (int32 Index = 0; Index < Enum->NumEnums(); ++Index)
		{
			const FString EnumName = Enum->GetNameStringByIndex(Index);
			if (EnumName == TEXT("MAX") || EnumName.EndsWith(TEXT("_MAX")))
			{
				continue;
			}

			TSharedRef<FJsonObject> OptionObject = MakeShared<FJsonObject>();
			OptionObject->SetStringField(TEXT("value"), EnumName);
			OptionObject->SetStringField(TEXT("label"), Enum->GetDisplayNameTextByIndex(Index).ToString());
			Options.Add(MakeShared<FJsonValueObject>(OptionObject));
		}
		PropertyObject->SetArrayField(TEXT("options"), Options);
	}
}

bool UWebUIRuntimeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

bool UWebUIRuntimeSubsystem::StartServerFromSettings()
{
	ApplyHttpServerBindAddressSetting();
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
	RouteHandles.Add(Router->BindRoute(FHttpPath(TEXT("/api/webui/action")), EHttpServerRequestVerbs::VERB_POST, FHttpRequestHandler::CreateUObject(this, &UWebUIRuntimeSubsystem::HandleAction)));
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

void UWebUIRuntimeSubsystem::ApplyHttpServerBindAddressSetting() const
{
	const UWebUIRuntimeSettings* Settings = GetDefault<UWebUIRuntimeSettings>();
	const FString BindAddress = Settings && Settings->bAllowRemoteAccess ? TEXT("any") : TEXT("localhost");
	GConfig->SetString(TEXT("HTTPServer.Listeners"), TEXT("DefaultBindAddress"), *BindAddress, GEngineIni);
	GConfig->Flush(false, GEngineIni);
}

FString UWebUIRuntimeSubsystem::GetWebUITitle() const
{
	FString ProjectName;
	if (const UGeneralProjectSettings* ProjectSettings = GetDefault<UGeneralProjectSettings>())
	{
		ProjectName = ProjectSettings->ProjectName.TrimStartAndEnd();
	}

	FString LevelName;
	for (UWebUIHostComponent* Host : Hosts)
	{
		if (!IsValid(Host) || !IsValid(Host->GetOwner()))
		{
			continue;
		}

		if (const UWorld* World = Host->GetOwner()->GetWorld())
		{
			LevelName = FPackageName::GetShortName(UWorld::RemovePIEPrefix(World->GetMapName()));
			break;
		}
	}

	if (LevelName.IsEmpty())
	{
		LevelName = TEXT("Level");
	}

	if (ProjectName.IsEmpty())
	{
		return LevelName;
	}

	return FString::Printf(TEXT("%s-%s"), *ProjectName, *LevelName);
}

bool UWebUIRuntimeSubsystem::HandleWebUI(const FHttpServerRequest& Request, const TFunction<void(TUniquePtr<FHttpServerResponse>&&)>& OnComplete)
{
	FString Html = BuildWebUIHtml();
	Html.ReplaceInline(TEXT("__WEBUI_TITLE__"), *GetWebUITitle(), ESearchCase::CaseSensitive);
	OnComplete(FHttpServerResponse::Create(MoveTemp(Html), HtmlContentType));
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
	const FString OwnerType = Body->HasField(TEXT("ownerType")) ? Body->GetStringField(TEXT("ownerType")) : TEXT("component");
	const FString ComponentId = Body->HasField(TEXT("componentId")) ? Body->GetStringField(TEXT("componentId")) : FString();
	const FString PropertyName = Body->GetStringField(TEXT("propertyName"));
	const TSharedPtr<FJsonValue> Value = Body->TryGetField(TEXT("value"));
	UWebUIHostComponent* Host = nullptr;
	UObject* Owner = FindPropertyOwner(WebUIId, OwnerType, ComponentId, Host);

	if (!Owner || !Value.IsValid() || !SetPropertyFromJson(Owner, Host, PropertyName, Value, Error))
	{
		OnComplete(MakeJsonResponse(MakeErrorObject(Error.IsEmpty() ? TEXT("Failed to set property") : Error)));
		return true;
	}

	TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("ok"), true);
	OnComplete(MakeJsonResponse(Response));
	return true;
}

bool UWebUIRuntimeSubsystem::HandleAction(const FHttpServerRequest& Request, const TFunction<void(TUniquePtr<FHttpServerResponse>&&)>& OnComplete)
{
	FString Error;
	TSharedPtr<FJsonObject> Body = ParseRequestJson(Request, Error);
	if (!Body.IsValid())
	{
		OnComplete(MakeJsonResponse(MakeErrorObject(Error)));
		return true;
	}

	const FString WebUIId = Body->GetStringField(TEXT("webUIId"));
	const FString OwnerType = Body->HasField(TEXT("ownerType")) ? Body->GetStringField(TEXT("ownerType")) : TEXT("component");
	const FString ComponentId = Body->HasField(TEXT("componentId")) ? Body->GetStringField(TEXT("componentId")) : FString();
	const FName ActionId(*Body->GetStringField(TEXT("actionId")));
	UWebUIHostComponent* Host = nullptr;
	UObject* Owner = FindPropertyOwner(WebUIId, OwnerType, ComponentId, Host);
	if (!Owner || ActionId.IsNone())
	{
		OnComplete(MakeJsonResponse(MakeErrorObject(TEXT("Action not found"))));
		return true;
	}

	if (!InvokeWebUIActionFunction(Owner, ActionId, Error))
	{
		OnComplete(MakeJsonResponse(MakeErrorObject(Error.IsEmpty() ? TEXT("Action not found") : Error)));
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
	const FString OwnerType = Body->HasField(TEXT("ownerType")) ? Body->GetStringField(TEXT("ownerType")) : TEXT("component");
	const FString ComponentId = Body->GetStringField(TEXT("componentId"));
	const FName ButtonId(*Body->GetStringField(TEXT("buttonId")));
	UWebUIHostComponent* Host = nullptr;
	UObject* Owner = FindPropertyOwner(WebUIId, OwnerType, ComponentId, Host);
	if (!Owner || ButtonId.IsNone())
	{
		OnComplete(MakeJsonResponse(MakeErrorObject(TEXT("Button not found"))));
		return true;
	}

	if (OwnerType.Equals(TEXT("host"), ESearchCase::IgnoreCase))
	{
		if (!Host || !Host->GetWebUIButtons().Contains(ButtonId))
		{
			OnComplete(MakeJsonResponse(MakeErrorObject(TEXT("Button not found"))));
			return true;
		}
		Host->NotifyWebUIButtonClicked(ButtonId);
	}
	else if (OwnerType.Equals(TEXT("actor"), ESearchCase::IgnoreCase))
	{
		FString FunctionError;
		if (InvokeWebUIButtonFunction(Owner, ButtonId, FunctionError))
		{
			TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>();
			Response->SetBoolField(TEXT("ok"), true);
			OnComplete(MakeJsonResponse(Response));
			return true;
		}

		if (AWebUIHostActor* HostActor = Cast<AWebUIHostActor>(Owner))
		{
			if (!HostActor->GetWebUIButtons().Contains(ButtonId))
			{
				OnComplete(MakeJsonResponse(MakeErrorObject(TEXT("Button not found"))));
				return true;
			}
			HostActor->NotifyWebUIButtonClicked(ButtonId);
		}
		else
		{
			OnComplete(MakeJsonResponse(MakeErrorObject(TEXT("Button not found"))));
			return true;
		}
	}
	else
	{
		FString FunctionError;
		if (InvokeWebUIButtonFunction(Owner, ButtonId, FunctionError))
		{
			TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>();
			Response->SetBoolField(TEXT("ok"), true);
			OnComplete(MakeJsonResponse(Response));
			return true;
		}

		UWebUIComponentBase* Component = Cast<UWebUIComponentBase>(Owner);
		if (!Component || !Component->GetWebUIButtons().Contains(ButtonId))
		{
			OnComplete(MakeJsonResponse(MakeErrorObject(TEXT("Button not found"))));
			return true;
		}
		Component->NotifyWebUIButtonClicked(ButtonId);
	}

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
		HostObject->SetStringField(TEXT("description"), Host->GetDescription());
		TArray<TSharedPtr<FJsonValue>> HostButtonValues;
		for (const FName Button : Host->GetWebUIButtons())
		{
			HostButtonValues.Add(MakeShared<FJsonValueObject>(MakeButtonObject(Button.ToString(), Button.ToString(), TEXT("registered"))));
		}
		HostObject->SetArrayField(TEXT("hostButtons"), HostButtonValues);
		if (TSharedPtr<FJsonObject> ActorObject = BuildActorSchema(Cast<AActor>(Host->GetOwner())))
		{
			HostObject->SetArrayField(TEXT("actorProperties"), ActorObject->GetArrayField(TEXT("properties")));
			HostObject->SetArrayField(TEXT("actorButtons"), ActorObject->GetArrayField(TEXT("buttons")));
		}

		TArray<TSharedPtr<FJsonValue>> ComponentValues;
		TArray<UActorComponent*> Components;
		Host->GetOwner()->GetComponents(Components);
		for (UActorComponent* Component : Components)
		{
			if (Cast<UWebUIHostComponent>(Component))
			{
				continue;
			}
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

TSharedPtr<FJsonObject> UWebUIRuntimeSubsystem::BuildActorSchema(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	TArray<TSharedPtr<FJsonValue>> PropertyValues;
	for (TFieldIterator<FProperty> It(Actor->GetClass()); It; ++It)
	{
		FProperty* Property = *It;
		if (!IsWebUIProperty(Property))
		{
			continue;
		}

		TSharedRef<FJsonObject> PropertyObject = MakeShared<FJsonObject>();
		PropertyObject->SetStringField(TEXT("name"), Property->GetName());
		PropertyObject->SetStringField(TEXT("type"), GetPropertyWebUIType(Property));
		PropertyObject->SetField(TEXT("value"), PropertyToJsonValue(Property, Actor));
		if (PropertyObject->GetStringField(TEXT("type")) == TEXT("float") || PropertyObject->GetStringField(TEXT("type")) == TEXT("int32"))
		{
			AddNumericSchemaFields(Property, PropertyObject);
		}
		if (PropertyObject->GetStringField(TEXT("type")) == TEXT("enum"))
		{
			AddEnumSchemaFields(Property, PropertyObject);
		}
		if (PropertyObject->GetStringField(TEXT("type")) == TEXT("string"))
		{
			AddStringOptionFields(Actor, Property, PropertyObject);
			AddStringActionFields(Actor, Property, PropertyObject);
		}
		PropertyValues.Add(MakeShared<FJsonValueObject>(PropertyObject));
	}

	TArray<TSharedPtr<FJsonValue>> ButtonValues;
	AppendWebUIButtonFunctions(Actor, ButtonValues);

	TSharedRef<FJsonObject> ActorObject = MakeShared<FJsonObject>();
	ActorObject->SetArrayField(TEXT("properties"), PropertyValues);
	ActorObject->SetArrayField(TEXT("buttons"), ButtonValues);
	return ActorObject;
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
		if (PropertyObject->GetStringField(TEXT("type")) == TEXT("float") || PropertyObject->GetStringField(TEXT("type")) == TEXT("int32"))
		{
			AddNumericSchemaFields(Property, PropertyObject);
		}
		if (PropertyObject->GetStringField(TEXT("type")) == TEXT("enum"))
		{
			AddEnumSchemaFields(Property, PropertyObject);
		}
		if (PropertyObject->GetStringField(TEXT("type")) == TEXT("string"))
		{
			AddStringOptionFields(Component, Property, PropertyObject);
			AddStringActionFields(Component, Property, PropertyObject);
		}
		PropertyValues.Add(MakeShared<FJsonValueObject>(PropertyObject));
	}

	TArray<TSharedPtr<FJsonValue>> ButtonValues;
	AppendWebUIButtonFunctions(Component, ButtonValues);
	if (const UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Component))
	{
		for (const FName Button : WebUIComponent->GetWebUIButtons())
		{
			ButtonValues.Add(MakeShared<FJsonValueObject>(MakeButtonObject(Button.ToString(), Button.ToString(), TEXT("registered"))));
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

UObject* UWebUIRuntimeSubsystem::FindPropertyOwner(const FString& WebUIId, const FString& OwnerType, const FString& ComponentId, UWebUIHostComponent*& OutHost) const
{
	for (UWebUIHostComponent* Host : Hosts)
	{
		if (!IsValid(Host) || Host->GetWebUIId() != WebUIId || !IsValid(Host->GetOwner()))
		{
			continue;
		}

	OutHost = Host;
		if (OwnerType.Equals(TEXT("host"), ESearchCase::IgnoreCase))
		{
			return Host;
		}
		if (OwnerType.Equals(TEXT("actor"), ESearchCase::IgnoreCase))
		{
			return Host->GetOwner();
		}
		if (OwnerType.Equals(TEXT("component"), ESearchCase::IgnoreCase))
		{
			return FindComponent(WebUIId, ComponentId);
		}
		break;
	}

	return nullptr;
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

bool UWebUIRuntimeSubsystem::SetPropertyFromJson(UObject* Owner, UWebUIHostComponent* Host, const FString& PropertyName, const TSharedPtr<FJsonValue>& Value, FString& OutError, bool bPersistAfterChange)
{
	FProperty* Property = FindFProperty<FProperty>(Owner->GetClass(), *PropertyName);
	if (!Property || !IsWebUIProperty(Property))
	{
		OutError = TEXT("Property not found or not exposed to WebUI");
		return false;
	}

	auto NotifyPropertyChanged = [&](FName Name)
	{
		if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Owner))
		{
			WebUIComponent->NotifyWebUIPropertyChanged(Name);
		}
		else if (Host)
		{
			Host->NotifyWebUIPropertyChanged(Name);
		}
	};

	auto NotifyBoolChanged = [&](FName Name, bool ValueToSend)
	{
		if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Owner))
		{
			WebUIComponent->NotifyWebUIBoolChanged(Name, ValueToSend);
		}
		else if (Host)
		{
			Host->NotifyWebUIBoolChanged(Name, ValueToSend);
		}
	};

	auto NotifyFloatChanged = [&](FName Name, double ValueToSend)
	{
		if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Owner))
		{
			WebUIComponent->NotifyWebUIFloatChanged(Name, ValueToSend);
		}
		else if (Host)
		{
			Host->NotifyWebUIFloatChanged(Name, ValueToSend);
		}
	};

	auto NotifyStringChanged = [&](FName Name, const FString& ValueToSend)
	{
		if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Owner))
		{
			WebUIComponent->NotifyWebUIStringChanged(Name, ValueToSend);
		}
		else if (Host)
		{
			Host->NotifyWebUIStringChanged(Name, ValueToSend);
		}
	};

	auto NotifyVectorChanged = [&](FName Name, FVector ValueToSend)
	{
		if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Owner))
		{
			WebUIComponent->NotifyWebUIVectorChanged(Name, ValueToSend);
		}
		else if (Host)
		{
			Host->NotifyWebUIVectorChanged(Name, ValueToSend);
		}
	};

	auto NotifyRotatorChanged = [&](FName Name, FRotator ValueToSend)
	{
		if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Owner))
		{
			WebUIComponent->NotifyWebUIRotatorChanged(Name, ValueToSend);
		}
		else if (Host)
		{
			Host->NotifyWebUIRotatorChanged(Name, ValueToSend);
		}
	};

	auto NotifyColorChanged = [&](FName Name, FLinearColor ValueToSend)
	{
		if (UWebUIComponentBase* WebUIComponent = Cast<UWebUIComponentBase>(Owner))
		{
			WebUIComponent->NotifyWebUIColorChanged(Name, ValueToSend);
		}
		else if (Host)
		{
			Host->NotifyWebUIColorChanged(Name, ValueToSend);
		}
	};

	void* PropertyPtr = Property->ContainerPtrToValuePtr<void>(Owner);
	if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
	{
		BoolProperty->SetPropertyValue(PropertyPtr, Value->AsBool());
		NotifyBoolChanged(*PropertyName, Value->AsBool());
	}
	else if (FIntProperty* IntProperty = CastField<FIntProperty>(Property))
	{
		IntProperty->SetPropertyValue(PropertyPtr, static_cast<int32>(Value->AsNumber()));
	}
	else if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
	{
		const double Number = Value->AsNumber();
		FloatProperty->SetPropertyValue(PropertyPtr, static_cast<float>(Number));
		NotifyFloatChanged(*PropertyName, Number);
	}
	else if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
	{
		const double Number = Value->AsNumber();
		DoubleProperty->SetPropertyValue(PropertyPtr, Number);
		NotifyFloatChanged(*PropertyName, Number);
	}
	else if (FStrProperty* StringProperty = CastField<FStrProperty>(Property))
	{
		const FString StringValue = Value->AsString();
		StringProperty->SetPropertyValue(PropertyPtr, StringValue);
		NotifyStringChanged(*PropertyName, StringValue);
		const FString OnChangedFunctionName = Property->GetMetaData(TEXT("WebUIOnChanged"));
		if (!OnChangedFunctionName.IsEmpty())
		{
			FString CallbackError;
			if (!InvokeWebUIStringFunction(Owner, FName(*OnChangedFunctionName), StringValue, CallbackError))
			{
				OutError = CallbackError;
				return false;
			}
		}
	}
	else if (FNameProperty* NameProperty = CastField<FNameProperty>(Property))
	{
		const FString StringValue = Value->AsString();
		NameProperty->SetPropertyValue(PropertyPtr, FName(*StringValue));
		NotifyStringChanged(*PropertyName, StringValue);
		const FString OnChangedFunctionName = Property->GetMetaData(TEXT("WebUIOnChanged"));
		if (!OnChangedFunctionName.IsEmpty())
		{
			FString CallbackError;
			if (!InvokeWebUIStringFunction(Owner, FName(*OnChangedFunctionName), StringValue, CallbackError))
			{
				OutError = CallbackError;
				return false;
			}
		}
	}
	else if (FTextProperty* TextProperty = CastField<FTextProperty>(Property))
	{
		const FString StringValue = Value->AsString();
		TextProperty->SetPropertyValue(PropertyPtr, FText::FromString(StringValue));
		NotifyStringChanged(*PropertyName, StringValue);
		const FString OnChangedFunctionName = Property->GetMetaData(TEXT("WebUIOnChanged"));
		if (!OnChangedFunctionName.IsEmpty())
		{
			FString CallbackError;
			if (!InvokeWebUIStringFunction(Owner, FName(*OnChangedFunctionName), StringValue, CallbackError))
			{
				OutError = CallbackError;
				return false;
			}
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
			NotifyVectorChanged(*PropertyName, Vector);
		}
		else if (StructProperty->Struct == TBaseStructure<FColor>::Get())
		{
			const FLinearColor LinearColor(
				static_cast<float>(ObjectValue->GetNumberField(TEXT("r"))),
				static_cast<float>(ObjectValue->GetNumberField(TEXT("g"))),
				static_cast<float>(ObjectValue->GetNumberField(TEXT("b"))),
				static_cast<float>(ObjectValue->GetNumberField(TEXT("a"))));
			const FColor Color = LinearColor.ToFColor(true);
			*static_cast<FColor*>(PropertyPtr) = Color;
			NotifyColorChanged(*PropertyName, LinearColor);
		}
		else if (StructProperty->Struct == TBaseStructure<FRotator>::Get())
		{
			FRotator Rotator(ObjectValue->GetNumberField(TEXT("pitch")), ObjectValue->GetNumberField(TEXT("yaw")), ObjectValue->GetNumberField(TEXT("roll")));
			*static_cast<FRotator*>(PropertyPtr) = Rotator;
			NotifyRotatorChanged(*PropertyName, Rotator);
		}
		else if (StructProperty->Struct == TBaseStructure<FLinearColor>::Get())
		{
			FLinearColor Color(ObjectValue->GetNumberField(TEXT("r")), ObjectValue->GetNumberField(TEXT("g")), ObjectValue->GetNumberField(TEXT("b")), ObjectValue->GetNumberField(TEXT("a")));
			*static_cast<FLinearColor*>(PropertyPtr) = Color;
			NotifyColorChanged(*PropertyName, Color);
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

	NotifyPropertyChanged(*PropertyName);

	if (bPersistAfterChange && Host && Host->IsAutoSaveChangedValuesEnabled())
	{
		SavePersistedState(Host);
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
		if (StructProperty->Struct == TBaseStructure<FColor>::Get())
		{
			const FColor& Value = *static_cast<const FColor*>(PropertyPtr);
			Object->SetNumberField(TEXT("r"), Value.R / 255.0);
			Object->SetNumberField(TEXT("g"), Value.G / 255.0);
			Object->SetNumberField(TEXT("b"), Value.B / 255.0);
			Object->SetNumberField(TEXT("a"), Value.A / 255.0);
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
		if (StructProperty->Struct == TBaseStructure<FColor>::Get()) return TEXT("color");
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
		&& (Property->HasMetaData(TEXT("WebUI")) || IsWebUICategory(Property->GetMetaData(TEXT("Category"))));
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

bool UWebUIRuntimeSubsystem::SerializeJsonValue(const TSharedPtr<FJsonValue>& Value, FString& OutJson) const
{
	if (!Value.IsValid())
	{
		return false;
	}

	TSharedRef<FJsonObject> Wrapper = MakeShared<FJsonObject>();
	Wrapper->SetField(TEXT("value"), Value);
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
	return FJsonSerializer::Serialize(Wrapper, Writer);
}

TSharedPtr<FJsonValue> UWebUIRuntimeSubsystem::DeserializeJsonValue(const FString& Json, FString& OutError) const
{
	TSharedPtr<FJsonObject> Wrapper;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Wrapper) || !Wrapper.IsValid())
	{
		OutError = TEXT("Invalid saved JSON");
		return nullptr;
	}

	TSharedPtr<FJsonValue> Value = Wrapper->TryGetField(TEXT("value"));
	if (!Value.IsValid())
	{
		OutError = TEXT("Missing saved value");
		return nullptr;
	}

	return Value;
}

void UWebUIRuntimeSubsystem::SavePersistedState(UWebUIHostComponent* Host)
{
	if (!IsValid(Host) || !Host->IsAutoSaveChangedValuesEnabled() || !IsValid(Host->GetOwner()))
	{
		return;
	}

	AActor* Owner = Host->GetOwner();
	const FString HostKey = Host->GetWebUIId();
	if (HostKey.IsEmpty())
	{
		return;
	}

	UWebUIRuntimeSaveGame* SaveGame = nullptr;
	if (UGameplayStatics::DoesSaveGameExist(TEXT("WebUIRuntime"), 0))
	{
		SaveGame = Cast<UWebUIRuntimeSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("WebUIRuntime"), 0));
	}
	if (!SaveGame)
	{
		SaveGame = Cast<UWebUIRuntimeSaveGame>(UGameplayStatics::CreateSaveGameObject(UWebUIRuntimeSaveGame::StaticClass()));
	}
	if (!SaveGame)
	{
		return;
	}

	FWebUIRuntimeSavedHostState HostState;
	auto AppendObjectState = [&](UObject* Object, const FString& OwnerType, const FString& ComponentId)
	{
		if (!IsValid(Object))
		{
			return;
		}

		for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
		{
			FProperty* Property = *It;
			if (!IsWebUIProperty(Property))
			{
				continue;
			}

			TSharedPtr<FJsonValue> JsonValue = PropertyToJsonValue(Property, Object);
			FString ValueJson;
			if (!SerializeJsonValue(JsonValue, ValueJson))
			{
				continue;
			}

			FWebUIRuntimeSavedProperty Entry;
			Entry.OwnerType = OwnerType;
			Entry.ComponentId = ComponentId;
			Entry.PropertyName = Property->GetName();
			Entry.ValueJson = MoveTemp(ValueJson);
			HostState.Properties.Add(MoveTemp(Entry));
		}
	};

	AppendObjectState(Owner, TEXT("actor"), FString());

	TArray<UActorComponent*> Components;
	Owner->GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		if (!IsValid(Component) || Cast<UWebUIHostComponent>(Component))
		{
			continue;
		}
		AppendObjectState(Component, TEXT("component"), Component->GetName());
	}

	SaveGame->HostStates.Add(HostKey, MoveTemp(HostState));
	UGameplayStatics::SaveGameToSlot(SaveGame, TEXT("WebUIRuntime"), 0);
}

void UWebUIRuntimeSubsystem::LoadPersistedState(UWebUIHostComponent* Host)
{
	if (!IsValid(Host) || !Host->IsAutoSaveChangedValuesEnabled() || !IsValid(Host->GetOwner()))
	{
		return;
	}

	const FString HostKey = Host->GetWebUIId();
	if (HostKey.IsEmpty())
	{
		return;
	}

	UWebUIRuntimeSaveGame* SaveGame = Cast<UWebUIRuntimeSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("WebUIRuntime"), 0));
	if (!SaveGame)
	{
		return;
	}

	const FWebUIRuntimeSavedHostState* HostState = SaveGame->HostStates.Find(HostKey);
	if (!HostState)
	{
		return;
	}

	for (const FWebUIRuntimeSavedProperty& Entry : HostState->Properties)
	{
		FString Error;
		TSharedPtr<FJsonValue> Value = DeserializeJsonValue(Entry.ValueJson, Error);
		if (!Value.IsValid())
		{
			continue;
		}

		UObject* OwnerObject = FindPropertyOwner(HostKey, Entry.OwnerType, Entry.ComponentId, Host);
		if (!OwnerObject)
		{
			continue;
		}

		SetPropertyFromJson(OwnerObject, Host, Entry.PropertyName, Value, Error, false);
	}
}

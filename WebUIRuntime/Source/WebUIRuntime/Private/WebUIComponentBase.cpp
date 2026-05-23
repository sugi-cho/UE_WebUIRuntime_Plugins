#include "WebUIComponentBase.h"

UWebUIComponentBase::UWebUIComponentBase()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWebUIComponentBase::RegisterWebUIButton(FName ButtonId)
{
	if (!ButtonId.IsNone())
	{
		WebUIButtons.AddUnique(ButtonId);
	}
}

void UWebUIComponentBase::UnregisterWebUIButton(FName ButtonId)
{
	WebUIButtons.Remove(ButtonId);
}

void UWebUIComponentBase::ClearWebUIButtons()
{
	WebUIButtons.Reset();
}

const TArray<FName>& UWebUIComponentBase::GetWebUIButtons() const
{
	return WebUIButtons;
}

void UWebUIComponentBase::NotifyWebUIPropertyChanged(FName PropertyName)
{
	K2_OnWebUIPropertyChanged(PropertyName);
}

void UWebUIComponentBase::NotifyWebUIBoolChanged(FName PropertyName, bool Value)
{
	K2_OnWebUIBoolChanged(PropertyName, Value);
}

void UWebUIComponentBase::NotifyWebUIFloatChanged(FName PropertyName, double Value)
{
	K2_OnWebUIFloatChanged(PropertyName, Value);
}

void UWebUIComponentBase::NotifyWebUIStringChanged(FName PropertyName, const FString& Value)
{
	K2_OnWebUIStringChanged(PropertyName, Value);
}

void UWebUIComponentBase::NotifyWebUIVectorChanged(FName PropertyName, FVector Value)
{
	K2_OnWebUIVectorChanged(PropertyName, Value);
}

void UWebUIComponentBase::NotifyWebUIRotatorChanged(FName PropertyName, FRotator Value)
{
	K2_OnWebUIRotatorChanged(PropertyName, Value);
}

void UWebUIComponentBase::NotifyWebUIColorChanged(FName PropertyName, FLinearColor Value)
{
	K2_OnWebUIColorChanged(PropertyName, Value);
}

void UWebUIComponentBase::NotifyWebUIButtonClicked(FName ButtonId)
{
	HandleWebUIButtonClicked(ButtonId);
	K2_OnWebUIButtonClicked(ButtonId);
}

void UWebUIComponentBase::HandleWebUIButtonClicked(FName ButtonId)
{
}

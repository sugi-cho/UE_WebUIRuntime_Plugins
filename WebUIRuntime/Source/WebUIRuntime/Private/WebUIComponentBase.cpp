#include "WebUIComponentBase.h"

#include "WebUIRuntimeSubsystem.h"

namespace
{
	FString StripWebUIOrderPrefix(const FString& RawLabel)
	{
		int32 SeparatorIndex = INDEX_NONE;
		for (int32 Index = 0; Index < RawLabel.Len(); ++Index)
		{
			const TCHAR Char = RawLabel[Index];
			if (Char == TEXT('_') || Char == TEXT(' '))
			{
				SeparatorIndex = Index;
				break;
			}
			if (!FChar::IsDigit(Char))
			{
				return RawLabel;
			}
		}

		if (SeparatorIndex > 0 && SeparatorIndex < RawLabel.Len())
		{
			const FString Prefix = RawLabel.Left(SeparatorIndex);
			int64 ParsedOrder = 0;
			if (LexTryParseString(ParsedOrder, *Prefix) && ParsedOrder >= 0)
			{
				const FString DisplayLabel = RawLabel.Mid(SeparatorIndex + 1);
				if (!DisplayLabel.IsEmpty())
				{
					return DisplayLabel;
				}
			}
		}

		return RawLabel;
	}

	bool IsWebUIButtonFunction(const UFunction* Function)
	{
		if (!Function || Function->NumParms != 0 || !Function->HasMetaData(TEXT("Category")))
		{
			return false;
		}

		FString Normalized = Function->GetMetaData(TEXT("Category"));
		Normalized.ReplaceInline(TEXT(" "), TEXT(""));
		return Normalized.Equals(TEXT("WebUI"), ESearchCase::IgnoreCase);
	}

	FName ResolveWebUIButtonId(const TArray<FName>& Buttons, const FName RequestedButtonId)
	{
		if (RequestedButtonId.IsNone())
		{
			return NAME_None;
		}

		if (Buttons.Contains(RequestedButtonId))
		{
			return RequestedButtonId;
		}

		const FString RequestedLabel = StripWebUIOrderPrefix(RequestedButtonId.ToString());
		for (const FName& Button : Buttons)
		{
			if (StripWebUIOrderPrefix(Button.ToString()).Equals(RequestedLabel, ESearchCase::IgnoreCase))
			{
				return Button;
			}
		}

		return NAME_None;
	}

	FName ResolveWebUIButtonFunctionId(const UObject* Owner, const FName RequestedButtonId)
	{
		if (!IsValid(Owner) || RequestedButtonId.IsNone())
		{
			return NAME_None;
		}

		UFunction* Function = Owner->FindFunction(RequestedButtonId);
		if (IsWebUIButtonFunction(Function))
		{
			return Function->GetFName();
		}

		const FString RequestedLabel = StripWebUIOrderPrefix(RequestedButtonId.ToString());
		for (TFieldIterator<UFunction> It(Owner->GetClass()); It; ++It)
		{
			UFunction* Candidate = *It;
			if (!IsWebUIButtonFunction(Candidate))
			{
				continue;
			}

			if (StripWebUIOrderPrefix(Candidate->GetName()).Equals(RequestedLabel, ESearchCase::IgnoreCase))
			{
				return Candidate->GetFName();
			}
		}

		return NAME_None;
	}
}

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
	ButtonId = ResolveWebUIButtonId(WebUIButtons, ButtonId);
	if (ButtonId.IsNone())
	{
		return;
	}

	WebUIButtons.Remove(ButtonId);
	WebUIButtonEnabledStates.Remove(ButtonId);
}

void UWebUIComponentBase::ClearWebUIButtons()
{
	WebUIButtons.Reset();
	WebUIButtonEnabledStates.Reset();
}

void UWebUIComponentBase::SetWebUIButtonEnabled(FName ButtonId, bool bEnabled)
{
	const FName RequestedButtonId = ButtonId;
	ButtonId = ResolveWebUIButtonId(WebUIButtons, RequestedButtonId);
	if (ButtonId.IsNone())
	{
		ButtonId = ResolveWebUIButtonFunctionId(this, RequestedButtonId);
	}

	if (!ButtonId.IsNone())
	{
		const bool* CurrentEnabled = WebUIButtonEnabledStates.Find(ButtonId);
		if (CurrentEnabled && *CurrentEnabled == bEnabled)
		{
			return;
		}
		WebUIButtonEnabledStates.Add(ButtonId, bEnabled);

		if (UWorld* World = GetWorld())
		{
			if (UWebUIRuntimeSubsystem* Runtime = World->GetSubsystem<UWebUIRuntimeSubsystem>())
			{
				Runtime->NotifyWebUIComponentStateChanged(this);
			}
		}
	}
}

bool UWebUIComponentBase::IsWebUIButtonEnabled(FName ButtonId) const
{
	const FName RequestedButtonId = ButtonId;
	ButtonId = ResolveWebUIButtonId(WebUIButtons, RequestedButtonId);
	if (ButtonId.IsNone())
	{
		ButtonId = ResolveWebUIButtonFunctionId(this, RequestedButtonId);
	}
	if (ButtonId.IsNone())
	{
		return false;
	}

	if (const bool* bEnabled = WebUIButtonEnabledStates.Find(ButtonId))
	{
		return *bEnabled;
	}

	return true;
}

FString UWebUIComponentBase::GetWebUIDisplayName() const
{
	if (!WebUIDisplayName.IsEmpty())
	{
		return WebUIDisplayName;
	}

	return GetName();
}

void UWebUIComponentBase::SetWebUIDisplayName(const FString& InDisplayName)
{
	WebUIDisplayName = InDisplayName;
}

const TArray<FName>& UWebUIComponentBase::GetWebUIButtons() const
{
	return WebUIButtons;
}

void UWebUIComponentBase::NotifyWebUIPropertyChanged(FName PropertyName)
{
	K2_OnWebUIPropertyChanged(FName(*StripWebUIOrderPrefix(PropertyName.ToString())));
}

void UWebUIComponentBase::NotifyWebUIBoolChanged(FName PropertyName, bool Value)
{
	K2_OnWebUIBoolChanged(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIComponentBase::NotifyWebUIFloatChanged(FName PropertyName, double Value)
{
	K2_OnWebUIFloatChanged(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIComponentBase::NotifyWebUIStringChanged(FName PropertyName, const FString& Value)
{
	K2_OnWebUIStringChanged(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIComponentBase::NotifyWebUIVectorChanged(FName PropertyName, FVector Value)
{
	K2_OnWebUIVectorChanged(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIComponentBase::NotifyWebUIRotatorChanged(FName PropertyName, FRotator Value)
{
	K2_OnWebUIRotatorChanged(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIComponentBase::NotifyWebUIColorChanged(FName PropertyName, FLinearColor Value)
{
	K2_OnWebUIColorChanged(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIComponentBase::NotifyWebUIButtonClicked(FName ButtonId)
{
	HandleWebUIButtonClicked(ButtonId);
	OnWebUIButtonClicked.Broadcast(ButtonId);
	K2_OnWebUIButtonClicked(ButtonId);
}

void UWebUIComponentBase::HandleWebUIButtonClicked(FName ButtonId)
{
}

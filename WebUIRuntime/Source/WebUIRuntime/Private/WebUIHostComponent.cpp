#include "WebUIHostComponent.h"

#include "WebUIHostActor.h"
#include "WebUIRuntime.h"
#include "WebUIRuntimeSubsystem.h"
#include "WebUIRuntimeSettings.h"
#include "Engine/Texture2D.h"
#include "SocketSubsystem.h"

namespace
{
	bool IsWebUIButtonFunction(const UFunction* Function)
	{
		if (!Function || Function->NumParms != 0 || Function->HasAnyFunctionFlags(FUNC_BlueprintPure | FUNC_Event))
		{
			return false;
		}

		const FString FunctionName = Function->GetName();
		int32 OrderStart = 0;
		if (FunctionName.StartsWith(TEXT("WUI"), ESearchCase::IgnoreCase))
		{
			OrderStart = 3;
		}

		int32 PrefixEnd = OrderStart;
		while (PrefixEnd < FunctionName.Len() && FChar::IsDigit(FunctionName[PrefixEnd]))
		{
			++PrefixEnd;
		}

		if (PrefixEnd <= OrderStart || PrefixEnd >= FunctionName.Len())
		{
			return false;
		}

		const TCHAR Separator = FunctionName[PrefixEnd];
		if (Separator != TEXT('_') && Separator != TEXT(' '))
		{
			return false;
		}

		int64 ParsedOrder = 0;
		return LexTryParseString(ParsedOrder, *FunctionName.Mid(OrderStart, PrefixEnd - OrderStart)) && ParsedOrder >= 0;
	}

	constexpr int32 QRCodeVersion = 5;
	constexpr int32 QRCodeSize = 17 + QRCodeVersion * 4;
	constexpr int32 QRCodeDataCodewords = 108;
	constexpr int32 QRCodeEccCodewords = 26;
	constexpr int32 QRCodeMaxByteLength = 106;
	constexpr uint16 QRCodeFormatLMask0 = 0x77C4;

	FString GetLocalIPAddressString()
	{
		bool bCanBindAll = false;
		if (ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
		{
			const TSharedRef<FInternetAddr> LocalHostAddr = SocketSubsystem->GetLocalHostAddr(*GLog, bCanBindAll);
			const FString LocalHostAddrString = LocalHostAddr->ToString(false);
			if (!LocalHostAddrString.IsEmpty() && !LocalHostAddrString.Equals(TEXT("127.0.0.1")))
			{
				return LocalHostAddrString;
			}
		}

		return TEXT("127.0.0.1");
	}

	FString UrlEncodeQueryValue(const FString& Value)
	{
		FTCHARToUTF8 Converted(*Value);
		FString Encoded;
		Encoded.Reserve(Value.Len() * 3);

		for (int32 Index = 0; Index < Converted.Length(); ++Index)
		{
			const uint8 Byte = static_cast<uint8>(Converted.Get()[Index]);
			const bool bIsUnreserved =
				(Byte >= 'A' && Byte <= 'Z') ||
				(Byte >= 'a' && Byte <= 'z') ||
				(Byte >= '0' && Byte <= '9') ||
				Byte == '-' ||
				Byte == '_' ||
				Byte == '.' ||
				Byte == '~';

			if (bIsUnreserved)
			{
				Encoded.AppendChar(static_cast<TCHAR>(Byte));
			}
			else
			{
				Encoded += FString::Printf(TEXT("%%%02X"), Byte);
			}
		}

		return Encoded;
	}

	FString StripWebUIOrderPrefix(const FString& RawLabel)
	{
		if (RawLabel.StartsWith(TEXT("WUI"), ESearchCase::IgnoreCase))
		{
			int32 PrefixEnd = 3;
			int32 DigitStart = PrefixEnd;
			while (PrefixEnd < RawLabel.Len() && FChar::IsDigit(RawLabel[PrefixEnd]))
			{
				++PrefixEnd;
			}

			if (PrefixEnd > DigitStart && PrefixEnd < RawLabel.Len() && RawLabel[PrefixEnd] == TEXT('_'))
			{
				const FString DisplayLabel = RawLabel.Mid(PrefixEnd + 1);
				if (!DisplayLabel.IsEmpty())
				{
					return DisplayLabel;
				}
			}
		}

		return RawLabel;
	}

	void AppendQueryParameter(FString& InURL, const FString& Key, const FString& Value)
	{
		if (Value.IsEmpty())
		{
			return;
		}

		InURL += InURL.Contains(TEXT("?")) ? TEXT("&") : TEXT("?");
		InURL += Key;
		InURL += TEXT("=");
		InURL += UrlEncodeQueryValue(Value);
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

	bool QRGetModule(const TArray<bool>& Modules, const int32 X, const int32 Y)
	{
		return Modules[Y * QRCodeSize + X];
	}

	void QRSetModule(TArray<bool>& Modules, TArray<bool>& FunctionModules, const int32 X, const int32 Y, const bool bBlack, const bool bFunction)
	{
		if (X < 0 || Y < 0 || X >= QRCodeSize || Y >= QRCodeSize)
		{
			return;
		}

		const int32 Index = Y * QRCodeSize + X;
		Modules[Index] = bBlack;
		if (bFunction)
		{
			FunctionModules[Index] = true;
		}
	}

	void QRDrawFinder(TArray<bool>& Modules, TArray<bool>& FunctionModules, const int32 Left, const int32 Top)
	{
		for (int32 DY = -1; DY <= 7; ++DY)
		{
			for (int32 DX = -1; DX <= 7; ++DX)
			{
				const bool bInside = DX >= 0 && DX <= 6 && DY >= 0 && DY <= 6;
				const bool bBorder = DX == 0 || DX == 6 || DY == 0 || DY == 6;
				const bool bCenter = DX >= 2 && DX <= 4 && DY >= 2 && DY <= 4;
				QRSetModule(Modules, FunctionModules, Left + DX, Top + DY, bInside && (bBorder || bCenter), true);
			}
		}
	}

	void QRDrawAlignment(TArray<bool>& Modules, TArray<bool>& FunctionModules, const int32 CenterX, const int32 CenterY)
	{
		for (int32 DY = -2; DY <= 2; ++DY)
		{
			for (int32 DX = -2; DX <= 2; ++DX)
			{
				const bool bBlack = FMath::Max(FMath::Abs(DX), FMath::Abs(DY)) == 2 || (DX == 0 && DY == 0);
				QRSetModule(Modules, FunctionModules, CenterX + DX, CenterY + DY, bBlack, true);
			}
		}
	}

	void QRAppendBits(TArray<bool>& Bits, const uint32 Value, const int32 Count)
	{
		for (int32 Index = Count - 1; Index >= 0; --Index)
		{
			Bits.Add(((Value >> Index) & 1U) != 0);
		}
	}

	void QRInitGalois(TArray<uint8>& Exp, TArray<uint8>& Log)
	{
		Exp.SetNumZeroed(512);
		Log.SetNumZeroed(256);

		int32 Value = 1;
		for (int32 Index = 0; Index < 255; ++Index)
		{
			Exp[Index] = static_cast<uint8>(Value);
			Log[Value] = static_cast<uint8>(Index);
			Value <<= 1;
			if ((Value & 0x100) != 0)
			{
				Value ^= 0x11D;
			}
		}

		for (int32 Index = 255; Index < Exp.Num(); ++Index)
		{
			Exp[Index] = Exp[Index - 255];
		}
	}

	uint8 QRGaloisMultiply(const uint8 X, const uint8 Y, const TArray<uint8>& Exp, const TArray<uint8>& Log)
	{
		if (X == 0 || Y == 0)
		{
			return 0;
		}
		return Exp[Log[X] + Log[Y]];
	}

	TArray<uint8> QRCreateGenerator(const int32 Degree, const TArray<uint8>& Exp, const TArray<uint8>& Log)
	{
		TArray<uint8> Generator;
		Generator.Init(0, Degree);
		Generator[Degree - 1] = 1;

		uint8 Root = 1;
		for (int32 Index = 0; Index < Degree; ++Index)
		{
			for (int32 CoeffIndex = 0; CoeffIndex < Degree; ++CoeffIndex)
			{
				Generator[CoeffIndex] = QRGaloisMultiply(Generator[CoeffIndex], Root, Exp, Log);
				if (CoeffIndex + 1 < Degree)
				{
					Generator[CoeffIndex] ^= Generator[CoeffIndex + 1];
				}
			}
			Root = QRGaloisMultiply(Root, 2, Exp, Log);
		}

		return Generator;
	}

	TArray<uint8> QRCreateErrorCorrection(const TArray<uint8>& DataCodewords)
	{
		TArray<uint8> Exp;
		TArray<uint8> Log;
		QRInitGalois(Exp, Log);
		const TArray<uint8> Generator = QRCreateGenerator(QRCodeEccCodewords, Exp, Log);

		TArray<uint8> Remainder;
		Remainder.Init(0, QRCodeEccCodewords);
		for (const uint8 DataCodeword : DataCodewords)
		{
			const uint8 Factor = DataCodeword ^ Remainder[0];
			for (int32 Index = 0; Index < QRCodeEccCodewords - 1; ++Index)
			{
				Remainder[Index] = Remainder[Index + 1];
			}
			Remainder[QRCodeEccCodewords - 1] = 0;

			for (int32 Index = 0; Index < QRCodeEccCodewords; ++Index)
			{
				Remainder[Index] ^= QRGaloisMultiply(Generator[Index], Factor, Exp, Log);
			}
		}

		return Remainder;
	}

	bool QRBuildCodewords(const FString& Text, TArray<uint8>& OutCodewords)
	{
		const FTCHARToUTF8 ConvertedText(*Text);
		const int32 ByteLength = ConvertedText.Length();
		if (ByteLength <= 0 || ByteLength > QRCodeMaxByteLength)
		{
			return false;
		}

		TArray<bool> Bits;
		QRAppendBits(Bits, 0x4, 4);
		QRAppendBits(Bits, static_cast<uint32>(ByteLength), 8);
		for (int32 Index = 0; Index < ByteLength; ++Index)
		{
			QRAppendBits(Bits, static_cast<uint8>(ConvertedText.Get()[Index]), 8);
		}

		const int32 DataBitCapacity = QRCodeDataCodewords * 8;
		QRAppendBits(Bits, 0, FMath::Min(4, DataBitCapacity - Bits.Num()));
		while ((Bits.Num() % 8) != 0)
		{
			Bits.Add(false);
		}

		TArray<uint8> DataCodewords;
		for (int32 BitIndex = 0; BitIndex < Bits.Num(); BitIndex += 8)
		{
			uint8 Codeword = 0;
			for (int32 Offset = 0; Offset < 8; ++Offset)
			{
				Codeword = static_cast<uint8>((Codeword << 1) | (Bits[BitIndex + Offset] ? 1 : 0));
			}
			DataCodewords.Add(Codeword);
		}

		for (uint8 PadCodeword = 0xEC; DataCodewords.Num() < QRCodeDataCodewords; PadCodeword ^= 0xEC ^ 0x11)
		{
			DataCodewords.Add(PadCodeword);
		}

		OutCodewords = DataCodewords;
		OutCodewords.Append(QRCreateErrorCorrection(DataCodewords));
		return true;
	}

	void QRDrawFormatBits(TArray<bool>& Modules, TArray<bool>& FunctionModules)
	{
		auto GetFormatBit = [](const int32 Index)
		{
			return ((QRCodeFormatLMask0 >> Index) & 1) != 0;
		};

		for (int32 Index = 0; Index <= 5; ++Index)
		{
			QRSetModule(Modules, FunctionModules, 8, Index, GetFormatBit(Index), true);
		}
		QRSetModule(Modules, FunctionModules, 8, 7, GetFormatBit(6), true);
		QRSetModule(Modules, FunctionModules, 8, 8, GetFormatBit(7), true);
		QRSetModule(Modules, FunctionModules, 7, 8, GetFormatBit(8), true);
		for (int32 Index = 9; Index < 15; ++Index)
		{
			QRSetModule(Modules, FunctionModules, 14 - Index, 8, GetFormatBit(Index), true);
		}

		for (int32 Index = 0; Index < 8; ++Index)
		{
			QRSetModule(Modules, FunctionModules, QRCodeSize - 1 - Index, 8, GetFormatBit(Index), true);
		}
		for (int32 Index = 8; Index < 15; ++Index)
		{
			QRSetModule(Modules, FunctionModules, 8, QRCodeSize - 15 + Index, GetFormatBit(Index), true);
		}
		QRSetModule(Modules, FunctionModules, 8, QRCodeSize - 8, true, true);
	}

	void QRDrawFunctionPatterns(TArray<bool>& Modules, TArray<bool>& FunctionModules)
	{
		QRDrawFinder(Modules, FunctionModules, 0, 0);
		QRDrawFinder(Modules, FunctionModules, QRCodeSize - 7, 0);
		QRDrawFinder(Modules, FunctionModules, 0, QRCodeSize - 7);
		QRDrawAlignment(Modules, FunctionModules, QRCodeSize - 7, QRCodeSize - 7);

		for (int32 Index = 8; Index < QRCodeSize - 8; ++Index)
		{
			const bool bBlack = (Index % 2) == 0;
			QRSetModule(Modules, FunctionModules, 6, Index, bBlack, true);
			QRSetModule(Modules, FunctionModules, Index, 6, bBlack, true);
		}

		QRDrawFormatBits(Modules, FunctionModules);
	}

	bool QRMask0(const int32 X, const int32 Y)
	{
		return ((X + Y) % 2) == 0;
	}

	void QRDrawCodewords(TArray<bool>& Modules, const TArray<bool>& FunctionModules, const TArray<uint8>& Codewords)
	{
		TArray<bool> Bits;
		for (const uint8 Codeword : Codewords)
		{
			QRAppendBits(Bits, Codeword, 8);
		}

		int32 BitIndex = 0;
		int32 Direction = -1;
		for (int32 Right = QRCodeSize - 1; Right >= 1; Right -= 2)
		{
			if (Right == 6)
			{
				Right = 5;
			}

			for (int32 VerticalIndex = 0; VerticalIndex < QRCodeSize; ++VerticalIndex)
			{
				const int32 Y = Direction == 1 ? VerticalIndex : QRCodeSize - 1 - VerticalIndex;
				for (int32 XOffset = 0; XOffset < 2; ++XOffset)
				{
					const int32 X = Right - XOffset;
					if (FunctionModules[Y * QRCodeSize + X])
					{
						continue;
					}

					bool bBlack = BitIndex < Bits.Num() ? Bits[BitIndex] : false;
					++BitIndex;
					if (QRMask0(X, Y))
					{
						bBlack = !bBlack;
					}
					Modules[Y * QRCodeSize + X] = bBlack;
				}
			}
			Direction = -Direction;
		}
	}

	UTexture2D* CreateQRCodeTextureFromText(const UObject* Outer, const FString& Text, const int32 PixelsPerModule, const int32 QuietZoneModules)
	{
		TArray<uint8> Codewords;
		if (!QRBuildCodewords(Text, Codewords))
		{
			UE_LOG(LogWebUIRuntime, Warning, TEXT("Failed to create QR code texture. URL is empty or longer than %d bytes: %s"), QRCodeMaxByteLength, *Text);
			return nullptr;
		}

		TArray<bool> Modules;
		TArray<bool> FunctionModules;
		Modules.Init(false, QRCodeSize * QRCodeSize);
		FunctionModules.Init(false, QRCodeSize * QRCodeSize);
		QRDrawFunctionPatterns(Modules, FunctionModules);
		QRDrawCodewords(Modules, FunctionModules, Codewords);

		const int32 ClampedPixelsPerModule = FMath::Max(1, PixelsPerModule);
		const int32 ClampedQuietZoneModules = FMath::Max(0, QuietZoneModules);
		const int32 OutputModules = QRCodeSize + ClampedQuietZoneModules * 2;
		const int32 TextureSize = OutputModules * ClampedPixelsPerModule;
		UTexture2D* Texture = UTexture2D::CreateTransient(TextureSize, TextureSize, PF_B8G8R8A8, NAME_None);
		if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() <= 0)
		{
			return nullptr;
		}

		Texture->SRGB = false;
		Texture->NeverStream = true;

		TArray<FColor> Pixels;
		Pixels.Init(FColor::White, TextureSize * TextureSize);
		for (int32 Y = 0; Y < QRCodeSize; ++Y)
		{
			for (int32 X = 0; X < QRCodeSize; ++X)
			{
				if (!QRGetModule(Modules, X, Y))
				{
					continue;
				}

				const int32 StartX = (X + ClampedQuietZoneModules) * ClampedPixelsPerModule;
				const int32 StartY = (Y + ClampedQuietZoneModules) * ClampedPixelsPerModule;
				for (int32 PixelY = 0; PixelY < ClampedPixelsPerModule; ++PixelY)
				{
					for (int32 PixelX = 0; PixelX < ClampedPixelsPerModule; ++PixelX)
					{
						Pixels[(StartY + PixelY) * TextureSize + StartX + PixelX] = FColor::Black;
					}
				}
			}
		}

		void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(TextureData, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
		Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
		Texture->UpdateResource();
		Texture->Rename(nullptr, const_cast<UObject*>(Outer));
		return Texture;
	}
}

UWebUIHostComponent::UWebUIHostComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWebUIHostComponent::PostInitProperties()
{
	Super::PostInitProperties();
}

void UWebUIHostComponent::PostLoad()
{
	Super::PostLoad();
}

void UWebUIHostComponent::OnRegister()
{
	if (AActor* Owner = GetOwner())
	{
		if (!Owner->IsTemplate())
		{
			TInlineComponentArray<UWebUIHostComponent*> HostComponents;
			Owner->GetComponents(HostComponents);
			if (HostComponents.Num() > 1 && HostComponents[0] != this)
			{
				UE_LOG(LogWebUIRuntime, Warning, TEXT("Duplicate WebUIHostComponent on '%s' was removed."), *Owner->GetName());
				DestroyComponent();
				return;
			}
		}
	}

	Super::OnRegister();
}

void UWebUIHostComponent::BeginPlay()
{
	Super::BeginPlay();

	if (WebUIId.IsEmpty() && GetOwner())
	{
		WebUIId = GetOwner()->GetName();
	}

	if (UWebUIRuntimeSubsystem* Runtime = GetRuntimeSubsystem())
	{
		Runtime->RegisterHost(this);
		if (IsAutoSaveChangedValuesEnabled() && ShouldAutoLoadSavedValues())
		{
			Runtime->LoadPersistedState(this);
		}
		if (bAutoStartServer)
		{
			Runtime->StartServerFromSettings();
		}
	}
}

void UWebUIHostComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWebUIRuntimeSubsystem* Runtime = GetRuntimeSubsystem())
	{
		if (IsAutoSaveChangedValuesEnabled())
		{
			Runtime->SavePersistedState(this);
		}
		Runtime->UnregisterHost(this);
	}

	Super::EndPlay(EndPlayReason);
}

bool UWebUIHostComponent::StartWebUIServer()
{
	if (UWebUIRuntimeSubsystem* Runtime = GetRuntimeSubsystem())
	{
		Runtime->RegisterHost(this);
		if (IsAutoSaveChangedValuesEnabled() && ShouldAutoLoadSavedValues())
		{
			Runtime->LoadPersistedState(this);
		}
		return Runtime->StartServerFromSettings();
	}
	return false;
}

void UWebUIHostComponent::StopWebUIServer()
{
	if (UWebUIRuntimeSubsystem* Runtime = GetRuntimeSubsystem())
	{
		Runtime->StopServer();
	}
}

FString UWebUIHostComponent::GetWebUIId() const
{
	if (!WebUIId.IsEmpty())
	{
		return WebUIId;
	}
	return GetOwner() ? GetOwner()->GetName() : GetName();
}

int32 UWebUIHostComponent::GetWebUIPort() const
{
	return GetDefault<UWebUIRuntimeSettings>()->Port;
}

FString UWebUIHostComponent::GetDescription() const
{
	if (!Description.IsEmpty())
	{
		return Description;
	}
	return GetOwner() ? GetOwner()->GetName() : GetName();
}

FString UWebUIHostComponent::GetBrowserURL() const
{
	if (!GetOwner())
	{
		return FString();
	}

	int32 Port = 0;
	if (const UWorld* World = GetWorld())
	{
		if (UWebUIRuntimeSubsystem* Runtime = World->GetSubsystem<UWebUIRuntimeSubsystem>())
		{
			Port = Runtime->GetServerPort();
		}
	}

	if (Port <= 0)
	{
		Port = GetDefault<UWebUIRuntimeSettings>()->Port;
	}

	const bool bAllowRemote = GetDefault<UWebUIRuntimeSettings>()->bAllowRemoteAccess;
	const FString Host = bAllowRemote ? GetLocalIPAddressString() : TEXT("localhost");

	return FString::Printf(TEXT("http://%s:%d/webui?webuiId=%s"), *Host, Port, *GetWebUIId());
}

FString UWebUIHostComponent::GetEmbeddedURL() const
{
	const FString BrowserURL = GetBrowserURL();
	if (BrowserURL.IsEmpty())
	{
		return BrowserURL;
	}

	return BrowserURL + TEXT("&embed=1");
}

FString UWebUIHostComponent::BuildControlTokenQRCodeURL(const bool bEmbed) const
{
	if (!GetOwner())
	{
		return FString();
	}

	int32 Port = 0;
	if (const UWorld* World = GetWorld())
	{
		if (const UWebUIRuntimeSubsystem* Runtime = World->GetSubsystem<UWebUIRuntimeSubsystem>())
		{
			Port = Runtime->GetServerPort();
		}
	}

	if (Port <= 0)
	{
		Port = GetDefault<UWebUIRuntimeSettings>()->Port;
	}

	const UWebUIRuntimeSettings* Settings = GetDefault<UWebUIRuntimeSettings>();
	const bool bAllowRemote = Settings && Settings->bAllowRemoteAccess;
	const FString Host = bAllowRemote ? GetLocalIPAddressString() : TEXT("localhost");
	UWebUIRuntimeSubsystem* Runtime = GetRuntimeSubsystem();
	if (!Runtime)
	{
		return FString();
	}

	const FString Token = Runtime->GetMobileControlToken();
	if (Token.IsEmpty())
	{
		return FString();
	}

	FString URL = FString::Printf(TEXT("http://%s:%d/webui"), *Host, Port);
	AppendQueryParameter(URL, TEXT("i"), GetWebUIId());
	if (bEmbed)
	{
		AppendQueryParameter(URL, TEXT("e"), TEXT("1"));
	}
	AppendQueryParameter(URL, TEXT("t"), Token);
	return URL;
}

UTexture2D* UWebUIHostComponent::CreateURLQRCodeTexture(const FString& URL, const int32 PixelsPerModule, const int32 QuietZoneModules) const
{
	return CreateQRCodeTextureFromText(this, URL, PixelsPerModule, QuietZoneModules);
}

UTexture2D* UWebUIHostComponent::CreateBrowserURLQRCodeTexture(const int32 PixelsPerModule, const int32 QuietZoneModules) const
{
	return CreateURLQRCodeTexture(GetBrowserURL(), PixelsPerModule, QuietZoneModules);
}

UTexture2D* UWebUIHostComponent::CreateEmbeddedURLQRCodeTexture(const int32 PixelsPerModule, const int32 QuietZoneModules) const
{
	return CreateURLQRCodeTexture(GetEmbeddedURL(), PixelsPerModule, QuietZoneModules);
}

UTexture2D* UWebUIHostComponent::CreateBrowserURLQRCodeTextureWithControlToken(const int32 PixelsPerModule, const int32 QuietZoneModules) const
{
	return CreateURLQRCodeTexture(BuildControlTokenQRCodeURL(false), PixelsPerModule, QuietZoneModules);
}

UTexture2D* UWebUIHostComponent::CreateEmbeddedURLQRCodeTextureWithControlToken(const int32 PixelsPerModule, const int32 QuietZoneModules) const
{
	return CreateURLQRCodeTexture(BuildControlTokenQRCodeURL(true), PixelsPerModule, QuietZoneModules);
}

void UWebUIHostComponent::RegisterWebUIButton(FName ButtonId)
{
	if (!ButtonId.IsNone())
	{
		WebUIButtons.AddUnique(ButtonId);
	}
}

void UWebUIHostComponent::UnregisterWebUIButton(FName ButtonId)
{
	ButtonId = ResolveWebUIButtonId(WebUIButtons, ButtonId);
	if (ButtonId.IsNone())
	{
		return;
	}

	WebUIButtons.Remove(ButtonId);
	WebUIButtonEnabledStates.Remove(ButtonId);
}

void UWebUIHostComponent::ClearWebUIButtons()
{
	WebUIButtons.Reset();
	WebUIButtonEnabledStates.Reset();
}

void UWebUIHostComponent::SetWebUIButtonEnabled(FName ButtonId, bool bEnabled)
{
	const FName RequestedButtonId = ButtonId;
	ButtonId = ResolveWebUIButtonId(WebUIButtons, RequestedButtonId);
	if (ButtonId.IsNone())
	{
		ButtonId = ResolveWebUIButtonFunctionId(GetOwner(), RequestedButtonId);
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

bool UWebUIHostComponent::IsWebUIButtonEnabled(FName ButtonId) const
{
	const FName RequestedButtonId = ButtonId;
	ButtonId = ResolveWebUIButtonId(WebUIButtons, RequestedButtonId);
	if (ButtonId.IsNone())
	{
		ButtonId = ResolveWebUIButtonFunctionId(GetOwner(), RequestedButtonId);
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

const TArray<FName>& UWebUIHostComponent::GetWebUIButtons() const
{
	return WebUIButtons;
}

void UWebUIHostComponent::NotifyWebUIPropertyChanged(FName PropertyName)
{
	OnWebUIPropertyChanged.Broadcast(FName(*StripWebUIOrderPrefix(PropertyName.ToString())));
}

void UWebUIHostComponent::NotifyWebUIBoolChanged(FName PropertyName, bool Value)
{
	OnWebUIBoolChanged.Broadcast(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIHostComponent::NotifyWebUIFloatChanged(FName PropertyName, double Value)
{
	OnWebUIFloatChanged.Broadcast(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIHostComponent::NotifyWebUIStringChanged(FName PropertyName, const FString& Value)
{
	OnWebUIStringChanged.Broadcast(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIHostComponent::NotifyWebUIVectorChanged(FName PropertyName, FVector Value)
{
	OnWebUIVectorChanged.Broadcast(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIHostComponent::NotifyWebUIRotatorChanged(FName PropertyName, FRotator Value)
{
	OnWebUIRotatorChanged.Broadcast(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIHostComponent::NotifyWebUIColorChanged(FName PropertyName, FLinearColor Value)
{
	OnWebUIColorChanged.Broadcast(FName(*StripWebUIOrderPrefix(PropertyName.ToString())), Value);
}

void UWebUIHostComponent::NotifyWebUIButtonClicked(FName ButtonId)
{
	OnWebUIButtonClicked.Broadcast(ButtonId);
	K2_OnWebUIButtonClicked(ButtonId);
	if (AWebUIHostActor* HostActor = Cast<AWebUIHostActor>(GetOwner()))
	{
		HostActor->NotifyWebUIButtonClicked(ButtonId);
	}
}

bool UWebUIHostComponent::IsAutoSaveChangedValuesEnabled() const
{
	return bAutoSaveChangedValues;
}

bool UWebUIHostComponent::ShouldAutoLoadSavedValues() const
{
	return bAutoLoadSavedValues;
}

UWebUIRuntimeSubsystem* UWebUIHostComponent::GetRuntimeSubsystem() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UWebUIRuntimeSubsystem>() : nullptr;
}

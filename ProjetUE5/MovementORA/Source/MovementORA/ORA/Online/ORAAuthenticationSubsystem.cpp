#include "ORA/Online/ORAAuthenticationSubsystem.h"

#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "GenericPlatform/GenericPlatformProperties.h"
#include "HttpModule.h"
#include "JsonObjectConverter.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Base64.h"
#include "ORA/Online/ORALocalAuthSaveGame.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <wincrypt.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

namespace
{
	bool ProtectLocalSession(const FString& PlainText, FString& ProtectedText)
	{
#if PLATFORM_WINDOWS
		FTCHARToUTF8 Utf8(*PlainText);
		DATA_BLOB Input{static_cast<DWORD>(Utf8.Length()), reinterpret_cast<BYTE*>(const_cast<ANSICHAR*>(Utf8.Get()))};
		DATA_BLOB Output{};
		if (!CryptProtectData(&Input, L"ORA guest session", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &Output))
		{
			return false;
		}
		ProtectedText = FBase64::Encode(Output.pbData, static_cast<uint32>(Output.cbData));
		LocalFree(Output.pbData);
		return true;
#else
		return false;
#endif
	}

	bool UnprotectLocalSession(const FString& ProtectedText, FString& PlainText)
	{
#if PLATFORM_WINDOWS
		TArray<uint8> Encoded;
		if (!FBase64::Decode(ProtectedText, Encoded) || Encoded.IsEmpty())
		{
			return false;
		}
		DATA_BLOB Input{static_cast<DWORD>(Encoded.Num()), Encoded.GetData()};
		DATA_BLOB Output{};
		if (!CryptUnprotectData(&Input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &Output))
		{
			return false;
		}
		TArray<ANSICHAR> Decoded;
		Decoded.Append(reinterpret_cast<ANSICHAR*>(Output.pbData), static_cast<int32>(Output.cbData));
		Decoded.Add('\0');
		PlainText = UTF8_TO_TCHAR(Decoded.GetData());
		LocalFree(Output.pbData);
		return true;
#else
		return false;
#endif
	}
}

void UORAAuthenticationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (!IsRunningDedicatedServer())
	{
		StartAuthentication();
	}
}

void UORAAuthenticationSubsystem::StartAuthentication()
{
	if (bRequestInFlight || IsRunningDedicatedServer())
	{
		return;
	}

	if (LoadSession())
	{
		RequestProfile();
	}
	else
	{
		CreateNewGuestAccount();
	}
}

void UORAAuthenticationSubsystem::RetryAuthentication()
{
	if (AuthenticationState == EORAAuthenticationState::RequiresNewGuest)
	{
		SetState(EORAAuthenticationState::RequiresNewGuest,
			TEXT("La session invitée a expiré. Créez un nouveau compte uniquement si l'ancien compte ne doit pas être récupéré."));
		return;
	}
	StartAuthentication();
}

void UORAAuthenticationSubsystem::CreateNewGuestAccount(const FString& RequestedDisplayName)
{
	if (bRequestInFlight || IsRunningDedicatedServer())
	{
		return;
	}

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	if (!RequestedDisplayName.IsEmpty())
	{
		Body->SetStringField(TEXT("displayName"), RequestedDisplayName);
	}
	FString SerializedBody;
	FJsonSerializer::Serialize(Body, TJsonWriterFactory<>::Create(&SerializedBody));

	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(ApiBaseUrl / TEXT("v1/auth/guest"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	Request->SetContentAsString(SerializedBody);
	Request->OnProcessRequestComplete().BindUObject(this, &UORAAuthenticationSubsystem::HandleGuestCreated);
	bRequestInFlight = true;
	SetState(EORAAuthenticationState::Connecting);
	if (!Request->ProcessRequest())
	{
		bRequestInFlight = false;
		SetState(EORAAuthenticationState::Failed, TEXT("Impossible de lancer la création du compte invité."));
	}
}

void UORAAuthenticationSubsystem::RequestProfile()
{
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(ApiBaseUrl / TEXT("v1/profile"));
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AccessToken));
	Request->OnProcessRequestComplete().BindUObject(this, &UORAAuthenticationSubsystem::HandleProfileReceived);
	bRequestInFlight = true;
	SetState(EORAAuthenticationState::Connecting);
	if (!Request->ProcessRequest())
	{
		bRequestInFlight = false;
		SetState(EORAAuthenticationState::Failed, TEXT("Impossible de contacter le service de profil ORA."));
	}
}

void UORAAuthenticationSubsystem::HandleGuestCreated(FHttpRequestPtr, FHttpResponsePtr Response, const bool bSucceeded)
{
	bRequestInFlight = false;
	if (!bSucceeded || !Response.IsValid() || Response->GetResponseCode() < 200 || Response->GetResponseCode() >= 300)
	{
		SetState(EORAAuthenticationState::Failed, TEXT("La création du compte invité a échoué."));
		return;
	}

	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response->GetContentAsString()), Root) || !Root.IsValid())
	{
		SetState(EORAAuthenticationState::Failed, TEXT("Réponse d'authentification invalide."));
		return;
	}

	const TSharedPtr<FJsonObject>* Player = nullptr;
	if (!Root->TryGetStringField(TEXT("accessToken"), AccessToken) ||
		!Root->TryGetObjectField(TEXT("player"), Player) || !Player ||
		!(*Player)->TryGetStringField(TEXT("id"), PlayerId))
	{
		SetState(EORAAuthenticationState::Failed, TEXT("La réponse ne contient pas la session ORA attendue."));
		return;
	}

	(*Player)->TryGetStringField(TEXT("display_name"), DisplayName);
	if (!SaveSession())
	{
		SetState(EORAAuthenticationState::Failed, TEXT("Le compte existe, mais sa session n'a pas pu être enregistrée localement."));
		return;
	}
	RequestProfile();
}

void UORAAuthenticationSubsystem::HandleProfileReceived(FHttpRequestPtr, FHttpResponsePtr Response, const bool bSucceeded)
{
	bRequestInFlight = false;
	if (Response.IsValid() && Response->GetResponseCode() == 401)
	{
		ClearSession();
		SetState(EORAAuthenticationState::RequiresNewGuest,
			TEXT("La session invitée n'est plus valide. Une récupération doit être proposée avant de remplacer le compte."));
		return;
	}
	if (!bSucceeded || !Response.IsValid() || Response->GetResponseCode() < 200 || Response->GetResponseCode() >= 300)
	{
		SetState(EORAAuthenticationState::Failed, TEXT("Le profil ORA est temporairement indisponible."));
		return;
	}

	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response->GetContentAsString()), Root) || !Root.IsValid() ||
		!Root->TryGetStringField(TEXT("id"), PlayerId))
	{
		SetState(EORAAuthenticationState::Failed, TEXT("Réponse de profil invalide."));
		return;
	}
	Root->TryGetStringField(TEXT("display_name"), DisplayName);
	SetState(EORAAuthenticationState::Authenticated);
}

void UORAAuthenticationSubsystem::SetState(const EORAAuthenticationState NewState, const FString& Error)
{
	AuthenticationState = NewState;
	LastError = Error;
	OnAuthenticationStateChanged.Broadcast(NewState);
	UE_LOG(LogTemp, Display, TEXT("[ORA Auth] State=%d%s"), static_cast<int32>(NewState),
		Error.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" Error=%s"), *Error));
}

bool UORAAuthenticationSubsystem::LoadSession()
{
	const UORALocalAuthSaveGame* Save = Cast<UORALocalAuthSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlot, 0));
	FString PlainText;
	if (!Save || Save->ProtectedSession.IsEmpty() || !UnprotectLocalSession(Save->ProtectedSession, PlainText))
	{
		return false;
	}
	return PlainText.Split(TEXT("\n"), &PlayerId, &AccessToken) && !PlayerId.IsEmpty() && !AccessToken.IsEmpty();
}

bool UORAAuthenticationSubsystem::SaveSession() const
{
	UORALocalAuthSaveGame* Save = Cast<UORALocalAuthSaveGame>(UGameplayStatics::CreateSaveGameObject(UORALocalAuthSaveGame::StaticClass()));
	if (!Save)
	{
		return false;
	}
	if (!ProtectLocalSession(PlayerId + TEXT("\n") + AccessToken, Save->ProtectedSession))
	{
		return false;
	}
	return UGameplayStatics::SaveGameToSlot(Save, SaveSlot, 0);
}

void UORAAuthenticationSubsystem::ClearSession()
{
	AccessToken.Reset();
	PlayerId.Reset();
	UGameplayStatics::DeleteGameInSlot(SaveSlot, 0);
}

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "ORAAuthenticationSubsystem.generated.h"

UENUM(BlueprintType)
enum class EORAAuthenticationState : uint8
{
	NotStarted,
	Connecting,
	Authenticated,
	RequiresNewGuest,
	Failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FORAAuthenticationStateChanged, EORAAuthenticationState, NewState);

UCLASS(Config=Game)
class MOVEMENTORA_API UORAAuthenticationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintAssignable, Category="ORA|Authentication")
	FORAAuthenticationStateChanged OnAuthenticationStateChanged;

	UFUNCTION(BlueprintCallable, Category="ORA|Authentication")
	void StartAuthentication();

	UFUNCTION(BlueprintCallable, Category="ORA|Authentication")
	void CreateNewGuestAccount(const FString& DisplayName = TEXT(""));

	UFUNCTION(BlueprintCallable, Category="ORA|Authentication")
	void RetryAuthentication();

	UFUNCTION(BlueprintPure, Category="ORA|Authentication")
	EORAAuthenticationState GetAuthenticationState() const { return AuthenticationState; }

	UFUNCTION(BlueprintPure, Category="ORA|Authentication")
	bool IsAuthenticated() const { return AuthenticationState == EORAAuthenticationState::Authenticated; }

	UFUNCTION(BlueprintPure, Category="ORA|Authentication")
	FString GetPlayerId() const { return PlayerId; }

	UFUNCTION(BlueprintPure, Category="ORA|Authentication")
	FString GetDisplayName() const { return DisplayName; }

	UFUNCTION(BlueprintPure, Category="ORA|Authentication")
	FString GetLastError() const { return LastError; }

private:
	void RequestProfile();
	void HandleGuestCreated(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSucceeded);
	void HandleProfileReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSucceeded);
	void SetState(EORAAuthenticationState NewState, const FString& Error = TEXT(""));
	bool LoadSession();
	bool SaveSession() const;
	void ClearSession();

	UPROPERTY(Config)
	FString ApiBaseUrl = TEXT("https://api.oragame.eu");

	UPROPERTY()
	EORAAuthenticationState AuthenticationState = EORAAuthenticationState::NotStarted;

	UPROPERTY()
	FString AccessToken;

	UPROPERTY()
	FString PlayerId;

	UPROPERTY()
	FString DisplayName;

	UPROPERTY()
	FString LastError;

	bool bRequestInFlight = false;
	static constexpr const TCHAR* SaveSlot = TEXT("ORA_Authentication");
};


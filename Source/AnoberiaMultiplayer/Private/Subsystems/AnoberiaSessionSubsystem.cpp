


#include "Subsystems/AnoberiaSessionSubsystem.h"

#include "AnoberiaMultiplayer.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "GameFramework/GameModeBase.h"

UAnoberiaSessionSubsystem::UAnoberiaSessionSubsystem()
{
	OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();
		if(SessionInterface)
			SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &UAnoberiaSessionSubsystem::OnSessionInviteAccepted));
	}
	else
		UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("OnlineSubsystem is nullptr on %s"), *FString(__FUNCTION__))
	
	//Network failure
	if (GetGameInstance() && GetGameInstance()->GetEngine())
		GetGameInstance()->GetEngine()->OnNetworkFailure().AddUObject(this, &UAnoberiaSessionSubsystem::OnNetworkFailure);
	else
		UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("GEngine is not valid in %s"), *FString(__FUNCTION__))
}

void UAnoberiaSessionSubsystem::SetMatchStarted(bool bNewStarted)
{
	if (SessionInterface)
	{
		FOnlineSessionSettings* NewSessionSettings = SessionInterface->GetSessionSettings(NAME_GameSession);
		const int32 MatchStartedValue = bNewStarted ? 1 : 0;
		if (NewSessionSettings)
		{
			NewSessionSettings->Set(FName("HasMatchStarted"), MatchStartedValue, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			SessionInterface->UpdateSession(NAME_GameSession, *NewSessionSettings);
		}

		UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("Updating match has started: %i"), bNewStarted)
	}
}

void UAnoberiaSessionSubsystem::CreateSession(int32 PlayerCount, bool bPrivateSession)
{
	if (SessionInterface)
	{
		PlayerCount++; //For advertising, 
		LastNumPublicConnections = PlayerCount;

		if (auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession))
		{
			UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("Found existing session, Destroying.."))
			bCreateSessionOnDestroy = true;
			DestroySession();
		}
		else
		{
			UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("Creating session for %i players, Private: %i"), PlayerCount, bPrivateSession)
			
			SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &UAnoberiaSessionSubsystem::OnCreateSessionComplete));

			SessionSettings = MakeShareable(new FOnlineSessionSettings());
			if (!SessionSettings.Get()) return;
			SessionSettings->bIsLANMatch = IsLANGame();
			SessionSettings->NumPublicConnections = PlayerCount;
			SessionSettings->bAllowJoinInProgress = true;
			SessionSettings->bAllowJoinViaPresence = true;
			SessionSettings->bShouldAdvertise = true;
			SessionSettings->bUsesPresence = true;
			SessionSettings->bUseLobbiesIfAvailable = true;
			SessionSettings->Set(FName("HasMatchStarted"), 0, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			SessionSettings->Set(FName("PrivateSession"), bPrivateSession, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

			if (GetWorld())
			{
				if (const ULocalPlayer* SessionLocalPlayer = GetWorld()->GetFirstLocalPlayerFromController())
				{
					if (!SessionInterface->CreateSession(*SessionLocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings))
					{
						//Failed to create
						SessionInterface->ClearOnCreateSessionCompleteDelegates(this);
						AnoberiaOnCreateSessionComplete.Broadcast(false);
					}
				}
			}
		}		
	}
}

void UAnoberiaSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegates(this);
		AnoberiaOnCreateSessionComplete.Broadcast(bWasSuccessful);
	}
}

void UAnoberiaSessionSubsystem::FindSessions()
{
	if (SessionInterface)
	{
		SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &UAnoberiaSessionSubsystem::OnFindSessionsComplete));

		SessionSearch = MakeShareable(new FOnlineSessionSearch());
		if (!SessionSearch.Get()) return;

		SessionSearch->MaxSearchResults = 1000;
		SessionSearch->bIsLanQuery = IsLANGame();
		SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
		SessionSearch->QuerySettings.Set(FName("HasMatchStarted"), 3, EOnlineComparisonOp::LessThan); //Show all sessions (1-started, 0-didnt started)

		if (GetWorld())
		{
			if (const ULocalPlayer* SessionLocalPlayer = GetWorld()->GetFirstLocalPlayerFromController())
			{
				if (!SessionInterface->FindSessions(*SessionLocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef()))
				{
					//Failed to find
					SessionInterface->ClearOnFindSessionsCompleteDelegates(this);
					AnoberiaOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
				}
			}
		}
	}
}

void UAnoberiaSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("OnFindSessions %i , found %i session in %s"), bWasSuccessful, SessionSearch->SearchResults.Num(), *FString(__FUNCTION__))
	
	if (SessionInterface)
		SessionInterface->ClearOnFindSessionsCompleteDelegates(this);

	if (!SessionSearch.Get()) return;

	AnoberiaOnFindSessionsComplete.Broadcast(SessionSearch->SearchResults, bWasSuccessful);
}

void UAnoberiaSessionSubsystem::DestroySession()
{
	if (SessionInterface)
	{
		SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &UAnoberiaSessionSubsystem::OnDestroySessionComplete));

		UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("Destroying session.."))

		if (!SessionInterface->DestroySession(NAME_GameSession))
		{
			SessionInterface->ClearOnDestroySessionCompleteDelegates(this);
			AnoberiaOnDestroySessionComplete.Broadcast(false);
		}
	}
	else
		AnoberiaOnDestroySessionComplete.Broadcast(false);
}

void UAnoberiaSessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
		SessionInterface->ClearOnDestroySessionCompleteDelegates(this);

	UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("Destroy Session completed %i in %s"), bWasSuccessful, *FString(__FUNCTION__))
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("Creating session again.."))
		bCreateSessionOnDestroy = false;
		CreateSession(--LastNumPublicConnections); //CreatingSession increments Count
	}

	AnoberiaOnDestroySessionComplete.Broadcast(bWasSuccessful);
}

void UAnoberiaSessionSubsystem::JoinFoundedSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (SessionInterface)
	{
		SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &UAnoberiaSessionSubsystem::OnJoinSessionComplete));

		const ULocalPlayer* SessionLocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
		if (!SessionInterface->JoinSession(*SessionLocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
		{
			SessionInterface->ClearOnJoinSessionCompleteDelegates(this);
			AnoberiaOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		}
	}
	else
		AnoberiaOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
}

void UAnoberiaSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegates(this);
		AnoberiaOnJoinSessionComplete.Broadcast(Result);
	}
}

int32 UAnoberiaSessionSubsystem::GetPublicConnections() const
{
	if (SessionInterface)
	{
		if (FOnlineSessionSettings* CurrentSessionSettings = SessionInterface->GetSessionSettings(NAME_GameSession))
			return CurrentSessionSettings->NumPublicConnections;
	}
	return 3;
}

void UAnoberiaSessionSubsystem::DisconnectClient(const FUniqueNetId& NetId)
{
	if (SessionInterface)
		SessionInterface->UnregisterPlayer(NAME_GameSession, NetId);
}

bool UAnoberiaSessionSubsystem::IsLANGame() const
{
	if (OnlineSubsystem)
		return OnlineSubsystem->GetSubsystemName() == "NULL";
	else
		return true;
}

void UAnoberiaSessionSubsystem::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver,
	ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	//Wait 1 sec for broadcast to MainMenuHUD
	if (GetWorld())
	{
		TD_NetworkFailure = FTimerDelegate::CreateUFunction(this, "SendNetworkFailureMessage", ErrorString);
		GetWorld()->GetTimerManager().SetTimer(TH_NetworkFailure, TD_NetworkFailure, 1.f, false);

		if (GetWorld())
		{
			AGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AGameModeBase>();
			if (!GameMode) //Client
			{
				if (GetWorld()->GetFirstPlayerController())
					GetWorld()->GetFirstPlayerController()->ClientReturnToMainMenuWithTextReason(FText());
			}
		}
	}
}

void UAnoberiaSessionSubsystem::OnSessionInviteAccepted(const bool bWasSuccesful, const int32 LocalUserNum,
	FUniqueNetIdPtr FromId, const FOnlineSessionSearchResult& InviteResult)
{
	UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("Session Invite Accepted: %i"), bWasSuccesful);
	
	if (bWasSuccesful)
		JoinFoundedSession(InviteResult);

	if (SessionInterface)
		SessionInterface->ClearOnSessionUserInviteAcceptedDelegates(this);

	AnoberiaSessionInviteAcceptedDelegate.Broadcast(bWasSuccesful);
}

void UAnoberiaSessionSubsystem::SendNetworkFailureMessage(const FString& ErrorString)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TH_NetworkFailure);
		AnoberiaNetworkFailureDelegate.Broadcast(ErrorString);
	}
}

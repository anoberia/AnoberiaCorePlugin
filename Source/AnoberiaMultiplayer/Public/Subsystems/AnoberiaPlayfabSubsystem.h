

#pragma once

#include "CoreMinimal.h"
#include "PlayFab.h"
#include "Core/PlayFabClientDataModels.h"
#include "Core/PlayFabMultiplayerDataModels.h"
#include "Core/PlayFabError.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AnoberiaPlayfabSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAnoberiaOnMatchmakingStatus, const FString&, NewMatchmakingStatus);

/**
 * 
 */
UCLASS()
class ANOBERIAMULTIPLAYER_API UAnoberiaPlayfabSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	void StartMathmaking();
	void CancelMatchmakingTickets(bool bNewCreateNewTicket);

	FAnoberiaOnMatchmakingStatus AnoberiaOnMatchmakingStatusDelegate;

protected:

	void InitPlayFab();
	UFUNCTION()
	void OnGSDKShutdown();
	UFUNCTION()
	bool OnGSDKHealthCheck();
	UFUNCTION()
	void OnGSDKServerActive();
	UFUNCTION()
	void OnGSDKReadyForPlayers();

	void OnLoggedIn(const PlayFab::ClientModels::FLoginResult& Result);
	void OnMatchmakingTicketsCanceled(const PlayFab::MultiplayerModels::FCancelAllMatchmakingTicketsForPlayerResult& CancelTicketResult);
	void OnMatchmakingTicketCreated(const PlayFab::MultiplayerModels::FCreateMatchmakingTicketResult& TicketResult);
	void GetMatchmakingTicket(const PlayFab::MultiplayerModels::FGetMatchmakingTicketResult& TicketResult);
	void ClearMatchmakingTimer();
	void GetMatch(const PlayFab::MultiplayerModels::FGetMatchResult& MatchResult);
	void OnPlayFabError(const PlayFab::FPlayFabCppError& ErrorResult);
	void PollMatchmakingTicket();
	bool bCreateNewTicket;

	PlayFabClientPtr ClientAPI = nullptr;
	PlayFabMultiplayerPtr MultiplayerAPI = nullptr;
	PlayFabAuthenticationPtr AuthenticationAPI = nullptr;

	FTimerHandle CreateLobbyTimer;
	FString EntityToken;
	FTimerHandle TH_MatchmakingTicket;
	FString MMTicketId;
	PlayFab::ClientModels::FLoginResult LoginResult;
	
};

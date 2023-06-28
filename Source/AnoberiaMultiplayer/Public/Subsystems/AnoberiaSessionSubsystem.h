

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AnoberiaSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAnoberiaOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FAnoberiaOnFindSessionsComplete, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FAnoberiaOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAnoberiaOnDestroySessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAnoberiaSessionInviteAcceptedDelegate, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAnoberiaNetworkFailureDelegate, const FString&, ErrorString);

/**
 * 
 */
UCLASS()
class ANOBERIAMULTIPLAYER_API UAnoberiaSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	UAnoberiaSessionSubsystem();

public:

	void SetMatchStarted(bool bNewStarted);
	void CreateSession(int32 PlayerCount, bool bPrivateSession = false);
	void FindSessions();
	void DestroySession();
	void JoinFoundedSession(const FOnlineSessionSearchResult& SessionResult);
	int32 GetPublicConnections() const;
	void DisconnectClient(const FUniqueNetId& NetId);
	bool IsLANGame() const;

	FAnoberiaOnCreateSessionComplete AnoberiaOnCreateSessionComplete;
	FAnoberiaOnFindSessionsComplete AnoberiaOnFindSessionsComplete;
	FAnoberiaOnJoinSessionComplete AnoberiaOnJoinSessionComplete;
	FAnoberiaOnDestroySessionComplete AnoberiaOnDestroySessionComplete;
	FAnoberiaNetworkFailureDelegate AnoberiaNetworkFailureDelegate;
	FAnoberiaSessionInviteAcceptedDelegate AnoberiaSessionInviteAcceptedDelegate;

private:
	
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void OnSessionInviteAccepted(const bool bWasSuccesful, const int32 LocalUserNum, FUniqueNetIdPtr FromId, const FOnlineSessionSearchResult& InviteResult);
	
protected:

	class IOnlineSubsystem* OnlineSubsystem;
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> SessionSettings;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	bool bCreateSessionOnDestroy;
	int32 LastNumPublicConnections;

private:
	
	FTimerHandle TH_NetworkFailure;
	FTimerDelegate TD_NetworkFailure;
	UFUNCTION()
	void SendNetworkFailureMessage(const FString& ErrorString);
	
	
};

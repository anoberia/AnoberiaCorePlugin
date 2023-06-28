

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "AnoberiaFriendsSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FAnoberiaOnReadFriendsComplete, TArray<TSharedRef<FOnlineFriend>>& Friends);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAnoberiaOnSendInviteCompleted, bool, bWasSuccessful);

/**
 * 
 */
UCLASS()
class ANOBERIAMULTIPLAYER_API UAnoberiaFriendsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	UAnoberiaFriendsSubsystem();
	
public:

	void ReadFriends();
	void OnReadFriendsComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr);
	void InviteFriend(FUniqueNetIdRef FriendId);
	bool IsFriend(FUniqueNetIdPtr InId);
	void OnPresenceUpdated(const class FUniqueNetId& UserId, const TSharedRef<FOnlineUserPresence>& Presence);
	void OnSendInviteComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr);
	void InviteSent();

	FAnoberiaOnReadFriendsComplete AnoberiaOnReadFriendsCompleteDelegate;
	FAnoberiaOnSendInviteCompleted AnoberiaOnSendInviteCompletedDelegate;

protected:

	class IOnlineSubsystem* OnlineSubsystem;
	IOnlineFriendsPtr FriendsInterface;
	TArray<TSharedRef<FOnlineFriend>> Friends;
};

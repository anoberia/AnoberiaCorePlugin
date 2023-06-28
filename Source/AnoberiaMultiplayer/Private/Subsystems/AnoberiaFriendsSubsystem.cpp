


#include "Subsystems/AnoberiaFriendsSubsystem.h"

#include "AnoberiaMultiplayer.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Interfaces/OnlinePresenceInterface.h"

UAnoberiaFriendsSubsystem::UAnoberiaFriendsSubsystem()
{
	OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		FriendsInterface = OnlineSubsystem->GetFriendsInterface();
		
		if (FriendsInterface)
			FriendsInterface->AddOnOutgoingInviteSentDelegate_Handle(0, FOnOutgoingInviteSentDelegate::CreateUObject(this, &UAnoberiaFriendsSubsystem::InviteSent));
		
		if(IOnlinePresencePtr PresenceInterface = OnlineSubsystem->GetPresenceInterface())
			PresenceInterface->AddOnPresenceReceivedDelegate_Handle(FOnPresenceReceivedDelegate::CreateUObject(this, &UAnoberiaFriendsSubsystem::OnPresenceUpdated));
	}		
}

void UAnoberiaFriendsSubsystem::ReadFriends()
{
	//Default, InGamePlayers
	
	//Don't read friends in Game Map
	const FString MapName = UGameplayStatics::GetCurrentLevelName(this);
	if (MapName.Equals(TEXT("ColorMap"))) return;

	if (FriendsInterface)
		FriendsInterface->ReadFriendsList(0, "Default", FOnReadFriendsListComplete::CreateUObject(this, &UAnoberiaFriendsSubsystem::OnReadFriendsComplete));
}

void UAnoberiaFriendsSubsystem::OnReadFriendsComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName,
	const FString& ErrorStr)
{
	UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("OnReadFriendsCompleted %i"), bWasSuccessful)

	if (FriendsInterface)
	{
		Friends.Empty();
		FriendsInterface->GetFriendsList(0, "Default", Friends);

		TArray<TSharedRef<FOnlineFriend>> OnlineFriends;
		for (auto& Online : Friends) 
		{
			if (Online->GetPresence().bIsOnline) //Just send Online Friends
				OnlineFriends.AddUnique(Online);
		}

		//@TODO Reorder by presence (?)

		AnoberiaOnReadFriendsCompleteDelegate.Broadcast(OnlineFriends);  //@TODO OnlineFriends
	}
}

void UAnoberiaFriendsSubsystem::InviteFriend(FUniqueNetIdRef FriendId)
{
	if(OnlineSubsystem)
	{
		if(IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface())
		{
			bool bSendInvite = SessionInterface->SendSessionInviteToFriend(0, NAME_GameSession, *FriendId);
			UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("Invite Sent : %i"), bSendInvite);
		}
	}
}

bool UAnoberiaFriendsSubsystem::IsFriend(FUniqueNetIdPtr InId)
{
	/*for (auto& Friend : Friends)
	{	
		if (Friend == InId.ToSharedRef())
			return true;
	}
	return false;*/

	if (FriendsInterface)
		return FriendsInterface->IsFriend(0, *InId, TEXT("default"));
	else
		return false;
}

void UAnoberiaFriendsSubsystem::OnPresenceUpdated(const FUniqueNetId& UserId,
	const TSharedRef<FOnlineUserPresence>& Presence)
{
	UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("%s"), *FString(__FUNCTION__))
	ReadFriends();
}

void UAnoberiaFriendsSubsystem::OnSendInviteComplete(int32 LocalUserNum, bool bWasSuccessful,
	const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr)
{
	//From FriendsInterface->SendInvite
	UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("%s: %i. Error: %s"), *FString(__FUNCTION__), bWasSuccessful, *ErrorStr)
	AnoberiaOnSendInviteCompletedDelegate.Broadcast(bWasSuccessful);
}

void UAnoberiaFriendsSubsystem::InviteSent()
{
	UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("%s"), *FString(__FUNCTION__))
	AnoberiaOnSendInviteCompletedDelegate.Broadcast(true);
}

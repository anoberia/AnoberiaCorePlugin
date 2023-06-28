

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineLeaderboardInterface.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "AnoberiaLeaderboardsSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FLeaderboardPlayerInfo
{
	GENERATED_BODY()

	FString Nickname;
	int32 Rank;
	int32 Value;
	FString LeaderboardName;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAnoberiaOnLeaderboardReadCompleteSingle, const FLeaderboardPlayerInfo&, PlayerInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAnoberiaOnLeaderboardReadCompleteBest, TArray<FLeaderboardPlayerInfo>&, PlayerInfos);

/**
 * 
 */
UCLASS()
class ANOBERIAMULTIPLAYER_API UAnoberiaLeaderboardsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	UAnoberiaLeaderboardsSubsystem();

public:

	void ReadWriteLeaderboards(const FName& AchievementName, APlayerController* PlayerController, bool bNewSingleAchievement, 
	bool bNewIncreaseLeaderboardValue, int32 NewLeaderboardValue, bool bNewReadBestPlayers);
	void ResetAch();
	void ReadLeaderboardSingle(const FName& AchievementName, const FUniqueNetId& PlayerId);
	void ReadLeaderboardBest(const FName& AchievementName, const FUniqueNetId& PlayerId);
	
	FAnoberiaOnLeaderboardReadCompleteSingle OnLeaderboardReadCompleteSingleDelegate;
	FAnoberiaOnLeaderboardReadCompleteBest OnLeaderboardReadCompleteBestDelegate;

protected:

	IOnlineAchievementsPtr AchievementsInterface;
	IOnlineLeaderboardsPtr LeaderboardsInterface;

	void OnQueryAchievementsComplete(const FUniqueNetId& PlayerId, const bool bWasSuccessful);
	void OnLeaderboardReadCompleteSingle(bool bWasSuccessful);
	void OnLeaderboardReadCompleteBest(bool bWasSuccessful);
	void AchievementesWritten(const FUniqueNetId& PlayerId, bool bWasSuccessful);
	TSharedPtr<class FOnlineLeaderboardRead, ESPMode::ThreadSafe> ReadObject;
	bool bCanReadWriteAchievements = true;
	FName LocalAchievementName;
	UPROPERTY()
	const ULocalPlayer* LocalPlayer;
	bool bIncreaseLeaderboardValue;
	bool bSingleAchievement;
	bool bReadBestPlayers;
	int32 LeaderboardValue;
};

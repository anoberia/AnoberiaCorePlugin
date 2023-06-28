


#include "Subsystems/AnoberiaLeaderboardsSubsystem.h"

#include "AnoberiaMultiplayer.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "Interfaces/OnlineLeaderboardInterface.h"
#include "Kismet/GameplayStatics.h"

UAnoberiaLeaderboardsSubsystem::UAnoberiaLeaderboardsSubsystem()
{
	if(IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get())
	{
		AchievementsInterface = OnlineSubsystem->GetAchievementsInterface();
		LeaderboardsInterface = OnlineSubsystem->GetLeaderboardsInterface();
		
	}
	else
		UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("OnlineSubsystem is not valid in %s"), *FString(__FUNCTION__))
}

void UAnoberiaLeaderboardsSubsystem::ReadWriteLeaderboards(const FName& AchievementName,
                                                           APlayerController* PlayerController, bool bNewSingleAchievement, bool bNewIncreaseLeaderboardValue,
                                                           int32 NewLeaderboardValue, bool bNewReadBestPlayers)
{
	if (AchievementsInterface && bCanReadWriteAchievements)
	{
		bCanReadWriteAchievements = false;

		LocalAchievementName = AchievementName;
		bIncreaseLeaderboardValue = bNewIncreaseLeaderboardValue;
		bSingleAchievement = bNewSingleAchievement;
		bReadBestPlayers = bNewReadBestPlayers;
		LeaderboardValue = NewLeaderboardValue;

		if (PlayerController)
		{
			LocalPlayer = Cast<ULocalPlayer>(PlayerController->Player);
			if (LocalPlayer)
				AchievementsInterface->QueryAchievements(*LocalPlayer->GetPreferredUniqueNetId(),
					FOnQueryAchievementsCompleteDelegate::CreateUObject(this, &UAnoberiaLeaderboardsSubsystem::OnQueryAchievementsComplete));
		}
	}
}

void UAnoberiaLeaderboardsSubsystem::ReadLeaderboardSingle(const FName& AchievementName, const FUniqueNetId& PlayerId)
{
	if (LeaderboardsInterface)
	{
		TArray< FUniqueNetIdRef > Players;
		Players.Add(PlayerId.AsShared());
		ReadObject = MakeShareable(new FOnlineLeaderboardRead());
		ReadObject->LeaderboardName = LocalAchievementName;
		ReadObject->SortedColumn = LocalAchievementName;
		new (ReadObject->ColumnMetadata) FColumnMetaData(LocalAchievementName, EOnlineKeyValuePairDataType::Int32);

		FOnlineLeaderboardReadRef ReadObjectRef = ReadObject.ToSharedRef();

		LeaderboardsInterface->AddOnLeaderboardReadCompleteDelegate_Handle(FOnLeaderboardReadCompleteDelegate::CreateUObject(this, &UAnoberiaLeaderboardsSubsystem::OnLeaderboardReadCompleteSingle));
		LeaderboardsInterface->ReadLeaderboards(Players, ReadObjectRef);
	}
}

void UAnoberiaLeaderboardsSubsystem::OnLeaderboardReadCompleteSingle(bool bWasSuccessful)
{
	if (LeaderboardsInterface)
		LeaderboardsInterface->ClearOnLeaderboardReadCompleteDelegates(this);

	int32 CurrentLeaderboardValue = 0;
	if (ReadObject.IsValid())
	{
		for (int i = 0; i < ReadObject->Rows.Num(); ++i)
		{
			const FOnlineStatsRow Stat = ReadObject->Rows[i];

			FVariantData* Variant = ReadObject->Rows[i].Columns.Find(LocalAchievementName);
			if (Variant)
				Variant->GetValue(CurrentLeaderboardValue);

			//Construct LeaderboardInfo and broadcast (Main Menu leaderboard or Live Leaderboard)
			FLeaderboardPlayerInfo Info;
			Info.Nickname = Stat.NickName;
			Info.Rank = Stat.Rank;
			Info.LeaderboardName = LocalAchievementName.ToString();

					
			if (LeaderboardValue == -1) //If coming from MainMenu, dont increase it. If coming from ColorMap, increase +1
			{
				const FString LevelName = UGameplayStatics::GetCurrentLevelName(this);
				if (LevelName == "MainMenu")
					Info.Value = CurrentLeaderboardValue; //Broadcast without +1, Coming from mainmenu
				else
					Info.Value = CurrentLeaderboardValue + 1; //Broadcast with +1, LiveLeaderboard
			}				
			else
				Info.Value = CurrentLeaderboardValue + LeaderboardValue; //New value

			OnLeaderboardReadCompleteSingleDelegate.Broadcast(Info); //Send Leaderboard Info
		}
	}

	if (LeaderboardValue == -1) //Increase CurrentLeaderboardValue by 1
		LeaderboardValue = ++CurrentLeaderboardValue;
	else
		LeaderboardValue += CurrentLeaderboardValue; //Increase Current + InGame value

	if (LocalPlayer)
	{
		if (bReadBestPlayers)
			ReadLeaderboardBest(LocalAchievementName, *LocalPlayer->GetPreferredUniqueNetId());

		if (bIncreaseLeaderboardValue)
		{
			//Increase Leaderboard Integer (First get current leaderboard value, than ++)
			FOnlineLeaderboardWrite WriteObject;
			WriteObject.LeaderboardNames.Add(LocalAchievementName);
			WriteObject.RatedStat = LocalAchievementName;
			WriteObject.DisplayFormat = ELeaderboardFormat::Number;
			WriteObject.SortMethod = ELeaderboardSort::Descending;
			WriteObject.UpdateMethod = ELeaderboardUpdateMethod::KeepBest;
			WriteObject.SetIntStat(LocalAchievementName, LeaderboardValue); //IncrementInt is not working....

			LeaderboardsInterface->WriteLeaderboards(NAME_GameSession, *LocalPlayer->GetPreferredUniqueNetId(), WriteObject);
			LeaderboardsInterface->FlushLeaderboards(NAME_GameSession);
		}
	}
}

void UAnoberiaLeaderboardsSubsystem::ReadLeaderboardBest(const FName& AchievementName, const FUniqueNetId& PlayerId)
{
	if (LeaderboardsInterface)
	{
		LocalAchievementName = AchievementName;

		TArray< FUniqueNetIdRef > Players;
		Players.Add(PlayerId.AsShared());
		ReadObject = MakeShareable(new FOnlineLeaderboardRead());
		ReadObject->LeaderboardName = AchievementName;
		ReadObject->SortedColumn = AchievementName;
		new (ReadObject->ColumnMetadata) FColumnMetaData(AchievementName, EOnlineKeyValuePairDataType::Int32);

		FOnlineLeaderboardReadRef ReadObjectRef = ReadObject.ToSharedRef();

		LeaderboardsInterface->AddOnLeaderboardReadCompleteDelegate_Handle(FOnLeaderboardReadCompleteDelegate::CreateUObject(this, &UAnoberiaLeaderboardsSubsystem::OnLeaderboardReadCompleteBest));
		LeaderboardsInterface->ReadLeaderboardsAroundRank(1, 15, ReadObjectRef);
	}
}

void UAnoberiaLeaderboardsSubsystem::OnLeaderboardReadCompleteBest(bool bWasSuccessful)
{
	if (LeaderboardsInterface)
		LeaderboardsInterface->ClearOnLeaderboardReadCompleteDelegates(this);

	TArray<FLeaderboardPlayerInfo> PlayerInfos;
	if (ReadObject.IsValid())
	{
		for (int i = 0; i < ReadObject->Rows.Num(); ++i)
		{
			const FOnlineStatsRow Stat = ReadObject->Rows[i];

			int32 Value = 0;
			FVariantData* Variant = ReadObject->Rows[i].Columns.Find(LocalAchievementName);
			if (Variant)
				Variant->GetValue(Value);

			FLeaderboardPlayerInfo Info;
			Info.Nickname = Stat.NickName;
			Info.Rank = Stat.Rank;
			Info.Value = Value;
			Info.LeaderboardName = LocalAchievementName.ToString();

			PlayerInfos.Add(Info);
		}
	}

	OnLeaderboardReadCompleteBestDelegate.Broadcast(PlayerInfos); //Send Leaderboard Infos
}

void UAnoberiaLeaderboardsSubsystem::OnQueryAchievementsComplete(const FUniqueNetId& PlayerId,
	const bool bWasSuccessful)
{
	bCanReadWriteAchievements = true;

	if (bSingleAchievement)
	{
		if (AchievementsInterface && LocalPlayer)
		{
			FOnlineAchievementsWritePtr WritePtr = MakeShareable(new FOnlineAchievementsWrite());
			FOnlineAchievementsWriteRef WriteRef = WritePtr.ToSharedRef();
			WritePtr->SetFloatStat(LocalAchievementName, 100.f);

			AchievementsInterface->WriteAchievements(*LocalPlayer->GetPreferredUniqueNetId(), WriteRef, FOnAchievementsWrittenDelegate::CreateUObject(this, &UAnoberiaLeaderboardsSubsystem::AchievementesWritten));
		}
	}
	else
		ReadLeaderboardSingle(LocalAchievementName, PlayerId);
}

void UAnoberiaLeaderboardsSubsystem::AchievementesWritten(const FUniqueNetId& PlayerId, bool bWasSuccessful)
{
	//printf(-1, 1.f, "AchievementesWritten %i", bWasSuccessful);
}

void UAnoberiaLeaderboardsSubsystem::ResetAch()
{
#if !UE_BUILD_SHIPPING
	const ULocalPlayer* SessionLocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (SessionLocalPlayer && AchievementsInterface)
		UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("Resetting achievements: %i"), AchievementsInterface->ResetAchievements(*SessionLocalPlayer->GetPreferredUniqueNetId()))
#endif
}
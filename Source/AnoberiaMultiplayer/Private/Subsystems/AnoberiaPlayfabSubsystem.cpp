


#include "Subsystems/AnoberiaPlayfabSubsystem.h"

#include "AnoberiaMultiplayer.h"
#include "GSDKUtils.h"
#include "PlayFabClientAPI.h"
#include "PlayFabMultiplayerAPI.h"

void UAnoberiaPlayfabSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	InitPlayFab();
	
	UGSDKUtils::ReadyForPlayers();
}

void UAnoberiaPlayfabSubsystem::InitPlayFab()
{
	FOnGSDKShutdown_Dyn OnGsdkShutdown;
	OnGsdkShutdown.Clear();
	OnGsdkShutdown.BindDynamic(this, &UAnoberiaPlayfabSubsystem::OnGSDKShutdown);
	FOnGSDKHealthCheck_Dyn OnGsdkHealthCheck;
	OnGsdkHealthCheck.Clear();
	OnGsdkHealthCheck.BindDynamic(this, &UAnoberiaPlayfabSubsystem::OnGSDKHealthCheck);
	FOnGSDKServerActive_Dyn OnGSDKServerActive;
	OnGSDKServerActive.Clear();
	OnGSDKServerActive.BindDynamic(this, &UAnoberiaPlayfabSubsystem::OnGSDKServerActive);
	FOnGSDKReadyForPlayers_Dyn OnGSDKReadyForPlayers;
	OnGSDKReadyForPlayers.Clear();
	OnGSDKReadyForPlayers.BindDynamic(this, &UAnoberiaPlayfabSubsystem::OnGSDKReadyForPlayers);

	UGSDKUtils::RegisterGSDKShutdownDelegate(OnGsdkShutdown);
	UGSDKUtils::RegisterGSDKHealthCheckDelegate(OnGsdkHealthCheck);
	UGSDKUtils::RegisterGSDKServerActiveDelegate(OnGSDKServerActive);
	UGSDKUtils::RegisterGSDKReadyForPlayers(OnGSDKReadyForPlayers);

#if UE_SERVER
	UGSDKUtils::SetDefaultServerHostPort();
#endif
}

void UAnoberiaPlayfabSubsystem::OnGSDKShutdown()
{
	UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("Shutdown!"));
	FPlatformMisc::RequestExit(false);
}

bool UAnoberiaPlayfabSubsystem::OnGSDKHealthCheck()
{
	UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("Healthy!"));
	return true;
}

void UAnoberiaPlayfabSubsystem::OnGSDKServerActive()
{
	/**
	* Server is transitioning to an active state.
	* Optional: Add in the implementation any code that is needed for the game server when
	* this transition occurs.
	*/
	UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("Active!"));
}

void UAnoberiaPlayfabSubsystem::OnGSDKReadyForPlayers()
{
	/**
	* Server is transitioning to a StandBy state. Game initialization is complete and the game is ready
	* to accept players.
	* Optional: Add in the implementation any code that is needed for the game server before
	* initialization completes.
	*/
	UE_LOG(LogAnoberiaMultiplayer, Warning, TEXT("Finished Initialization - Moving to StandBy!"));
}

void UAnoberiaPlayfabSubsystem::StartMathmaking()
{
	//Set Title Id
	if(GetMutableDefault<UPlayFabRuntimeSettings>())
		GetMutableDefault<UPlayFabRuntimeSettings>()->TitleId = TEXT("50E32");

	//Login
	ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
	if (ClientAPI)
	{
		PlayFab::ClientModels::FLoginWithCustomIDRequest LoginReq;
		LoginReq.CustomId = FGenericPlatformMisc::GetHashedMacAddressString();
		LoginReq.CreateAccount = true;
		ClientAPI->LoginWithCustomID(
			LoginReq,
			PlayFab::UPlayFabClientAPI::FLoginWithCustomIDDelegate::CreateUObject(this, &UAnoberiaPlayfabSubsystem::OnLoggedIn),
			PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &UAnoberiaPlayfabSubsystem::OnPlayFabError)
		);

		AnoberiaOnMatchmakingStatusDelegate.Broadcast(TEXT("Login.."));
	}
}

void UAnoberiaPlayfabSubsystem::OnLoggedIn(const PlayFab::ClientModels::FLoginResult& Result)
{
	AnoberiaOnMatchmakingStatusDelegate.Broadcast(TEXT("Logged in!"));

	LoginResult = Result;
	CancelMatchmakingTickets(true);
}

void UAnoberiaPlayfabSubsystem::OnMatchmakingTicketsCanceled(
	const PlayFab::MultiplayerModels::FCancelAllMatchmakingTicketsForPlayerResult& CancelTicketResult)
{
	if(bCreateNewTicket)
		AnoberiaOnMatchmakingStatusDelegate.Broadcast(TEXT("Previous tickets have been cancelled."));

	//Create Matchmaking Ticket
	MultiplayerAPI = IPlayFabModuleInterface::Get().GetMultiplayerAPI();
	if (MultiplayerAPI && bCreateNewTicket)
	{
		PlayFab::MultiplayerModels::FCreateMatchmakingTicketRequest CreateMMRequest;

		CreateMMRequest.GiveUpAfterSeconds = 60;
		CreateMMRequest.QueueName = TEXT("QuickMatch");
		CreateMMRequest.Creator.Entity.Id = LoginResult.EntityToken.Get()->Entity.Get()->Id;
		CreateMMRequest.Creator.Entity.Type = LoginResult.EntityToken.Get()->Entity.Get()->Type;

		TSharedPtr<FJsonObject> AttributesJsonObject = MakeShareable(new FJsonObject());

		TSharedPtr<FJsonObject> DataObject = MakeShareable(new FJsonObject());
		AttributesJsonObject->SetObjectField("DataObject", DataObject);

		TArray<TSharedPtr<FJsonValue>> LatenciesArray;
		TSharedPtr<FJsonObject> Latency = MakeShareable(new FJsonObject());
		LatenciesArray.Add(MakeShareable(new FJsonValueObject(Latency)));
		Latency->SetStringField("region", "NorthEurope");
		Latency->SetNumberField("latency", 50);

		DataObject->SetArrayField("Latencies", LatenciesArray);

		CreateMMRequest.Creator.Attributes = MakeShareable(new PlayFab::MultiplayerModels::FMatchmakingPlayerAttributes(AttributesJsonObject));

		MultiplayerAPI->CreateMatchmakingTicket(CreateMMRequest,
			PlayFab::UPlayFabMultiplayerAPI::FCreateMatchmakingTicketDelegate::CreateUObject(this, &UAnoberiaPlayfabSubsystem::OnMatchmakingTicketCreated),
			PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &UAnoberiaPlayfabSubsystem::OnPlayFabError));


		AnoberiaOnMatchmakingStatusDelegate.Broadcast(TEXT("A new matching ticket is being created.."));
	}
}

void UAnoberiaPlayfabSubsystem::OnMatchmakingTicketCreated(
	const PlayFab::MultiplayerModels::FCreateMatchmakingTicketResult& TicketResult)
{
	AnoberiaOnMatchmakingStatusDelegate.Broadcast(TEXT("A new matching ticket has been created."));

	MMTicketId = TicketResult.TicketId;

	if (GetWorld())
		GetWorld()->GetTimerManager().SetTimer(TH_MatchmakingTicket, this, &UAnoberiaPlayfabSubsystem::PollMatchmakingTicket, 6.f, true);
}

void UAnoberiaPlayfabSubsystem::PollMatchmakingTicket()
{
	MultiplayerAPI = IPlayFabModuleInterface::Get().GetMultiplayerAPI();
	if (MultiplayerAPI)
	{
		PlayFab::MultiplayerModels::FGetMatchmakingTicketRequest GetMMRequest;
		GetMMRequest.QueueName = TEXT("QuickMatch");
		GetMMRequest.TicketId = MMTicketId;
		MultiplayerAPI->GetMatchmakingTicket(GetMMRequest, PlayFab::UPlayFabMultiplayerAPI::FGetMatchmakingTicketDelegate::CreateUObject(this, &UAnoberiaPlayfabSubsystem::GetMatchmakingTicket),
			PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &UAnoberiaPlayfabSubsystem::OnPlayFabError));

		AnoberiaOnMatchmakingStatusDelegate.Broadcast(TEXT("Waiting for ticket response.."));
	}
}

void UAnoberiaPlayfabSubsystem::GetMatchmakingTicket(
	const PlayFab::MultiplayerModels::FGetMatchmakingTicketResult& TicketResult)
{
	AnoberiaOnMatchmakingStatusDelegate.Broadcast(FString::Printf(TEXT("Status: %s\nIn Queue: %i"), *TicketResult.Status, TicketResult.MembersToMatchWith.Num()));

	if (TicketResult.Status.Equals("Matched"))
	{
		ClearMatchmakingTimer();

		MultiplayerAPI = IPlayFabModuleInterface::Get().GetMultiplayerAPI();
		if (MultiplayerAPI)
		{
			PlayFab::MultiplayerModels::FGetMatchRequest GetMatchRequest;
			GetMatchRequest.QueueName = TEXT("QuickMatch");
			GetMatchRequest.MatchId = TicketResult.MatchId;
			MultiplayerAPI->GetMatch(GetMatchRequest, PlayFab::UPlayFabMultiplayerAPI::FGetMatchDelegate::CreateUObject(this, &UAnoberiaPlayfabSubsystem::GetMatch),
				PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &UAnoberiaPlayfabSubsystem::OnPlayFabError));

			AnoberiaOnMatchmakingStatusDelegate.Broadcast(TEXT("Match found! Joining.."));
		}
	}

	if (TicketResult.Status.Equals("Canceled"))
	{
		ClearMatchmakingTimer();
		//SendNetworkFailureMessage(FString::Printf(TEXT("Matchmaking has been cancelled: %s"), *TicketResult.CancellationReasonString));
	}
}

void UAnoberiaPlayfabSubsystem::GetMatch(const PlayFab::MultiplayerModels::FGetMatchResult& MatchResult)
{
	//open 55.55.55.55:30000
	const FString JoinAdress = FString::Printf(TEXT("open %s:%i"), *MatchResult.pfServerDetails.Get()->IPV4Address, MatchResult.pfServerDetails.Get()->Ports[0].Num);

	APlayerController* PCRef = GetWorld()->GetFirstPlayerController();
	if (PCRef)
		PCRef->ConsoleCommand(JoinAdress);
}

void UAnoberiaPlayfabSubsystem::OnPlayFabError(const PlayFab::FPlayFabCppError& ErrorResult)
{
	UE_LOG(LogTemp, Warning, TEXT("ERROR: %s"), *ErrorResult.GenerateErrorReport());
	AnoberiaOnMatchmakingStatusDelegate.Broadcast(FString::Printf(TEXT("ERROR: %s"), *ErrorResult.GenerateErrorReport()));
}

void UAnoberiaPlayfabSubsystem::CancelMatchmakingTickets(bool bNewCreateNewTicket)
{
	bCreateNewTicket = bNewCreateNewTicket;

	ClearMatchmakingTimer();

	MultiplayerAPI = IPlayFabModuleInterface::Get().GetMultiplayerAPI();
	if (MultiplayerAPI)
	{
		PlayFab::MultiplayerModels::FCancelAllMatchmakingTicketsForPlayerRequest CancelMMReq;
		CancelMMReq.QueueName = TEXT("QuickMatch");
		MultiplayerAPI->CancelAllMatchmakingTicketsForPlayer(CancelMMReq, PlayFab::UPlayFabMultiplayerAPI::FCancelAllMatchmakingTicketsForPlayerDelegate::CreateUObject(this, &UAnoberiaPlayfabSubsystem::OnMatchmakingTicketsCanceled),
			PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &UAnoberiaPlayfabSubsystem::OnPlayFabError));

		AnoberiaOnMatchmakingStatusDelegate.Broadcast(TEXT("Previous tickets are being cancelled.."));
	}
}

void UAnoberiaPlayfabSubsystem::ClearMatchmakingTimer()
{
	if (GetWorld())
		GetWorld()->GetTimerManager().ClearTimer(TH_MatchmakingTicket);
}





#include "HUD/BaseHUD.h"

#include "Blueprint/UserWidget.h"

void ABaseHUD::BeginPlay()
{
	Super::BeginPlay();

	SetInputMode();
	CreateWidgets();
}

void ABaseHUD::CreateWidgets()
{
	for (auto& WidgetClass : WidgetsToAdd)
	{
		if (WidgetClass)
		{
			if (UUserWidget* WidgetObject = CreateWidget<UUserWidget>(GetWorld(), WidgetClass))
				WidgetObject->AddToViewport();
		}
	}
}

void ABaseHUD::SetInputMode()
{
	if (const UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			if (bInputModeGameOnly)
				PlayerController->SetInputMode(FInputModeGameOnly());
			else
				PlayerController->SetInputMode(FInputModeUIOnly());
			PlayerController->SetShowMouseCursor(!bInputModeGameOnly);		
		}
	}
}

template <typename T>
T* ABaseHUD::GetWidgetObject(T* InWidgetClass)
{
	for (const auto& Widget : WidgetObjects)
	{
		if (Widget)
		{
			return Cast<T>(Widget);
		}
	}
	return nullptr;
}

void ABaseHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (const auto& Widget : WidgetObjects)
	{
		if (Widget)
			Widget->RemoveFromParent();
	}

	Super::EndPlay(EndPlayReason);
}
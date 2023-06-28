

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BaseHUD.generated.h"

/**
 * 
 */
UCLASS()
class ANOBERIACORE_API ABaseHUD : public AHUD
{
	GENERATED_BODY()

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual void CreateWidgets();

	UPROPERTY(EditDefaultsOnly, Category = "Base HUD")
	TArray<TSubclassOf<UUserWidget>> WidgetsToAdd;

	UPROPERTY(EditDefaultsOnly, Category = "Base HUD")
	bool bInputModeGameOnly = true;

private:
	
	UPROPERTY()
	TArray<UUserWidget*> WidgetObjects;

	void SetInputMode();
	
public:

	template <typename T>
	T* GetWidgetObject(T* InWidgetClass);
};


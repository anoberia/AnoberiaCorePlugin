

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BaseAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class ANOBERIACORE_API UBaseAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
    
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	
	UPROPERTY()
    ACharacter* OwnerCharacter;
	UPROPERTY()
    class UCharacterMovementComponent* CharacterMovement;
    
	/*
    * Animation properties
    */
	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bIsInAir;
	UPROPERTY(BlueprintReadOnly, Category = Movement)
	float Speed;
	FRotator RotationLastTick;
	UPROPERTY(BlueprintReadOnly, Category = Movement)
    float YawDelta;
	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bIsAccelerating;
	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bLayeredBlend;
	UPROPERTY(BlueprintReadOnly, Category = Movement)
	float Direction;
    
	/*
    * SFX
    */
	UFUNCTION(BlueprintCallable, Category = SFX)
	virtual void Footstep();
	UFUNCTION(BlueprintCallable, Category = SFX)
	virtual void JumpStart();
	UFUNCTION(BlueprintCallable, Category = SFX)
	virtual void JumpLand();
	
	UPROPERTY(EditAnywhere, Category = SFX)
	class USoundCue* FootstepCue;
	UPROPERTY(EditAnywhere, Category = SFX)
	USoundCue* JumpStartCue;
	UPROPERTY(EditAnywhere, Category = SFX)
	USoundCue* JumpLandCue;
	
};

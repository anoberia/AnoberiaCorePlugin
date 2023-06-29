


#include "AnimInstance/BaseAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundCue.h"

void UBaseAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
}

void UBaseAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	OwnerCharacter = !OwnerCharacter ? OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner()) : OwnerCharacter;
	if (OwnerCharacter)
	{
		/*
		* Speed
		*/
		Speed = OwnerCharacter->GetVelocity().Length();

		/*
		* Accelerating, InAir
		*/
		CharacterMovement = !CharacterMovement ? OwnerCharacter->GetCharacterMovement() : CharacterMovement;
		if (CharacterMovement)
		{
			bIsAccelerating = OwnerCharacter->GetCharacterMovement()->GetCurrentAcceleration().Length() > 0.f;
			bIsInAir = CharacterMovement->IsFalling();
		}
		
		/*
		* Layered Blend
		*/
		bLayeredBlend = GetCurveValue(FName("LayeredBlend")) ? true : false;

		/*
		* YawDelta
		*/
		const float TargetYawDelta = (UKismetMathLibrary::NormalizedDeltaRotator(RotationLastTick, OwnerCharacter->GetActorRotation()).Yaw / DeltaSeconds) / 7.f;
		YawDelta = FMath::FInterpTo(YawDelta, TargetYawDelta, DeltaSeconds, 6.f);
		RotationLastTick = OwnerCharacter->GetActorRotation();

		/*
		* Direction
		*/
		Direction = CalculateDirection(OwnerCharacter->GetVelocity(), OwnerCharacter->GetActorForwardVector().Rotation());
	}
}

void UBaseAnimInstance::Footstep()
{
	if (OwnerCharacter)
		UGameplayStatics::SpawnSoundAtLocation(this, FootstepCue, OwnerCharacter->GetActorLocation() - FVector(0.f, 0.f, 50.f));
}

void UBaseAnimInstance::JumpStart()
{
	if (OwnerCharacter)
		UGameplayStatics::SpawnSoundAttached(JumpStartCue, OwnerCharacter->GetMesh());
}

void UBaseAnimInstance::JumpLand()
{
	if (OwnerCharacter)
		UGameplayStatics::SpawnSoundAttached(JumpLandCue, OwnerCharacter->GetMesh());
}

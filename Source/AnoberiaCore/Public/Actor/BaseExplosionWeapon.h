

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseExplosionWeapon.generated.h"

UCLASS()
class ANOBERIACORE_API ABaseExplosionWeapon : public AActor
{
	GENERATED_BODY()

public:	
	ABaseExplosionWeapon();
	virtual void Destroyed() override;
protected: 
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void BoxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	virtual void ExplodeDamage();

	UPROPERTY(EditAnywhere, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float Damage = 20.f;
	UPROPERTY(EditAnywhere, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float DamageInnerRadius = 50.f;
	UPROPERTY(EditAnywhere, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float DamageOuterRadius = 300.f;
	UPROPERTY(EditAnywhere, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float ActivationTime = 2.f;
	
	bool bActivateImmediately = false;
	FTimerHandle TH_Activation;
	void ActivateBomb();

private:

	UPROPERTY(EditAnywhere, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* BoxCollision;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* WeaponMesh;
	UPROPERTY(EditAnywhere)
	class UParticleSystemComponent* LoopParticle;
	UPROPERTY(EditAnywhere)
	class UParticleSystem* DestroyedParticle;
	bool bAppliedDamage = false;
	UPROPERTY(EditAnywhere)
	class UAudioComponent* LoopSound;
	UPROPERTY(EditAnywhere)
	class USoundCue* ExplodeSound;
	UPROPERTY(EditAnywhere)
	FVector DestroyParticleScale = FVector(5.f);

};

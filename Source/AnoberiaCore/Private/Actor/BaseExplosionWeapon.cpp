


#include "Actor/BaseExplosionWeapon.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"


ABaseExplosionWeapon::ABaseExplosionWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	SetRootComponent(BoxCollision);
	BoxCollision->SetCollisionObjectType(ECC_WorldDynamic);
	BoxCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxCollision->SetCollisionResponseToAllChannels(ECR_Block);
	BoxCollision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	BoxCollision->SetBoxExtent(FVector(50.f));

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetRootComponent());
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

	LoopParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("LoopParticle"));
	LoopParticle->SetupAttachment(GetRootComponent());
	LoopParticle->SetActive(true);

	LoopSound = CreateDefaultSubobject<UAudioComponent>(TEXT("LoopSound"));
	LoopSound->SetupAttachment(GetRootComponent());
	LoopSound->SetActive(true);

	bReplicates = true;
}

void ABaseExplosionWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (bActivateImmediately)
			ActivateBomb();
		else
			GetWorldTimerManager().SetTimer(TH_Activation, this, &ABaseExplosionWeapon::ActivateBomb, ActivationTime);
	}	
}

void ABaseExplosionWeapon::ActivateBomb()
{
	if (BoxCollision)
	{
		BoxCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		BoxCollision->OnComponentHit.AddDynamic(this, &ABaseExplosionWeapon::BoxHit);
	}		
	GetWorldTimerManager().ClearTimer(TH_Activation);
}

void ABaseExplosionWeapon::BoxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	//on server
	if (OtherActor)
	{
		ExplodeDamage();
		Destroy();
	}
}

void ABaseExplosionWeapon::ExplodeDamage()
{
	if (bAppliedDamage) return;

	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (AController* OwnerController = OwnerCharacter->GetController<AController>())
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,
				Damage,
				10.f,
				GetActorLocation(),
				DamageInnerRadius,
				DamageOuterRadius,
				1.f,
				UDamageType::StaticClass(),
				TArray<AActor*>(),
				this,
				OwnerController
			);
			bAppliedDamage = true;
		}
	}
}

void ABaseExplosionWeapon::Destroyed()
{
	//replicated to all the clients
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DestroyedParticle, GetActorLocation(), GetActorRotation(), DestroyParticleScale);
	if (HasAuthority())
		ExplodeDamage();

	UGameplayStatics::SpawnSoundAtLocation(this, (USoundBase*)ExplodeSound, GetActorLocation());
	
	Super::Destroyed();
}

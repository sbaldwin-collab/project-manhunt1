#include "Gameplay/ClimbVolume.h"
#include "Characters/ManhuntPlayerCharacter.h"
#include "Components/SphereComponent.h"

AManhuntClimbVolume::AManhuntClimbVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	GroundPoint = CreateDefaultSubobject<USceneComponent>(TEXT("GroundPoint"));
	GroundPoint->SetupAttachment(RootComponent);
	GroundPoint->SetRelativeLocation(FVector(0.f, 0.f, 0.f));

	GroundTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("GroundTrigger"));
	GroundTrigger->SetupAttachment(GroundPoint);
	GroundTrigger->InitSphereRadius(100.f);
	GroundTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GroundTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	GroundTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Default vertical offset is a placeholder -- position both points to
	// the level's actual fire escape base/roof edge once built; see
	// /docs/NYC_LEVEL_DESIGN.md Section 5 for the two roof-access buildings'
	// intended story heights.
	RoofPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RoofPoint"));
	RoofPoint->SetupAttachment(RootComponent);
	RoofPoint->SetRelativeLocation(FVector(0.f, 0.f, 1700.f));

	RoofTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("RoofTrigger"));
	RoofTrigger->SetupAttachment(RoofPoint);
	RoofTrigger->InitSphereRadius(100.f);
	RoofTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RoofTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	RoofTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AManhuntClimbVolume::BeginPlay()
{
	Super::BeginPlay();

	GroundTrigger->OnComponentBeginOverlap.AddDynamic(this, &AManhuntClimbVolume::OnGroundTriggerOverlap);
	RoofTrigger->OnComponentBeginOverlap.AddDynamic(this, &AManhuntClimbVolume::OnRoofTriggerOverlap);
}

FTransform AManhuntClimbVolume::GetGroundTransform() const
{
	return GroundPoint->GetComponentTransform();
}

FTransform AManhuntClimbVolume::GetRoofTransform() const
{
	return RoofPoint->GetComponentTransform();
}

void AManhuntClimbVolume::OnGroundTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AManhuntPlayerCharacter* Player = Cast<AManhuntPlayerCharacter>(OtherActor))
	{
		if (!Player->IsOnRooftop())
		{
			Player->BeginClimb(this, /*bClimbingUp=*/true);
		}
	}
}

void AManhuntClimbVolume::OnRoofTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AManhuntPlayerCharacter* Player = Cast<AManhuntPlayerCharacter>(OtherActor))
	{
		if (Player->IsOnRooftop())
		{
			Player->BeginClimb(this, /*bClimbingUp=*/false);
		}
	}
}

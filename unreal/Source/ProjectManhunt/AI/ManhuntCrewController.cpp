#include "AI/ManhuntCrewController.h"
#include "AI/ManhuntHunterController.h"
#include "Characters/ManhuntHunterCharacter.h"
#include "Characters/ManhuntCrewCharacter.h"
#include "Components/ManhuntDetectionComponent.h"
#include "Data/ManhuntGameSettings.h"

#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

AManhuntCrewController::AManhuntCrewController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AManhuntCrewController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AActor* HunterActor = UGameplayStatics::GetActorOfClass(this, AManhuntHunterCharacter::StaticClass()))
	{
		CachedHunter = Cast<AManhuntHunterCharacter>(HunterActor);
	}
}

bool AManhuntCrewController::IsSeenByHunter(FVector& OutHunterLocation) const
{
	if (!CachedHunter)
	{
		return false;
	}

	if (const AManhuntHunterController* HunterController = Cast<AManhuntHunterController>(CachedHunter->GetController()))
	{
		if (UManhuntDetectionComponent* Detection = HunterController->GetDetectionComponent())
		{
			OutHunterLocation = CachedHunter->GetActorLocation();
			return Detection->IsTargetCurrentlyDetected(GetPawn());
		}
	}

	return false;
}

void AManhuntCrewController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AManhuntCrewCharacter* Crew = Cast<AManhuntCrewCharacter>(GetPawn());
	if (!Crew || Crew->IsCaptured())
	{
		return;
	}

	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();

	FVector HunterLocation;
	const bool bSeen = IsSeenByHunter(HunterLocation);
	const float DistToHunter = bSeen ? FVector::Dist(Crew->GetActorLocation(), HunterLocation) : TNumericLimits<float>::Max();

	// src/game/CrewAI.js: `hunterVisible = dHunter < fleeTriggerDistance && detection.canSee(...)`
	if (bSeen && DistToHunter < Settings->CrewFleeTriggerDistanceCm)
	{
		Crew->SetMovementSpeed(Settings->CrewFleeSpeed);
		Crew->SetLocomotionState(EManhuntLocomotionState::Sprint);
		TickFlee(DeltaTime, HunterLocation);
		bHasWanderTarget = false; // re-roll a wander target once the flee ends
	}
	else
	{
		Crew->SetMovementSpeed(Settings->CrewWanderSpeed);
		Crew->SetLocomotionState(EManhuntLocomotionState::Walk);
		TickWander(DeltaTime);
	}
}

void AManhuntCrewController::TickFlee(float DeltaTime, const FVector& HunterLocation)
{
	APawn* Crew = GetPawn();
	if (!Crew)
	{
		return;
	}

	const FVector FleeDirection = (Crew->GetActorLocation() - HunterLocation).GetSafeNormal();
	MoveToLocation(Crew->GetActorLocation() + FleeDirection * 500.f, /*AcceptanceRadius=*/50.f, /*bStopOnOverlap=*/false);
}

void AManhuntCrewController::TickWander(float DeltaTime)
{
	APawn* Crew = GetPawn();
	if (!Crew)
	{
		return;
	}

	const bool bReachedTarget = bHasWanderTarget && FVector::DistSquared(Crew->GetActorLocation(), WanderTarget) < FMath::Square(110.f);

	if (!bHasWanderTarget || bReachedTarget)
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			FNavLocation Result;
			// src/world/World.js `randomSpawnPoint()` -- 3600cm (~36m) search radius,
			// matches the browser's per-quadrant wander range loosely; tune once the
			// real level's NavMesh extents are known.
			if (NavSys->GetRandomReachablePointInRadius(Crew->GetActorLocation(), 3600.f, Result))
			{
				WanderTarget = Result.Location;
				bHasWanderTarget = true;
			}
		}
	}

	if (bHasWanderTarget)
	{
		MoveToLocation(WanderTarget, /*AcceptanceRadius=*/110.f);
	}
}

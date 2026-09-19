#include "AI/ManhuntHunterController.h"
#include "Characters/ManhuntHunterCharacter.h"
#include "Characters/ManhuntPlayerCharacter.h"
#include "Characters/ManhuntCrewCharacter.h"
#include "Components/ManhuntDetectionComponent.h"
#include "Data/ManhuntGameSettings.h"
#include "ProjectManhunt.h"

#include "Perception/AIPerceptionComponent.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"

AManhuntHunterController::AManhuntHunterController()
{
	PrimaryActorTick.bCanEverTick = true;

	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	Detection = CreateDefaultSubobject<UManhuntDetectionComponent>(TEXT("Detection"));
	SetPerceptionComponent(*Perception);
}

void AManhuntHunterController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	Detection->InitializeDetection(Perception);
	PickNewPatrolPoint();

	// See the class header comment: OptionalBehaviorTreeAsset is a hookup
	// point for a future Behavior-Tree-based reimplementation. The native
	// Tick() state machine below runs regardless and is the active
	// implementation in this scaffold.
	if (OptionalBehaviorTreeAsset)
	{
		UE_LOG(LogManhuntAI, Warning, TEXT("AManhuntHunterController: OptionalBehaviorTreeAsset is assigned but this scaffold does not run it -- "
			"see the class header comment before wiring RunBehaviorTree() up, to avoid two state machines fighting over the same pawn."));
	}
}

void AManhuntHunterController::SetState(EManhuntHunterState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	CurrentState = NewState;

	if (AManhuntHunterCharacter* Hunter = Cast<AManhuntHunterCharacter>(GetPawn()))
	{
		Hunter->SetLocomotionState(NewState == EManhuntHunterState::Chase ? EManhuntLocomotionState::Sprint : EManhuntLocomotionState::Walk);
	}

	Detection->SetPatrolAlertnessReduced(NewState == EManhuntHunterState::Patrol || NewState == EManhuntHunterState::Return);

	OnHunterStateChanged.Broadcast();
}

AActor* AManhuntHunterController::FindBestVisibleTarget() const
{
	TArray<AActor*> Candidates;

	if (AActor* Player = UGameplayStatics::GetActorOfClass(this, AManhuntPlayerCharacter::StaticClass()))
	{
		Candidates.Add(Player);
	}

	TArray<AActor*> CrewActors;
	UGameplayStatics::GetAllActorsOfClass(this, AManhuntCrewCharacter::StaticClass(), CrewActors);
	for (AActor* CrewActor : CrewActors)
	{
		if (const AManhuntCrewCharacter* Crew = Cast<AManhuntCrewCharacter>(CrewActor))
		{
			if (!Crew->IsCaptured())
			{
				Candidates.Add(CrewActor);
			}
		}
	}

	return Detection->FindNearestDetectedActor(Candidates);
}

void AManhuntHunterController::PickNewPatrolPoint()
{
	if (!GetPawn())
	{
		return;
	}

	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation Result;
		// Search radius is a placeholder -- once the level exists, prefer
		// pre-placed patrol point Actors per block (see
		// /docs/NYC_LEVEL_DESIGN.md) over a pure random NavMesh sample, so
		// patrol routes read as deliberate rather than jittery.
		if (NavSys->GetRandomReachablePointInRadius(GetPawn()->GetActorLocation(), 4000.f, Result))
		{
			CurrentTargetLocation = Result.Location;
			return;
		}
	}

	CurrentTargetLocation = GetPawn()->GetActorLocation();
}

void AManhuntHunterController::MoveTowardsAtSpeed(const FVector& Location, float SpeedCm)
{
	if (AManhuntHunterCharacter* Hunter = Cast<AManhuntHunterCharacter>(GetPawn()))
	{
		Hunter->SetMovementSpeed(SpeedCm);
	}
	MoveToLocation(Location, /*AcceptanceRadius=*/110.f, /*bStopOnOverlap=*/true, /*bUsePathfinding=*/true);
}

void AManhuntHunterController::NotifyCaptureEvent()
{
	// src/game/HunterAI.js `notifyCaptureEvent()`: rolls GuardCrewChance and,
	// on success, force-transitions to Guard from ANY current state.
	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	if (FMath::FRand() < Settings->GuardCrewChance)
	{
		GuardTimerSeconds = 0.f;
		SetState(EManhuntHunterState::Guard);
	}
}

void AManhuntHunterController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!GetPawn())
	{
		return;
	}

	// Guard is entered externally (NotifyCaptureEvent) and explicitly skips
	// the normal perception-driven branch below -- src/game/HunterAI.js:
	// `const spotted = this.state !== HunterState.GUARD ? this._findVisibleTarget(...) : null;`
	AActor* Spotted = (CurrentState != EManhuntHunterState::Guard) ? FindBestVisibleTarget() : nullptr;

	if (Spotted)
	{
		LastKnownLocation = Spotted->GetActorLocation();
		bHasLastKnownLocation = true;
		CurrentTargetLocation = LastKnownLocation;
		CurrentTargetRef = Spotted;

		if (CurrentState == EManhuntHunterState::Chase)
		{
			SearchTimerSeconds = 0.f;
		}
		else
		{
			const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
			SuspicionMeterSeconds += DeltaTime;
			SetState(EManhuntHunterState::Suspicious);

			if (SuspicionMeterSeconds >= Settings->SuspiciousToChaseSeconds)
			{
				SuspicionMeterSeconds = 0.f;
				SetState(EManhuntHunterState::Chase);
				OnHunterSpottedTarget.Broadcast(Spotted);
			}
		}
	}
	else
	{
		switch (CurrentState)
		{
		case EManhuntHunterState::Suspicious:
			SuspicionMeterSeconds -= DeltaTime * 1.4f; // browser: asymmetric decay rate
			if (SuspicionMeterSeconds <= 0.f)
			{
				SuspicionMeterSeconds = 0.f;
				SetState(EManhuntHunterState::Patrol);
			}
			break;

		case EManhuntHunterState::Chase:
			SetState(EManhuntHunterState::Search);
			SearchTimerSeconds = 0.f;
			if (bHasLastKnownLocation)
			{
				CurrentTargetLocation = LastKnownLocation;
			}
			OnHunterLostTarget.Broadcast();
			break;

		case EManhuntHunterState::Search:
			TickSearch(DeltaTime);
			break;

		case EManhuntHunterState::Return:
			TickReturn(DeltaTime);
			break;

		case EManhuntHunterState::Guard:
			TickGuard(DeltaTime);
			break;

		case EManhuntHunterState::Investigate:
			TickInvestigate(DeltaTime);
			break;

		case EManhuntHunterState::Patrol:
		default:
			TickPatrol(DeltaTime);
			break;
		}
	}

	// Movement + speed for whatever state we ended this tick in.
	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	float Speed = Settings->HunterSpeedPatrol;
	switch (CurrentState)
	{
	case EManhuntHunterState::Suspicious:   Speed = Settings->HunterSpeedSuspicious; break;
	case EManhuntHunterState::Investigate:  Speed = Settings->HunterSpeedInvestigate; break;
	case EManhuntHunterState::Search:       Speed = Settings->HunterSpeedSearch; break;
	case EManhuntHunterState::Chase:        Speed = Settings->HunterSpeedChase; break;
	case EManhuntHunterState::Guard:        Speed = Settings->HunterSpeedSuspicious * 0.7f; break;
	case EManhuntHunterState::Return:
	case EManhuntHunterState::Patrol:
	default:                                Speed = Settings->HunterSpeedPatrol; break;
	}

	if (CurrentState == EManhuntHunterState::Chase && CurrentTargetRef.IsValid())
	{
		if (AManhuntHunterCharacter* Hunter = Cast<AManhuntHunterCharacter>(GetPawn()))
		{
			Hunter->SetMovementSpeed(Speed);
		}
		MoveToActor(CurrentTargetRef.Get(), /*AcceptanceRadius=*/60.f);
	}
	else
	{
		MoveTowardsAtSpeed(CurrentTargetLocation, Speed);
	}
}

void AManhuntHunterController::TickPatrol(float DeltaTime)
{
	FVector NoiseLocation;
	if (Detection->ConsumePendingNoiseEvent(NoiseLocation))
	{
		CurrentTargetLocation = NoiseLocation;
		InvestigateTimerSeconds = 0.f;
		SetState(EManhuntHunterState::Investigate);
		return;
	}

	if (GetPawn() && FVector::DistSquared(GetPawn()->GetActorLocation(), CurrentTargetLocation) < FMath::Square(110.f))
	{
		PickNewPatrolPoint();
	}
}

void AManhuntHunterController::TickInvestigate(float DeltaTime)
{
	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();

	const bool bArrived = GetPawn() && FVector::DistSquared(GetPawn()->GetActorLocation(), CurrentTargetLocation) < FMath::Square(120.f);
	// src/game/HunterAI.js: timer runs 3x once arrived (a brisk look-around).
	InvestigateTimerSeconds += DeltaTime * (bArrived ? 3.f : 1.f);

	if (InvestigateTimerSeconds >= Settings->InvestigateTotalSeconds)
	{
		ReturnTimerSeconds = 0.f;
		SetState(EManhuntHunterState::Return);
		PickNewPatrolPoint();
	}
}

void AManhuntHunterController::TickSearch(float DeltaTime)
{
	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	SearchTimerSeconds += DeltaTime;

	const bool bReachedLastKnown = bHasLastKnownLocation && GetPawn()
		&& FVector::DistSquared(GetPawn()->GetActorLocation(), LastKnownLocation) < FMath::Square(130.f);

	if (bReachedLastKnown || SearchTimerSeconds >= Settings->SearchDurationSeconds)
	{
		ReturnTimerSeconds = 0.f;
		SetState(EManhuntHunterState::Return);
		PickNewPatrolPoint();
	}
}

void AManhuntHunterController::TickReturn(float DeltaTime)
{
	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	ReturnTimerSeconds += DeltaTime;

	if (ReturnTimerSeconds >= Settings->ReturnToPatrolGraceSeconds)
	{
		SetState(EManhuntHunterState::Patrol);
	}
}

void AManhuntHunterController::TickGuard(float DeltaTime)
{
	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	GuardTimerSeconds += DeltaTime;

	// src/game/HunterAI.js: orbits the jail at (radius + 1.6m), angular
	// speed 0.4 rad/s.
	const float OrbitAngle = GuardTimerSeconds * 0.4f;
	const float OrbitRadiusCm = Settings->JailRadiusCm + 160.f;
	const FVector JailCenter(Settings->JailLocationCm.X, Settings->JailLocationCm.Y, GetPawn() ? GetPawn()->GetActorLocation().Z : 0.f);

	CurrentTargetLocation = JailCenter + FVector(FMath::Cos(OrbitAngle), FMath::Sin(OrbitAngle), 0.f) * OrbitRadiusCm;

	if (GuardTimerSeconds >= Settings->GuardDurationSeconds)
	{
		ReturnTimerSeconds = 0.f;
		SetState(EManhuntHunterState::Return);
		PickNewPatrolPoint();
	}
}

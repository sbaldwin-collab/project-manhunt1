#include "Components/ManhuntDetectionComponent.h"
#include "Components/ManhuntDetectableInterface.h"
#include "Data/ManhuntGameSettings.h"
#include "ProjectManhunt.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"

UManhuntDetectionComponent::UManhuntDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UManhuntDetectionComponent::InitializeDetection(UAIPerceptionComponent* InPerception)
{
	Perception = InPerception;
	if (!Perception)
	{
		UE_LOG(LogManhuntAI, Warning, TEXT("UManhuntDetectionComponent::InitializeDetection called with a null AIPerceptionComponent."));
		return;
	}

	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();

	// Sight: base range + full cone -- AIPerception wants the HALF angle,
	// see /docs/HUNTER_AI_SPEC.md Section 4. The crouch-target and
	// patrol-alertness multipliers are NOT configured here because they are
	// per-candidate / per-state, not a fixed sense property -- they are
	// applied as a post-filter in HandlePerceptionUpdated below.
	UAISenseConfig_Sight* SightConfig = NewObject<UAISenseConfig_Sight>(this, TEXT("Sight"));
	SightConfig->SightRadius = Settings->HunterViewDistanceBaseCm;
	SightConfig->LoseSightRadius = Settings->HunterViewDistanceBaseCm * 1.15f;
	SightConfig->PeripheralVisionAngleDegrees = Settings->HunterFovDegrees * 0.5f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	UAISenseConfig_Hearing* HearingConfig = NewObject<UAISenseConfig_Hearing>(this, TEXT("Hearing"));
	HearingConfig->HearingRange = Settings->HunterHearingRadiusCm;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;

	Perception->ConfigureSense(*SightConfig);
	Perception->ConfigureSense(*HearingConfig);
	Perception->SetDominantSense(SightConfig->GetSenseImplementation());

	Perception->OnTargetPerceptionUpdated.AddDynamic(this, &UManhuntDetectionComponent::HandlePerceptionUpdated);
}

float UManhuntDetectionComponent::GetEffectiveSightRangeCmFor(const AActor* Target) const
{
	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	float Range = Settings->HunterViewDistanceBaseCm;

	if (bPatrolAlertnessReduced)
	{
		Range *= Settings->PatrolAlertnessViewMultiplier;
	}

	if (Target && Target->Implements<UManhuntDetectableInterface>())
	{
		if (IManhuntDetectableInterface::Execute_IsCrouchingForDetection(Target))
		{
			Range *= Settings->CrouchTargetViewMultiplier;
		}
	}

	return Range;
}

bool UManhuntDetectionComponent::PassesElevationGate(const AActor* Target) const
{
	if (!Target || !GetOwner())
	{
		return false;
	}

	// Fast path: an explicit rooftop-layer flag, mirroring
	// PlayerController.js's roofLayer !== null. Falls through to a raw Z
	// check as a defensive default if the target doesn't implement the
	// interface (e.g. a non-Manhunt actor briefly perceived).
	if (Target->Implements<UManhuntDetectableInterface>()
		&& IManhuntDetectableInterface::Execute_IsOnRooftopLayer(Target))
	{
		return false;
	}

	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	const float DeltaZ = FMath::Abs(GetOwner()->GetActorLocation().Z - Target->GetActorLocation().Z);
	return DeltaZ <= Settings->ElevationDetectionGateCm;
}

void UManhuntDetectionComponent::HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	bool bGatedDetected = false;
	const bool bIsHearing = Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>();

	if (Stimulus.WasSuccessfullySensed())
	{
		const bool bWithinGatedRange = Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>()
			? FVector::Dist(GetOwner()->GetActorLocation(), Actor->GetActorLocation()) <= GetEffectiveSightRangeCmFor(Actor)
			: true; // hearing already carries its own range via HearingConfig; no per-target multiplier documented for it.

		bGatedDetected = bWithinGatedRange && PassesElevationGate(Actor);

		if (bIsHearing && bGatedDetected)
		{
			// src/world/World.js noise pings carry their own (thrower's)
			// location, not the Hunter's -- Stimulus.StimulusLocation is the
			// correct source here (the reported noise point, e.g. a decoy
			// thrown 8m behind the player), not Actor->GetActorLocation().
			PendingNoiseLocation = Stimulus.StimulusLocation;
			bHasPendingNoiseEvent = true;
		}
	}

	const bool* PreviousState = GatedDetectionState.Find(Actor);
	const bool bChanged = !PreviousState || (*PreviousState != bGatedDetected);

	GatedDetectionState.FindOrAdd(Actor) = bGatedDetected;

	if (bChanged)
	{
		OnTargetDetectionChanged.Broadcast(Actor, bGatedDetected);
	}
}

bool UManhuntDetectionComponent::ConsumePendingNoiseEvent(FVector& OutLocation)
{
	if (!bHasPendingNoiseEvent)
	{
		return false;
	}

	OutLocation = PendingNoiseLocation;
	bHasPendingNoiseEvent = false;
	return true;
}

bool UManhuntDetectionComponent::IsTargetCurrentlyDetected(AActor* Target) const
{
	const bool* State = GatedDetectionState.Find(Target);
	return State && *State;
}

AActor* UManhuntDetectionComponent::FindNearestDetectedActor(const TArray<AActor*>& Candidates) const
{
	AActor* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();

	if (!GetOwner())
	{
		return nullptr;
	}

	const FVector SelfLocation = GetOwner()->GetActorLocation();

	for (AActor* Candidate : Candidates)
	{
		if (!Candidate || !IsTargetCurrentlyDetected(Candidate))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(SelfLocation, Candidate->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Candidate;
		}
	}

	return Nearest;
}

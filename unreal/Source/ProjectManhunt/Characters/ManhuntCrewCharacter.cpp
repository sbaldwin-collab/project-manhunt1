#include "Characters/ManhuntCrewCharacter.h"
#include "GameModes/ManhuntGameState.h"
#include "Gameplay/RescueComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AManhuntCrewCharacter::AManhuntCrewCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

void AManhuntCrewCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AManhuntGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AManhuntGameState>() : nullptr)
	{
		if (URescueComponent* Rescue = GameState->GetRescueComponent())
		{
			Rescue->RegisterCrewMember(this);
		}
	}
}

void AManhuntCrewCharacter::SetCaptured(bool bInCaptured)
{
	bIsCaptured = bInCaptured;

	// Halts the AController's Tick-driven wander/flee logic while captured
	// (see AManhuntCrewController::Tick) -- movement resumes on rescue via
	// URescueComponent calling SetCaptured(false) again.
	if (AController* Ctrl = GetController())
	{
		Ctrl->SetActorTickEnabled(!bInCaptured);
	}

	SetLocomotionState(EManhuntLocomotionState::Idle);
}

void AManhuntCrewCharacter::SetMovementSpeed(float NewSpeedCm)
{
	GetCharacterMovement()->MaxWalkSpeed = NewSpeedCm;
}

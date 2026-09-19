#include "GameModes/ManhuntGameMode.h"
#include "GameModes/ManhuntGameState.h"
#include "GameModes/ManhuntPlayerController.h"
#include "Characters/ManhuntPlayerCharacter.h"
#include "Characters/ManhuntHunterCharacter.h"
#include "Characters/ManhuntCrewCharacter.h"
#include "AI/ManhuntHunterController.h"
#include "Components/ManhuntCameraComponent.h"
#include "Gameplay/RescueComponent.h"
#include "Data/ManhuntGameSettings.h"
#include "Audio/ManhuntAudioManager.h"
#include "ProjectManhunt.h"

#include "Kismet/GameplayStatics.h"

AManhuntGameMode::AManhuntGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultPawnClass = AManhuntPlayerCharacter::StaticClass();
	PlayerControllerClass = AManhuntPlayerController::StaticClass();
	GameStateClass = AManhuntGameState::StaticClass();
}

void AManhuntGameMode::StartPlay()
{
	Super::StartPlay();

	if (AActor* HunterActor = UGameplayStatics::GetActorOfClass(this, AManhuntHunterCharacter::StaticClass()))
	{
		CachedHunter = Cast<AManhuntHunterCharacter>(HunterActor);

		if (AManhuntHunterController* HunterController = CachedHunter ? Cast<AManhuntHunterController>(CachedHunter->GetController()) : nullptr)
		{
			HunterController->OnHunterSpottedTarget.AddDynamic(this, &AManhuntGameMode::HandleHunterSpottedTarget);
			HunterController->OnHunterLostTarget.AddDynamic(this, &AManhuntGameMode::HandleHunterLostTarget);
		}
	}
	else
	{
		UE_LOG(LogManhunt, Error, TEXT("AManhuntGameMode::StartPlay -- no AManhuntHunterCharacter found in the level. Place one per /docs/NYC_LEVEL_DESIGN.md."));
	}

	if (AActor* PlayerActor = UGameplayStatics::GetActorOfClass(this, AManhuntPlayerCharacter::StaticClass()))
	{
		CachedPlayer = Cast<AManhuntPlayerCharacter>(PlayerActor);
	}

	RestartRound();
}

void AManhuntGameMode::RestartRound()
{
	bCaptureResolvedThisRound = false;

	if (AManhuntGameState* GameState = GetGameState<AManhuntGameState>())
	{
		if (GameState->GetRescueComponent())
		{
			GameState->GetRescueComponent()->ResetForNewRound();
		}
		GameState->BeginRound();
	}

	// Hand-off point: level-specific actor repositioning (player/Hunter/crew
	// spawn points, matching src/world/World.randomSpawnPoint()) belongs in
	// a level Blueprint or a dedicated spawn-point manager once the actual
	// NYC level exists -- not hardcoded here, since spawn locations are
	// level content, not a GameMode responsibility.

	if (CachedPlayer)
	{
		if (UManhuntCameraComponent* CameraBehavior = CachedPlayer->FindComponentByClass<UManhuntCameraComponent>())
		{
			CameraBehavior->PlayCinematicIntro();
		}
	}
}

void AManhuntGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AManhuntGameState* GameState = GetGameState<AManhuntGameState>();
	if (!GameState || !GameState->IsRoundActive())
	{
		return;
	}

	GameState->RoundElapsedSeconds += DeltaTime;

	if (GameState->GetTimeRemainingSeconds() <= 0.f)
	{
		EndRound(/*bWasWin=*/true);
		return;
	}

	CheckCaptures();
}

void AManhuntGameMode::CheckCaptures()
{
	if (!CachedHunter || bCaptureResolvedThisRound)
	{
		return;
	}

	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	const FVector HunterLocation = CachedHunter->GetActorLocation();

	// Player capture -- gated on NOT being on a rooftop, matching
	// src/game/Game.js `_update()`: `this.player.roofLayer === null`.
	if (CachedPlayer && !CachedPlayer->IsOnRooftop())
	{
		const float DistToPlayer = FVector::Dist(HunterLocation, CachedPlayer->GetActorLocation());
		if (DistToPlayer < Settings->CaptureDistanceCm)
		{
			if (UManhuntCameraComponent* CameraBehavior = CachedPlayer->FindComponentByClass<UManhuntCameraComponent>())
			{
				CameraBehavior->PlayCaptureShake();
			}
			EndRound(/*bWasWin=*/false);
			return;
		}
	}

	// Crew capture -- no rooftop gate; crew never leave street level
	// (see /docs/VERTICAL_SLICE_SCOPE.md Section 3).
	if (AManhuntGameState* GameState = GetGameState<AManhuntGameState>())
	{
		if (URescueComponent* Rescue = GameState->GetRescueComponent())
		{
			TArray<AActor*> CrewActors;
			UGameplayStatics::GetAllActorsOfClass(this, AManhuntCrewCharacter::StaticClass(), CrewActors);
			for (AActor* CrewActor : CrewActors)
			{
				AManhuntCrewCharacter* Crew = Cast<AManhuntCrewCharacter>(CrewActor);
				if (!Crew || Crew->IsCaptured())
				{
					continue;
				}
				if (FVector::Dist(HunterLocation, Crew->GetActorLocation()) < Settings->CaptureDistanceCm)
				{
					Rescue->CaptureCrew(Crew);
				}
			}
		}
	}
}

void AManhuntGameMode::HandleHunterSpottedTarget(AActor* Target)
{
	UManhuntAudioManager* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UManhuntAudioManager>() : nullptr;

	if (Target == CachedPlayer)
	{
		if (Audio)
		{
			Audio->SetChasing(true);
		}
		if (CachedPlayer)
		{
			if (UManhuntCameraComponent* CameraBehavior = CachedPlayer->FindComponentByClass<UManhuntCameraComponent>())
			{
				CameraBehavior->PlayChaseShake();
			}
		}
	}
	// NOTE (simplification): a Hunter chasing a crew member does not toggle
	// the chase audio layer in this scaffold, since the browser build's
	// chase cue is specifically a player-tension cue. Revisit if the final
	// design wants ambient tension for crew chases too.
}

void AManhuntGameMode::HandleHunterLostTarget()
{
	if (UManhuntAudioManager* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UManhuntAudioManager>() : nullptr)
	{
		Audio->SetChasing(false);
	}
}

void AManhuntGameMode::EndRound(bool bWasWin)
{
	bCaptureResolvedThisRound = true;

	if (AManhuntGameState* GameState = GetGameState<AManhuntGameState>())
	{
		GameState->EndRound(bWasWin);
	}

	if (UManhuntAudioManager* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UManhuntAudioManager>() : nullptr)
	{
		Audio->SetChasing(false);
		Audio->Play(bWasWin ? TEXT("Win") : TEXT("Lose"));
	}
}

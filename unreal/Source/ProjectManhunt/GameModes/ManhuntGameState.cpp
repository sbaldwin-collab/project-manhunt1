#include "GameModes/ManhuntGameState.h"
#include "Gameplay/RescueComponent.h"
#include "Data/ManhuntGameSettings.h"

AManhuntGameState::AManhuntGameState()
{
	RescueComponent = CreateDefaultSubobject<URescueComponent>(TEXT("RescueComponent"));
}

float AManhuntGameState::GetTimeRemainingSeconds() const
{
	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	return FMath::Max(0.f, Settings->RoundDurationSeconds - RoundElapsedSeconds);
}

void AManhuntGameState::BeginRound()
{
	bRoundActive = true;
	RoundElapsedSeconds = 0.f;
	RescuesThisRound = 0;
	OnRoundStarted.Broadcast();
}

void AManhuntGameState::EndRound(bool bWasWin)
{
	bRoundActive = false;
	OnRoundEnded.Broadcast(bWasWin);
}

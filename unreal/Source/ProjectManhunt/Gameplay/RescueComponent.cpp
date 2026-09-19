#include "Gameplay/RescueComponent.h"
#include "Gameplay/JailVolume.h"
#include "Characters/ManhuntCrewCharacter.h"
#include "Data/ManhuntGameSettings.h"
#include "AI/ManhuntHunterController.h"
#include "Characters/ManhuntHunterCharacter.h"
#include "Audio/ManhuntAudioManager.h"
#include "ProjectManhunt.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

URescueComponent::URescueComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URescueComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Jail = UGameplayStatics::GetActorOfClass(this, AJailVolume::StaticClass()))
	{
		CachedJailVolume = Cast<AJailVolume>(Jail);
	}
	else
	{
		UE_LOG(LogManhunt, Warning, TEXT("URescueComponent: no AJailVolume found in the level -- falling back to UManhuntGameSettings::JailLocationCm. Place one per /docs/NYC_LEVEL_DESIGN.md."));
	}
}

FVector URescueComponent::GetJailLocation() const
{
	if (CachedJailVolume)
	{
		return CachedJailVolume->GetActorLocation();
	}
	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	return FVector(Settings->JailLocationCm.X, Settings->JailLocationCm.Y, 0.f);
}

float URescueComponent::GetJailRadius() const
{
	return CachedJailVolume ? CachedJailVolume->GetRadius() : UManhuntGameSettings::Get()->JailRadiusCm;
}

void URescueComponent::RegisterCrewMember(AManhuntCrewCharacter* Crew)
{
	if (Crew)
	{
		AllCrew.AddUnique(Crew);
	}
}

void URescueComponent::CaptureCrew(AManhuntCrewCharacter* Crew)
{
	if (!Crew || Crew->IsCaptured())
	{
		return;
	}

	Crew->SetCaptured(true);
	ArrangeCapturedCrewInCage();

	if (UManhuntAudioManager* Audio = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UManhuntAudioManager>() : nullptr)
	{
		Audio->PlayAtLocation(TEXT("Capture"), Crew->GetActorLocation());
	}

	if (AActor* HunterActor = UGameplayStatics::GetActorOfClass(this, AManhuntHunterCharacter::StaticClass()))
	{
		if (AManhuntHunterController* HunterController = Cast<AManhuntHunterController>(Cast<APawn>(HunterActor)->GetController()))
		{
			HunterController->NotifyCaptureEvent();
		}
	}

	OnCrewCaptured.Broadcast(Crew);
}

void URescueComponent::ArrangeCapturedCrewInCage()
{
	TArray<AManhuntCrewCharacter*> Jailed;
	for (AManhuntCrewCharacter* Crew : AllCrew)
	{
		if (Crew && Crew->IsCaptured())
		{
			Jailed.Add(Crew);
		}
	}

	const FVector JailCenter = GetJailLocation();
	for (int32 i = 0; i < Jailed.Num(); ++i)
	{
		const float Angle = (float(i) / FMath::Max(1, Jailed.Num())) * 2.f * PI;
		const FVector Offset(FMath::Cos(Angle) * 130.f, FMath::Sin(Angle) * 130.f, 0.f);
		Jailed[i]->SetActorLocation(JailCenter + Offset);
	}
}

bool URescueComponent::IsPlayerInRescueRange(const APawn* PlayerPawn) const
{
	if (!PlayerPawn)
	{
		return false;
	}
	return FVector::Dist(PlayerPawn->GetActorLocation(), GetJailLocation()) < (GetJailRadius() + 160.f);
}

bool URescueComponent::TryRescue(APawn* PlayerPawn)
{
	if (!IsPlayerInRescueRange(PlayerPawn))
	{
		return false;
	}

	TArray<AManhuntCrewCharacter*> Jailed;
	for (AManhuntCrewCharacter* Crew : AllCrew)
	{
		if (Crew && Crew->IsCaptured())
		{
			Jailed.Add(Crew);
		}
	}

	if (Jailed.Num() == 0)
	{
		return false;
	}

	const FVector JailCenter = GetJailLocation();
	const float ReleaseRadius = GetJailRadius() + 140.f;

	for (int32 i = 0; i < Jailed.Num(); ++i)
	{
		const float Angle = (float(i) * 2.f * PI) / FMath::Max(1, Jailed.Num());
		const FVector ReleasePos = JailCenter + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * ReleaseRadius;
		Jailed[i]->SetActorLocation(ReleasePos);
		Jailed[i]->SetCaptured(false);
		// TODO(animation): trigger the rescue reaction montage here -- see
		// /docs/ANIMATION_SPEC.md Section 3 ("Rescue (reaction)").
	}

	if (UManhuntAudioManager* Audio = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UManhuntAudioManager>() : nullptr)
	{
		Audio->PlayAtLocation(TEXT("Rescue"), JailCenter);
	}

	OnRescuePerformed.Broadcast(Jailed.Num());
	return true;
}

int32 URescueComponent::GetJailedCount() const
{
	int32 Count = 0;
	for (const AManhuntCrewCharacter* Crew : AllCrew)
	{
		if (Crew && Crew->IsCaptured())
		{
			++Count;
		}
	}
	return Count;
}

int32 URescueComponent::GetFreeCount() const
{
	return AllCrew.Num() - GetJailedCount();
}

void URescueComponent::ResetForNewRound()
{
	for (AManhuntCrewCharacter* Crew : AllCrew)
	{
		if (Crew)
		{
			Crew->SetCaptured(false);
		}
	}
}

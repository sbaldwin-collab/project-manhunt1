#include "Components/ManhuntCameraComponent.h"
#include "Data/ManhuntGameSettings.h"
#include "ProjectManhunt.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Kismet/GameplayStatics.h"

UManhuntCameraComponent::UManhuntCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UManhuntCameraComponent::InitializeCamera(USpringArmComponent* InSpringArm, UCameraComponent* InCamera)
{
	SpringArm = InSpringArm;
	Camera = InCamera;

	if (!SpringArm || !Camera)
	{
		UE_LOG(LogManhunt, Warning, TEXT("UManhuntCameraComponent::InitializeCamera missing a SpringArm or Camera reference."));
		return;
	}

	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();

	SpringArm->TargetArmLength = Settings->CameraArmLengthCm;
	SpringArm->SocketOffset = FVector(0.f, Settings->CameraShoulderOffsetCm, 0.f);
	SpringArm->bDoCollisionTest = true;
	SpringArm->bEnableCameraLag = true;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraLagSpeed = 10.f;
	SpringArm->CameraRotationLagSpeed = 10.f;

	Camera->SetFieldOfView(Settings->CameraFovBase);
}

void UManhuntCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Camera)
	{
		return;
	}

	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	const float TargetFov = bIsSprinting ? Settings->CameraFovSprint : Settings->CameraFovBase;
	const float NewFov = FMath::FInterpTo(Camera->FieldOfView, TargetFov, DeltaTime, Settings->CameraFovInterpSpeed);
	Camera->SetFieldOfView(NewFov);
}

void UManhuntCameraComponent::PlayChaseShake()
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (ChaseShakeClass)
		{
			PC->ClientStartCameraShake(ChaseShakeClass);
		}
		else
		{
			UE_LOG(LogManhunt, Verbose, TEXT("ChaseShakeClass not set on UManhuntCameraComponent -- assign a UCameraShakeBase Blueprint in the character defaults."));
		}
	}
}

void UManhuntCameraComponent::PlayCaptureShake()
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (CaptureShakeClass)
		{
			PC->ClientStartCameraShake(CaptureShakeClass);
		}
	}
}

void UManhuntCameraComponent::PlayCinematicIntro_Implementation()
{
	// Intentionally left as a hand-off point: the round-open cinematic
	// should be authored as a Level Sequence (e.g. BP_RoundIntroSequence)
	// and played via ULevelSequencePlayer here once that asset exists.
	// See /docs/UNREAL_MIGRATION_PLAN.md Section 7.
	UE_LOG(LogManhunt, Log, TEXT("PlayCinematicIntro: no Level Sequence wired up yet -- see ManhuntCameraComponent.cpp."));
}

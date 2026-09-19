#include "Characters/ManhuntPlayerCharacter.h"
#include "Components/StaminaComponent.h"
#include "Components/ManhuntCameraComponent.h"
#include "Data/ManhuntGameSettings.h"
#include "Gameplay/ClimbVolume.h"
#include "GameModes/ManhuntGameState.h"
#include "Gameplay/RescueComponent.h"
#include "Audio/ManhuntAudioManager.h"
#include "ProjectManhunt.h"

#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "MotionWarpingComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Kismet/GameplayStatics.h"

AManhuntPlayerCharacter::AManhuntPlayerCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	CameraBehavior = CreateDefaultSubobject<UManhuntCameraComponent>(TEXT("CameraBehavior"));
	Stamina = CreateDefaultSubobject<UStaminaComponent>(TEXT("Stamina"));
	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f); // fallback rotation; a turn-in-place
	                                                                    // Anim state machine should own facing
	                                                                    // once real animation exists -- see
	                                                                    // /docs/ANIMATION_SPEC.md Section 5.
}

void AManhuntPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	CameraBehavior->InitializeCamera(CameraBoom, FollowCamera);
}

void AManhuntPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Classic Axis/Action bindings -- see the class header comment and
	// Config/DefaultInput.ini for the matching ini mappings.
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AManhuntPlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AManhuntPlayerCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AManhuntPlayerCharacter::OnSprintPressed);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AManhuntPlayerCharacter::OnSprintReleased);
	PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &AManhuntPlayerCharacter::OnCrouchToggle);
	PlayerInputComponent->BindAction(TEXT("Decoy"), IE_Pressed, this, &AManhuntPlayerCharacter::OnDecoyPressed);
	PlayerInputComponent->BindAction(TEXT("Rescue"), IE_Pressed, this, &AManhuntPlayerCharacter::OnRescuePressed);
	PlayerInputComponent->BindAction(TEXT("Pause"), IE_Pressed, this, &AManhuntPlayerCharacter::OnPausePressed);
}

void AManhuntPlayerCharacter::MoveForward(float Value)
{
	if (Controller && Value != 0.f && !bIsClimbing)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
	}
}

void AManhuntPlayerCharacter::MoveRight(float Value)
{
	if (Controller && Value != 0.f && !bIsClimbing)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
	}
}

void AManhuntPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsClimbing)
	{
		return; // root-motion montage owns movement for the duration -- see BeginClimb().
	}

	UpdateLocomotionAndSpeed(DeltaTime);
	CameraBehavior->SetSprinting(bIsSprinting);
}

void AManhuntPlayerCharacter::UpdateLocomotionAndSpeed(float DeltaTime)
{
	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	const FVector Velocity = GetVelocity();
	const bool bIsMoving = !Velocity.IsNearlyZero(10.f);

	Stamina->UpdateLocomotionState(bWantsSprint, bWantsCrouch, bIsMoving);
	bIsSprinting = Stamina->CanSprint();

	if (bWantsCrouch)
	{
		GetCharacterMovement()->MaxWalkSpeed = Settings->PlayerCrouchSpeed;
		SetLocomotionState(bIsMoving ? EManhuntLocomotionState::CrouchWalk : EManhuntLocomotionState::Crouch);
	}
	else if (bIsSprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = Settings->PlayerSprintSpeed;
		SetLocomotionState(EManhuntLocomotionState::Sprint);

		// src/game/PlayerController.js: `Math.random() < sprintNoiseChancePerSec * dt`
		if (FMath::FRand() < Settings->SprintNoiseChancePerSec * DeltaTime)
		{
			UAISense_Hearing::ReportNoiseEvent(this, GetActorLocation(), /*Loudness=*/0.6f, this, Settings->HunterHearingRadiusCm);
		}
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = Settings->PlayerWalkSpeed;
		SetLocomotionState(bIsMoving ? EManhuntLocomotionState::Walk : EManhuntLocomotionState::Idle);
	}
}

void AManhuntPlayerCharacter::OnDecoyPressed()
{
	ThrowDecoy();
}

void AManhuntPlayerCharacter::ThrowDecoy()
{
	// src/game/PlayerController.js `throwDecoy()`: a point ~8m behind the
	// player's facing direction. Loudness 1.4 (browser value) is louder /
	// farther-reaching than an ordinary sprint-noise ping (0.6).
	const FVector DecoyLocation = GetActorLocation() - (GetActorForwardVector() * 800.f);
	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	UAISense_Hearing::ReportNoiseEvent(this, DecoyLocation, /*Loudness=*/1.4f, this, Settings->HunterHearingRadiusCm * 1.5f);

	UE_LOG(LogManhunt, Log, TEXT("Decoy thrown at %s"), *DecoyLocation.ToString());
	if (UManhuntAudioManager* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UManhuntAudioManager>() : nullptr)
	{
		Audio->PlayAtLocation(TEXT("Decoy"), DecoyLocation);
	}
}

void AManhuntPlayerCharacter::OnRescuePressed()
{
	TryRescue();
}

void AManhuntPlayerCharacter::TryRescue()
{
	if (AManhuntGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AManhuntGameState>() : nullptr)
	{
		if (URescueComponent* Rescue = GameState->GetRescueComponent())
		{
			Rescue->TryRescue(this);
		}
	}
}

void AManhuntPlayerCharacter::OnPausePressed()
{
	// Hand-off point: route to the pause menu / UManhuntGameSettings-driven
	// pause flow once the UMG HUD (port of src/ui/HUD.js) exists.
	UGameplayStatics::SetGamePaused(this, !UGameplayStatics::IsGamePaused(this));
}

void AManhuntPlayerCharacter::BeginClimb(AManhuntClimbVolume* ClimbVolume, bool bClimbingUp)
{
	if (bIsClimbing || !ClimbVolume)
	{
		return;
	}

	bIsClimbing = true;
	SetLocomotionState(EManhuntLocomotionState::Climbing);

	// Hand-off point: play the climb/mantle root-motion montage here, with
	// Motion Warping targets set from ClimbVolume's ground/roof transforms
	// (see /docs/ANIMATION_SPEC.md Section 6). On montage completion, call
	// SetRoofLayer(bClimbingUp ? ClimbVolume->GetRoofLayerId() : NAME_None)
	// and clear bIsClimbing. Left unimplemented here because it depends on
	// the actual climb montage asset, which does not exist in this scaffold.
	UE_LOG(LogManhunt, Log, TEXT("BeginClimb: %s, climbing %s -- montage playback not yet wired up."),
		*ClimbVolume->GetName(), bClimbingUp ? TEXT("up") : TEXT("down"));
}

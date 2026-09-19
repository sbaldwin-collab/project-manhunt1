#include "Characters/ManhuntHunterCharacter.h"
#include "Data/ManhuntGameSettings.h"
#include "Components/SpotLightComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AManhuntHunterCharacter::AManhuntHunterCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(46.f, 96.f);

	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(GetMesh(), TEXT("hand_r")); // socket name depends on the final skeleton -- verify in-editor.
	Flashlight->Intensity = 4000.f;
	Flashlight->AttenuationRadius = 1800.f;
	Flashlight->InnerConeAngle = 12.f;
	Flashlight->OuterConeAngle = 22.f;
	Flashlight->LightColor = FColor(255, 224, 163); // warm sodium-adjacent tone, matches src/game/HunterAI.js's 0xffe0a3 beam color.
	Flashlight->CastShadows = true;

	GetCharacterMovement()->bOrientRotationToMovement = true;

	// UManhuntGameSettings::HunterSpeedPatrol is the default idle speed;
	// AManhuntHunterController overrides this per Behavior Tree state.
}

void AManhuntHunterCharacter::SetMovementSpeed(float NewSpeedCm)
{
	GetCharacterMovement()->MaxWalkSpeed = NewSpeedCm;
}

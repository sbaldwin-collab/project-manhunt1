#include "Components/StaminaComponent.h"
#include "Data/ManhuntGameSettings.h"

UStaminaComponent::UStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

bool UStaminaComponent::CanSprint() const
{
	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	return bWantsSprintCached
		&& CurrentStamina > Settings->StaminaMinToSprint
		&& !bIsCrouchedCached
		&& bIsMovingCached;
}

void UStaminaComponent::UpdateLocomotionState(bool bWantsSprint, bool bIsCrouched, bool bIsMoving)
{
	bWantsSprintCached = bWantsSprint;
	bIsCrouchedCached = bIsCrouched;
	bIsMovingCached = bIsMoving;
}

float UStaminaComponent::GetStaminaFraction() const
{
	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	return Settings->StaminaMax > 0.f ? CurrentStamina / Settings->StaminaMax : 0.f;
}

void UStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	const bool bWasSprinting = bIsSprintingCached;
	bIsSprintingCached = CanSprint();

	const float PreviousStamina = CurrentStamina;

	if (bIsSprintingCached)
	{
		// PlayerController.js: stamina = Math.max(0, stamina - sprintDrainPerSec * dt)
		CurrentStamina = FMath::Max(0.f, CurrentStamina - Settings->StaminaSprintDrainPerSec * DeltaTime);
	}
	else
	{
		// PlayerController.js: regenPerSec while moving, idleRegenPerSec while stationary
		const float RegenRate = bIsMovingCached ? Settings->StaminaRegenPerSec : Settings->StaminaIdleRegenPerSec;
		CurrentStamina = FMath::Min(Settings->StaminaMax, CurrentStamina + RegenRate * DeltaTime);
	}

	if (!FMath::IsNearlyEqual(PreviousStamina, CurrentStamina))
	{
		OnStaminaChanged.Broadcast(CurrentStamina, Settings->StaminaMax);
	}

	// bWasSprinting is available here for a future sprint-noise Anim Notify
	// hookup (see /docs/ANIMATION_SPEC.md Section 8) instead of a flat
	// per-second dice roll; left as a named local rather than removed so the
	// intent is visible to the next person wiring that up.
	(void)bWasSprinting;
}

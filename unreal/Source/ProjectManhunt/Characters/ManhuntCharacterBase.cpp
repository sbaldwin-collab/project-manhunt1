#include "Characters/ManhuntCharacterBase.h"

AManhuntCharacterBase::AManhuntCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = false;
}

void AManhuntCharacterBase::SetLocomotionState(EManhuntLocomotionState NewState)
{
	CurrentLocomotionState = NewState;
}

bool AManhuntCharacterBase::IsCrouchingForDetection_Implementation() const
{
	return CurrentLocomotionState == EManhuntLocomotionState::Crouch
		|| CurrentLocomotionState == EManhuntLocomotionState::CrouchWalk;
}

bool AManhuntCharacterBase::IsOnRooftopLayer_Implementation() const
{
	return IsOnRooftop();
}

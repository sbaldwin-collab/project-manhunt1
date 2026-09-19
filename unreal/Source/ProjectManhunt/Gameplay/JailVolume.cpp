#include "Gameplay/JailVolume.h"
#include "Components/StaticMeshComponent.h"

AJailVolume::AJailVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	PlaceholderCageMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderCageMesh"));
	RootComponent = PlaceholderCageMesh;
	PlaceholderCageMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// NOTE: no default mesh is assigned here (no binary asset reference can
	// be authored from this text-only scaffold). Assign a cylinder/cage
	// placeholder mesh on the Blueprint or per-instance in the level editor
	// until real jail cage art exists.
}

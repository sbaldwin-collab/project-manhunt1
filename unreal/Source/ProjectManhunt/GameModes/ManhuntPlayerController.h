#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ManhuntPlayerController.generated.h"

class UUserWidget;

/**
 * Hand-off point for the UMG HUD (port of src/ui/HUD.js). Creates and owns
 * the HUD widget; the widget itself binds to AManhuntGameState's delegates
 * (OnRoundStarted/OnRoundEnded), URescueComponent's delegates
 * (OnCrewCaptured/OnRescuePerformed), and UStaminaComponent's
 * OnStaminaChanged rather than polling, matching the event-driven update
 * pattern src/ui/HUD.js already used.
 */
UCLASS()
class PROJECTMANHUNT_API AManhuntPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	/** Assign the UMG HUD widget Blueprint here once authored in-editor
	 *  (port of the markup in index.html + styles/main.css). */
	UPROPERTY(EditDefaultsOnly, Category = "Manhunt|UI")
	TSubclassOf<UUserWidget> HudWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> HudWidgetInstance;
};

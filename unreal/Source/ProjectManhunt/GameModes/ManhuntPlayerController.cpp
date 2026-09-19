#include "GameModes/ManhuntPlayerController.h"
#include "ProjectManhunt.h"
#include "Blueprint/UserWidget.h"

void AManhuntPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController())
	{
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());

		if (HudWidgetClass)
		{
			HudWidgetInstance = CreateWidget<UUserWidget>(this, HudWidgetClass);
			if (HudWidgetInstance)
			{
				HudWidgetInstance->AddToViewport();
			}
		}
		else
		{
			UE_LOG(LogManhunt, Warning, TEXT("AManhuntPlayerController: HudWidgetClass is not set -- no HUD will be shown. Assign a UMG widget Blueprint (port of src/ui/HUD.js) in the Blueprint subclass of this controller."));
		}
	}
}

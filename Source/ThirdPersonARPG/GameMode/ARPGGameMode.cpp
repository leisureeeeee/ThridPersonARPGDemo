#include "ARPGGameMode.h"
#include "HUD/ARPGDebugHUD.h"
#include "MyCharacter/MyCharacter.h"
#include "PlayerController/MyPlayerController.h"

AARPGGameMode::AARPGGameMode()
{
	PlayerControllerClass = AMyPlayerController::StaticClass();
	DefaultPawnClass = AMyCharacter::StaticClass();
	HUDClass = AARPGDebugHUD::StaticClass();
}

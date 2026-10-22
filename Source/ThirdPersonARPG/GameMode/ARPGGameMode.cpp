#include "ARPGGameMode.h"
#include "HUD/ARPGDebugHUD.h"
#include "MyCharacter/MyCharacter.h"
#include "PlayerController/MyPlayerController.h"
#include "PlayerState/ARPGPlayerState.h"

AARPGGameMode::AARPGGameMode()
{
	PlayerControllerClass = AMyPlayerController::StaticClass();
	PlayerStateClass = AARPGPlayerState::StaticClass();
	DefaultPawnClass = AMyCharacter::StaticClass();
	HUDClass = AARPGDebugHUD::StaticClass();
}

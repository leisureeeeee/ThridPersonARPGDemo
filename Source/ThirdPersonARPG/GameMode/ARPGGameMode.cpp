#include "ARPGGameMode.h"
#include "HUD/ARPGDebugHUD.h"
#include "PlayerState/ARPGPlayerState.h"
#include "MyCharacter/MyCharacter.h"
#include "PlayerController/MyPlayerController.h"

AARPGGameMode::AARPGGameMode()
{
	PlayerStateClass = AARPGPlayerState::StaticClass();
	PlayerControllerClass = AMyPlayerController::StaticClass();
	DefaultPawnClass = AMyCharacter::StaticClass();
	HUDClass = AARPGDebugHUD::StaticClass();
}

#include "ARPGGameMode.h"
#include "MyCharacter/MyCharacter.h"
#include "PlayerController/MyPlayerController.h"

AARPGGameMode::AARPGGameMode()
{
	PlayerControllerClass = AMyPlayerController::StaticClass();
	DefaultPawnClass = AMyCharacter::StaticClass();
}

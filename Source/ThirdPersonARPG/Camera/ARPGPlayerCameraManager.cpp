#include "ARPGPlayerCameraManager.h"
#include "ARPGCameraRigComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

bool AARPGPlayerCameraManager::TryUseControlledPawnCamera(APawn* Pawn, float BlendTime)
{
	const APlayerController* Controller = GetOwningPlayerController();
	if (!Controller || !IsValid(Pawn) || Controller->GetPawn() != Pawn
		|| !Pawn->FindComponentByClass<UARPGCameraRigComponent>() || !FMath::IsFinite(BlendTime))
	{
		return false;
	}

	FViewTargetTransitionParams Transition;
	Transition.BlendTime = FMath::Max(0.0f, BlendTime);
	Transition.BlendFunction = VTBlend_Cubic;
	SetViewTarget(Pawn, Transition);
	return true;
}

bool AARPGPlayerCameraManager::RestoreControlledPawnCamera(float BlendTime)
{
	const APlayerController* Controller = GetOwningPlayerController();
	return Controller && TryUseControlledPawnCamera(Controller->GetPawn(), BlendTime);
}

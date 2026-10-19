#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "ARPGPlayerCameraManager.generated.h"

class APawn;

UCLASS()
class THIRDPERSONARPG_API AARPGPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	bool TryUseControlledPawnCamera(APawn* Pawn, float BlendTime = 0.0f);

	// Resolve the current pawn when returning from a cinematic or other view target.
	UFUNCTION(BlueprintCallable, Category = "Camera")
	bool RestoreControlledPawnCamera(float BlendTime = 0.2f);
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ARPGDebugHUD.generated.h"

// Screen-space camera alignment guides, independent of character/camera behavior.
UCLASS()
class THIRDPERSONARPG_API AARPGDebugHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawAttackTrace();
};

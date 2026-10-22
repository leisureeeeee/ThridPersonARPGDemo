#pragma once

#include "CoreMinimal.h"
#include "AttackWindow.generated.h"

UENUM(BlueprintType)
enum class EAttackWindow : uint8
{
    Hit,
    EvadeCancel,
    MoveCancel
};

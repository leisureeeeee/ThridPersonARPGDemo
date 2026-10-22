#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WeaponStateEffect.generated.h"

// A native definition, not a transient runtime GE. Each spec grants one state tag.
UCLASS()
class THIRDPERSONARPG_API UWeaponStateEffect : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UWeaponStateEffect();
};

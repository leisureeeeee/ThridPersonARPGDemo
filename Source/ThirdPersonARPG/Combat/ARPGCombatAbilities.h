#pragma once

#include "CoreMinimal.h"
#include "Combat/ARPGCombatAbility.h"
#include "ARPGCombatAbilities.generated.h"

UCLASS(Abstract)
class THIRDPERSONARPG_API UARPGAbility_NormalAttack : public UARPGCombatAbility
{
    GENERATED_BODY()

public:
    UARPGAbility_NormalAttack();
};


UCLASS(Abstract)
class THIRDPERSONARPG_API UARPGAbility_SpecialAttack : public UARPGCombatAbility
{
    GENERATED_BODY()

public:
    UARPGAbility_SpecialAttack();
};


UCLASS(Abstract)
class THIRDPERSONARPG_API UARPGAbility_Parry : public UARPGCombatAbility
{
    GENERATED_BODY()

public:
    UARPGAbility_Parry();
};


UCLASS(Abstract)
class THIRDPERSONARPG_API UARPGAbility_Dodge : public UARPGCombatAbility
{
    GENERATED_BODY()

public:
    UARPGAbility_Dodge();
};


UCLASS(Abstract)
class THIRDPERSONARPG_API UARPGAbility_Execution : public UARPGCombatAbility
{
    GENERATED_BODY()

public:
    UARPGAbility_Execution();
};

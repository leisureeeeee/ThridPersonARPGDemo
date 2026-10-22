#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ARPGCombatAbility.generated.h"

// Derive in C++ and implement ActivateAbility, CommitAbility, tasks and EndAbility.
// Native placeholders are abstract and cannot be granted as finished actions.
UCLASS(Abstract)
class THIRDPERSONARPG_API UARPGCombatAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UARPGCombatAbility();

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
};

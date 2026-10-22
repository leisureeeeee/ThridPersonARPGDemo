#include "Combat/ARPGCombatAbility.h"
#include "Combat/ARPGCombatTags.h"

UARPGCombatAbility::UARPGCombatAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    ActivationBlockedTags.AddTag(ARPGCombatTags::State_Dead);
    ActivationBlockedTags.AddTag(ARPGCombatTags::State_Executing);
    ActivationBlockedTags.AddTag(ARPGCombatTags::State_BeingExecuted);
    ActivationBlockedTags.AddTag(ARPGCombatTags::State_PostureBroken);
}

void UARPGCombatAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    // Fail closed if a future concrete subclass forgets to implement its action.
    UE_LOG(LogTemp, Warning, TEXT("%s: combat action is not implemented."), *GetName());
    EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
}

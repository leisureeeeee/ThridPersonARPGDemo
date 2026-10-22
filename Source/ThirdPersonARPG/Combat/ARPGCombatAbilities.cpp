#include "Combat/ARPGCombatAbilities.h"
#include "Combat/ARPGCombatTags.h"

UARPGAbility_NormalAttack::UARPGAbility_NormalAttack()
{
    SetAssetTags(FGameplayTagContainer(ARPGCombatTags::Ability_Attack_Normal));
}


UARPGAbility_SpecialAttack::UARPGAbility_SpecialAttack()
{
    SetAssetTags(FGameplayTagContainer(ARPGCombatTags::Ability_Attack_Special));
}


UARPGAbility_Parry::UARPGAbility_Parry()
{
    SetAssetTags(FGameplayTagContainer(ARPGCombatTags::Ability_Parry));
}


UARPGAbility_Dodge::UARPGAbility_Dodge()
{
    SetAssetTags(FGameplayTagContainer(ARPGCombatTags::Ability_Dodge));
}


UARPGAbility_Execution::UARPGAbility_Execution()
{
    SetAssetTags(FGameplayTagContainer(ARPGCombatTags::Ability_Execution));
    // Execution is requested by a future authoritative resolver, never by an input binding.
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    FAbilityTriggerData Trigger;
    Trigger.TriggerTag = ARPGCombatTags::Event_ExecutionRequested;
    Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(Trigger);
}

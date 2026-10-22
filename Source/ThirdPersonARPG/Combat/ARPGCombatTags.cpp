#include "Combat/ARPGCombatTags.h"

namespace ARPGCombatTags
{
    UE_DEFINE_GAMEPLAY_TAG(Event_Root, "ARPG.Event");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Normal, "ARPG.Ability.Attack.Normal");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Special, "ARPG.Ability.Attack.Special");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Parry, "ARPG.Ability.Parry");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Dodge, "ARPG.Ability.Dodge");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Execution, "ARPG.Ability.Execution");
    UE_DEFINE_GAMEPLAY_TAG(State_Dead, "ARPG.State.Dead");
    UE_DEFINE_GAMEPLAY_TAG(State_Attacking, "ARPG.State.Attacking");
    UE_DEFINE_GAMEPLAY_TAG(State_Parrying, "ARPG.State.Parrying");
    UE_DEFINE_GAMEPLAY_TAG(State_PerfectParryWindow, "ARPG.State.Parry.PerfectWindow");
    UE_DEFINE_GAMEPLAY_TAG(State_Recovery, "ARPG.State.Recovery");
    UE_DEFINE_GAMEPLAY_TAG(State_Dodging, "ARPG.State.Dodging");
    UE_DEFINE_GAMEPLAY_TAG(State_Invulnerable, "ARPG.State.Invulnerable");
    UE_DEFINE_GAMEPLAY_TAG(State_PostureBroken, "ARPG.State.PostureBroken");
    UE_DEFINE_GAMEPLAY_TAG(State_Executing, "ARPG.State.Executing");
    UE_DEFINE_GAMEPLAY_TAG(State_BeingExecuted, "ARPG.State.BeingExecuted");
    UE_DEFINE_GAMEPLAY_TAG(Event_HitReceived, "ARPG.Event.HitReceived");
    UE_DEFINE_GAMEPLAY_TAG(Event_PerfectParry, "ARPG.Event.PerfectParry");
    UE_DEFINE_GAMEPLAY_TAG(Event_PostureBroken, "ARPG.Event.PostureBroken");
    UE_DEFINE_GAMEPLAY_TAG(Event_ExecutionRequested, "ARPG.Event.Execution.Requested");
    UE_DEFINE_GAMEPLAY_TAG(Event_ExecutionCompleted, "ARPG.Event.Execution.Completed");
    UE_DEFINE_GAMEPLAY_TAG(Event_PerfectParryWindowOpened, "ARPG.Event.Parry.PerfectWindowOpened");
    UE_DEFINE_GAMEPLAY_TAG(Event_PerfectParryWindowClosed, "ARPG.Event.Parry.PerfectWindowClosed");
    UE_DEFINE_GAMEPLAY_TAG(Event_RecoveryFinished, "ARPG.Event.RecoveryFinished");
    UE_DEFINE_GAMEPLAY_TAG(Data_HealthDamage, "ARPG.Data.HealthDamage");
    UE_DEFINE_GAMEPLAY_TAG(Data_PostureDamage, "ARPG.Data.PostureDamage");
    UE_DEFINE_GAMEPLAY_TAG(Data_EnergyAmount, "ARPG.Data.EnergyAmount");
}

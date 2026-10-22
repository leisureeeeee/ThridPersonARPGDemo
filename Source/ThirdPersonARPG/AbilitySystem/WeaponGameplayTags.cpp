#include "AbilitySystem/WeaponGameplayTags.h"

namespace WeaponGameplayTags
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Attack_WindowBegin, "Event.Attack.WindowBegin", "Montage attack window begins.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Attack_WindowEnd, "Event.Attack.WindowEnd", "Montage attack window ends.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Window_Attack_Hit, "Window.Attack.Hit", "Blade contact sampling is enabled.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Window_Cancel_Evade, "Window.Cancel.Evade", "Attack permits evade cancellation.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Window_Cancel_Move, "Window.Cancel.Move", "Attack permits movement cancellation.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Evade, "Ability.Evade", "Grounded directional evade.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Action_Evade, "Event.Action.Evade", "Evade request with a captured direction sector.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Action_Evading, "State.Action.Evading", "Evade ability owns the full-body action.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack_Light, "Ability.Attack.Light", "Single grounded light attack.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Action_Attacking, "State.Action.Attacking", "Attack ability owns the full-body action.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Weapon_Drawn, "State.Weapon.Drawn", "Weapon is committed to the hand socket.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Weapon_Sheathed, "State.Weapon.Sheathed", "Weapon is committed to the sheath socket.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Action_Drawing, "State.Action.Drawing", "Draw montage is in progress.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Action_Sheathing, "State.Action.Sheathing", "Sheathe montage is in progress.");
}

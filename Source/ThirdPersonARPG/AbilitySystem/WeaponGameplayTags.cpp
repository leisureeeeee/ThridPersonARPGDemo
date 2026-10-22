#include "AbilitySystem/WeaponGameplayTags.h"

namespace WeaponGameplayTags
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Weapon_Drawn, "State.Weapon.Drawn", "Weapon is committed to the hand socket.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Weapon_Sheathed, "State.Weapon.Sheathed", "Weapon is committed to the sheath socket.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Action_Drawing, "State.Action.Drawing", "Draw montage is in progress.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Action_Sheathing, "State.Action.Sheathing", "Sheathe montage is in progress.");
}

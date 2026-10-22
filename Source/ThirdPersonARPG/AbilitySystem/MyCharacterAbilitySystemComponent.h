#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AbilitySystemComponentBase.h"
#include "MyCharacterAbilitySystemComponent.generated.h"

class AMyCharacter;

// Player-specific state ownership; the shared ASC base and Boss stay unchanged.
UCLASS()
class THIRDPERSONARPG_API UMyCharacterAbilitySystemComponent : public UAbilitySystemComponentBase
{
    GENERATED_BODY()

private:
    friend class AMyCharacter;

    void BindWeaponAvatar(AMyCharacter* Character);
    void ReleaseWeaponAvatar(const AMyCharacter* ExpectedAvatar);
    bool CanChangeWeaponState(const AMyCharacter* ExpectedAvatar) const;
    bool SetWeaponDrawn(const AMyCharacter* ExpectedAvatar, bool bDrawn);
    bool SetWeaponAction(const AMyCharacter* ExpectedAvatar, const FGameplayTag& ActionTag);
    bool ReplaceStateEffect(FActiveGameplayEffectHandle& Handle, const FGameplayTag& Tag);
    void ClearWeaponEffects();
    bool RequestLightAttack(const AMyCharacter* ExpectedAvatar);
    void CancelLightAttack();

    // Handles identify this subsystem's effects; the actual state lives in GAS tags.
    FGameplayAbilitySpecHandle LightAttackHandle;
    FActiveGameplayEffectHandle WeaponStateEffect;
    FActiveGameplayEffectHandle WeaponActionEffect;
};

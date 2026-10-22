#include "AbilitySystem/MyCharacterAbilitySystemComponent.h"
#include "AbilitySystem/WeaponGameplayTags.h"
#include "AbilitySystem/Abilities/LightAttackAbility.h"
#include "AbilitySystem/WeaponStateEffect.h"
#include "MyCharacter/MyCharacter.h"

void UMyCharacterAbilitySystemComponent::BindWeaponAvatar(AMyCharacter* Character)
{
    if (GetAvatarActor() != Character)
    {
        CancelLightAttack();
        ClearWeaponEffects();
    }
    InitAbilityActorInfo(GetOwner(), Character);
    if (GetOwner()->HasAuthority() && !FindAbilitySpecFromHandle(LightAttackHandle))
    {
        LightAttackHandle = GiveAbility(FGameplayAbilitySpec(ULightAttackAbility::StaticClass(), 1));
    }
}

void UMyCharacterAbilitySystemComponent::ReleaseWeaponAvatar(const AMyCharacter* ExpectedAvatar)
{
    // An old pawn ending play must not clear a newly possessed pawn's state.
    if (GetAvatarActor() == ExpectedAvatar)
    {
        CancelLightAttack();
        ClearWeaponEffects();
        ClearActorInfo();
    }
}

bool UMyCharacterAbilitySystemComponent::CanChangeWeaponState(const AMyCharacter* ExpectedAvatar) const
{
    return IsValid(ExpectedAvatar) && GetAvatarActor() == ExpectedAvatar
        && GetOwner() && GetOwner()->HasAuthority();
}

bool UMyCharacterAbilitySystemComponent::ReplaceStateEffect(
    FActiveGameplayEffectHandle& Handle, const FGameplayTag& Tag)
{
    // Create the replacement first so an application failure preserves the old state.
    FActiveGameplayEffectHandle Replacement;
    if (Tag.IsValid())
    {
        FGameplayEffectSpec Spec(GetDefault<UWeaponStateEffect>(), MakeEffectContext(), 1.f);
        Spec.DynamicGrantedTags.AddTag(Tag);
        Replacement = ApplyGameplayEffectSpecToSelf(Spec);
        if (!Replacement.IsValid())
        {
            return false;
        }
    }
    RemoveActiveGameplayEffect(Handle);
    Handle = Replacement;
    return true;
}

bool UMyCharacterAbilitySystemComponent::SetWeaponDrawn(const AMyCharacter* ExpectedAvatar, bool bDrawn)
{
    return CanChangeWeaponState(ExpectedAvatar) && ReplaceStateEffect(WeaponStateEffect,
        bDrawn ? WeaponGameplayTags::State_Weapon_Drawn : WeaponGameplayTags::State_Weapon_Sheathed);
}

bool UMyCharacterAbilitySystemComponent::SetWeaponAction(
    const AMyCharacter* ExpectedAvatar, const FGameplayTag& ActionTag)
{
    if (ActionTag.IsValid() && ActionTag != WeaponGameplayTags::State_Action_Drawing
        && ActionTag != WeaponGameplayTags::State_Action_Sheathing)
    {
        return false;
    }
    return CanChangeWeaponState(ExpectedAvatar) && ReplaceStateEffect(WeaponActionEffect, ActionTag);
}

void UMyCharacterAbilitySystemComponent::ClearWeaponEffects()
{
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        RemoveActiveGameplayEffect(WeaponActionEffect);
        RemoveActiveGameplayEffect(WeaponStateEffect);
        WeaponActionEffect.Invalidate();
        WeaponStateEffect.Invalidate();
    }
}

bool UMyCharacterAbilitySystemComponent::RequestLightAttack(const AMyCharacter* ExpectedAvatar)
{
    return CanChangeWeaponState(ExpectedAvatar) && LightAttackHandle.IsValid()
        && TryActivateAbility(LightAttackHandle, false);
}

void UMyCharacterAbilitySystemComponent::CancelLightAttack()
{
    if (LightAttackHandle.IsValid())
    {
        CancelAbilityHandle(LightAttackHandle);
    }
}

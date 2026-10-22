#include "AbilitySystem/MyCharacterAbilitySystemComponent.h"
#include "AbilitySystem/WeaponGameplayTags.h"
#include "AbilitySystem/WeaponStateEffect.h"
#include "MyCharacter/MyCharacter.h"

void UMyCharacterAbilitySystemComponent::BindWeaponAvatar(AMyCharacter* Character)
{
    if (GetAvatarActor() != Character)
    {
        ClearWeaponEffects();
    }
    InitAbilityActorInfo(GetOwner(), Character);
}

void UMyCharacterAbilitySystemComponent::ReleaseWeaponAvatar(const AMyCharacter* ExpectedAvatar)
{
    // An old pawn ending play must not clear a newly possessed pawn's state.
    if (GetAvatarActor() == ExpectedAvatar)
    {
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

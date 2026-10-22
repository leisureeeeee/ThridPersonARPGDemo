#include "AbilitySystem/MyCharacterAbilitySystemComponent.h"
#include "AbilitySystem/WeaponGameplayTags.h"
#include "AbilitySystem/Abilities/LightAttackAbility.h"
#include "AbilitySystem/Abilities/EvadeAbility.h"
#include "AbilitySystem/WeaponStateEffect.h"
#include "MyCharacter/MyCharacter.h"

void UMyCharacterAbilitySystemComponent::BindWeaponAvatar(AMyCharacter* Character)
{
    if (GetAvatarActor() != Character)
    {
        CancelCharacterActions();
        ClearWeaponEffects();
    }
    InitAbilityActorInfo(GetOwner(), Character);
    if (GetOwner()->HasAuthority() && !FindAbilitySpecFromHandle(LightAttackHandle))
    {
        LightAttackHandle = GiveAbility(FGameplayAbilitySpec(ULightAttackAbility::StaticClass(), 1));
    }
    if (GetOwner()->HasAuthority() && !FindAbilitySpecFromHandle(EvadeHandle))
    {
        EvadeHandle = GiveAbility(FGameplayAbilitySpec(UEvadeAbility::StaticClass(), 1));
    }
}

void UMyCharacterAbilitySystemComponent::ReleaseWeaponAvatar(const AMyCharacter* ExpectedAvatar)
{
    // An old pawn ending play must not clear a newly possessed pawn's state.
    if (GetAvatarActor() == ExpectedAvatar)
    {
        CancelCharacterActions();
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

void UMyCharacterAbilitySystemComponent::CancelCharacterActions()
{
    if (LightAttackHandle.IsValid())
    {
        CancelAbilityHandle(LightAttackHandle);
    }
    if (EvadeHandle.IsValid())
    {
        CancelAbilityHandle(EvadeHandle);
    }
}

bool UMyCharacterAbilitySystemComponent::RequestEvade(
    const AMyCharacter* ExpectedAvatar, const FVector& WorldDirection)
{
    if (!CanChangeWeaponState(ExpectedAvatar) || !EvadeHandle.IsValid() || WorldDirection.ContainsNaN())
    {
        return false;
    }
    int32 Sector = 4; // No movement intent: step backward relative to current facing.
    if (!WorldDirection.IsNearlyZero())
    {
        const FVector LocalDirection = ExpectedAvatar->GetActorTransform()
            .InverseTransformVectorNoScale(WorldDirection.GetSafeNormal2D());
        const float Angle = FMath::RadiansToDegrees(FMath::Atan2(LocalDirection.Y, LocalDirection.X));
        Sector = (FMath::RoundToInt(Angle / 45.f) + 8) % 8;
    }
    FGameplayEventData Event;
    Event.EventTag = WeaponGameplayTags::Event_Action_Evade;
    Event.EventMagnitude = static_cast<float>(Sector);
    return HandleGameplayEvent(Event.EventTag, &Event) > 0;
}

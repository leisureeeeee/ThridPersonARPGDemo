#include "AbilitySystem/Abilities/LightAttackAbility.h"
#include "AbilitySystem/WeaponGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MyCharacter/MyCharacter.h"

ULightAttackAbility::ULightAttackAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    bRetriggerInstancedAbility = false;
    SetAssetTags(FGameplayTagContainer(WeaponGameplayTags::Ability_Attack_Light));
    ActivationRequiredTags.AddTag(WeaponGameplayTags::State_Weapon_Drawn);
    ActivationBlockedTags.AddTag(WeaponGameplayTags::State_Action_Drawing);
    ActivationBlockedTags.AddTag(WeaponGameplayTags::State_Action_Sheathing);
    ActivationBlockedTags.AddTag(WeaponGameplayTags::State_Action_Attacking);
    ActivationOwnedTags.AddTag(WeaponGameplayTags::State_Action_Attacking);
}

bool ULightAttackAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }
    const AMyCharacter* Character = ActorInfo ? Cast<AMyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    if (!Character || !Character->GetCurrentWeapon()
        || !Character->GetCharacterMovement()->IsMovingOnGround())
    {
        return false;
    }
    const USkeletalMeshComponent* Mesh = Character->GetMesh();
    const UAnimInstance* Animation = Mesh ? Mesh->GetAnimInstance() : nullptr;
    const UAnimMontage* Montage = AttackMontage.LoadSynchronous();
    return Animation && !Animation->IsAnyMontagePlaying() && Montage && Mesh->GetSkeletalMeshAsset()
        && Montage->GetSkeleton() == Mesh->GetSkeletalMeshAsset()->GetSkeleton();
}

void ULightAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    AMyCharacter* Character = ActorInfo ? Cast<AMyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    UAnimMontage* Montage = AttackMontage.LoadSynchronous();
    if (!Character || !Montage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // Stop voluntary locomotion; montage root motion still drives collision-aware movement.
    Character->ConsumeMovementInputVector();
    Character->GetCharacterMovement()->StopMovementImmediately();
    const float Rate = FMath::IsFinite(PlayRate) ? FMath::Clamp(PlayRate, 0.1f, 3.f) : 1.f;
    UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, Montage, Rate, NAME_None, true, 1.f, 0.f, true);
    if (!Task)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    Task->OnCompleted.AddDynamic(this, &ULightAttackAbility::OnAttackCompleted);
    Task->OnInterrupted.AddDynamic(this, &ULightAttackAbility::OnAttackCancelled);
    Task->OnCancelled.AddDynamic(this, &ULightAttackAbility::OnAttackCancelled);
    Task->ReadyForActivation();
}

void ULightAttackAbility::OnAttackCompleted()
{
    if (IsActive())
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

void ULightAttackAbility::OnAttackCancelled()
{
    if (IsActive())
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
    }
}

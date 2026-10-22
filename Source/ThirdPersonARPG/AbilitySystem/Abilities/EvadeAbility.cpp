#include "AbilitySystem/Abilities/EvadeAbility.h"
#include "AbilitySystem/WeaponGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MyCharacter/MyCharacter.h"
#include "AbilitySystemComponent.h"

UEvadeAbility::UEvadeAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    bRetriggerInstancedAbility = false;
    SetAssetTags(FGameplayTagContainer(WeaponGameplayTags::Ability_Evade));
    ActivationBlockedTags.AddTag(WeaponGameplayTags::State_Action_Drawing);
    ActivationBlockedTags.AddTag(WeaponGameplayTags::State_Action_Sheathing);
    ActivationBlockedTags.AddTag(WeaponGameplayTags::State_Action_Evading);
    ActivationOwnedTags.AddTag(WeaponGameplayTags::State_Action_Evading);

    FAbilityTriggerData Trigger;
    Trigger.TriggerTag = WeaponGameplayTags::Event_Action_Evade;
    Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(Trigger);
}

bool UEvadeAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }
    const AMyCharacter* Character = ActorInfo ? Cast<AMyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    const UAnimInstance* Animation = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
    const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    const bool bAttacking = ASC && ASC->HasMatchingGameplayTag(WeaponGameplayTags::State_Action_Attacking);
    const bool bCanCancelAttack = bAttacking && ASC->HasMatchingGameplayTag(WeaponGameplayTags::Window_Cancel_Evade);
    return Character && Character->GetCharacterMovement()->IsMovingOnGround()
        && EvadeMontages.Num() == 8 && Animation
        && (bAttacking ? bCanCancelAttack : !Animation->IsAnyMontagePlaying());
}

void UEvadeAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    // Gameplay event magnitude carries the sector captured at the input request.
    int32 DirectionIndex = 4;
    if (TriggerEventData && TriggerEventData->EventMagnitude >= 0.f && TriggerEventData->EventMagnitude <= 7.f)
    {
        DirectionIndex = FMath::RoundToInt(TriggerEventData->EventMagnitude);
    }
    AMyCharacter* Character = ActorInfo ? Cast<AMyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    UAnimMontage* Montage = EvadeMontages.IsValidIndex(DirectionIndex)
        ? EvadeMontages[DirectionIndex].LoadSynchronous() : nullptr;
    const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
    if (!Character || !Montage || !Mesh || !Mesh->GetSkeletalMeshAsset()
        || Montage->GetSkeleton() != Mesh->GetSkeletalMeshAsset()->GetSkeleton()
        || !CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    FGameplayTagContainer AttackTags(WeaponGameplayTags::Ability_Attack_Light);
    ActorInfo->AbilitySystemComponent->CancelAbilities(&AttackTags);
    Character->ConsumeMovementInputVector();
    Character->GetCharacterMovement()->StopMovementImmediately();
    const float Rate = FMath::IsFinite(PlayRate) ? FMath::Clamp(PlayRate, .1f, 3.f) : 1.f;
    const float TranslationScale = FMath::IsFinite(RootMotionTranslationScale)
        ? FMath::Clamp(RootMotionTranslationScale, 0.f, 2.f) : 1.f;
    UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, Montage, Rate, NAME_None, true, TranslationScale, 0.f, true);
    if (!Task)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    Task->OnCompleted.AddDynamic(this, &UEvadeAbility::OnEvadeCompleted);
    Task->OnInterrupted.AddDynamic(this, &UEvadeAbility::OnEvadeCancelled);
    Task->OnCancelled.AddDynamic(this, &UEvadeAbility::OnEvadeCancelled);
    Task->ReadyForActivation();
}

void UEvadeAbility::OnEvadeCompleted()
{
    if (IsActive())
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

void UEvadeAbility::OnEvadeCancelled()
{
    if (IsActive())
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
    }
}

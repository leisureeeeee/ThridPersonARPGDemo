#include "AbilitySystem/Abilities/LightAttackAbility.h"
#include "AbilitySystem/WeaponGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MyCharacter/MyCharacter.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Combat/AttackTraceComponent.h"

namespace
{
    FGameplayTag GetWindowTag(int32 Window)
    {
        switch (static_cast<EAttackWindow>(Window))
        {
        case EAttackWindow::Hit: return WeaponGameplayTags::Window_Attack_Hit;
        case EAttackWindow::EvadeCancel: return WeaponGameplayTags::Window_Cancel_Evade;
        case EAttackWindow::MoveCancel: return WeaponGameplayTags::Window_Cancel_Move;
        default: return FGameplayTag();
        }
    }
}

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
    ActivationBlockedTags.AddTag(WeaponGameplayTags::State_Action_Evading);
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
    bEnding = false;
    AMyCharacter* Character = ActorInfo ? Cast<AMyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    UAnimMontage* Montage = AttackMontage.LoadSynchronous();
    if (!Character || !Montage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    OpenWindows = 0;
    TraceComponent = Character->GetAttackTrace();
    TraceComponent->BeginAttack(Montage);
    TraceComponent->OnMoveCancel.AddUObject(this, &ULightAttackAbility::OnMoveCancel);
    UAbilityTask_WaitGameplayEvent* BeginTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this, WeaponGameplayTags::Event_Attack_WindowBegin, nullptr, false, true);
    BeginTask->EventReceived.AddDynamic(this, &ULightAttackAbility::OnWindowBegin);
    BeginTask->ReadyForActivation();
    UAbilityTask_WaitGameplayEvent* EndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this, WeaponGameplayTags::Event_Attack_WindowEnd, nullptr, false, true);
    EndTask->EventReceived.AddDynamic(this, &ULightAttackAbility::OnWindowEnd);
    EndTask->ReadyForActivation();

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
    if (IsActive() && !bEnding)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

void ULightAttackAbility::OnAttackCancelled()
{
    if (IsActive() && !bEnding)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
    }
}

void ULightAttackAbility::OnWindowBegin(FGameplayEventData Event)
{
    UpdateWindow(Event, true);
}

void ULightAttackAbility::OnWindowEnd(FGameplayEventData Event)
{
    UpdateWindow(Event, false);
}

void ULightAttackAbility::UpdateWindow(const FGameplayEventData& Event, bool bOpen)
{
    if (!IsActive() || bEnding || Event.OptionalObject != AttackMontage.Get()
        || !FMath::IsFinite(Event.EventMagnitude) || Event.EventMagnitude < 0.f || Event.EventMagnitude > 2.f)
    {
        return;
    }
    const int32 Index = FMath::RoundToInt(Event.EventMagnitude);
    const uint8 Mask = 1 << Index;
    if (((OpenWindows & Mask) != 0) == bOpen)
    {
        return;
    }
    OpenWindows = bOpen ? OpenWindows | Mask : OpenWindows & ~Mask;
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        if (bOpen) { ASC->AddLooseGameplayTag(GetWindowTag(Index)); }
        else { ASC->RemoveLooseGameplayTag(GetWindowTag(Index)); }
    }
    if (TraceComponent.IsValid())
    {
        TraceComponent->SetWindow(static_cast<EAttackWindow>(Index), bOpen);
    }
}

void ULightAttackAbility::OnMoveCancel()
{
    if (IsActive() && !bEnding && (OpenWindows & (1 << static_cast<uint8>(EAttackWindow::MoveCancel))))
    {
        // Ending the montage task initiates the montage's configured blend-out.
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
    }
}

void ULightAttackAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility, bool bWasCancelled)
{
    if (!IsEndAbilityValid(Handle, ActorInfo) || bEnding)
    {
        return;
    }
    bEnding = true;
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        for (int32 Index = 0; Index != 3; ++Index)
        {
            if (OpenWindows & (1 << Index))
            {
                ASC->RemoveLooseGameplayTag(GetWindowTag(Index));
            }
        }
    }
    OpenWindows = 0;
    if (TraceComponent.IsValid())
    {
        TraceComponent->OnMoveCancel.RemoveAll(this);
        TraceComponent->EndAttack(!bWasCancelled);
        TraceComponent.Reset();
    }
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

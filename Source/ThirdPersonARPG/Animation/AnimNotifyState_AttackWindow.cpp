#include "Animation/AnimNotifyState_AttackWindow.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/WeaponGameplayTags.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

UAnimNotifyState_AttackWindow::UAnimNotifyState_AttackWindow()
{
    bIsNativeBranchingPoint = true;
#if WITH_EDITORONLY_DATA
    bShouldFireInEditor = false;
#endif
}

void UAnimNotifyState_AttackWindow::NotifyBegin(USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
    SendEvent(MeshComp, Animation, true);
}

void UAnimNotifyState_AttackWindow::NotifyEnd(USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);
    SendEvent(MeshComp, Animation, false);
}

void UAnimNotifyState_AttackWindow::SendEvent(USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation, bool bBegin) const
{
    AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
    if (!Owner || !Owner->HasAuthority() || !Owner->GetWorld()->IsGameWorld())
    {
        return;
    }
    FGameplayEventData Event;
    Event.EventTag = bBegin ? WeaponGameplayTags::Event_Attack_WindowBegin
                           : WeaponGameplayTags::Event_Attack_WindowEnd;
    Event.EventMagnitude = static_cast<float>(Window);
    Event.OptionalObject = Animation;
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, Event.EventTag, Event);
}

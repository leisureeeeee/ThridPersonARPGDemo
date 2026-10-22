#include "Combat/MyCharacterAbilitySystemSComponent.h"
#include "Combat/ARPGCombatAbility.h"
#include "Combat/ARPGCombatAttributeSet.h"
#include "Combat/ARPGCombatDefinition.h"
#include "Combat/ARPGCombatTags.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffect.h"

UMyCharacterAbilitySystemSComponent::UMyCharacterAbilitySystemSComponent()
{
    SetIsReplicatedByDefault(true);
    // PlayerState is owned by PlayerController, as required by Mixed replication.
    SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
    CombatAttributes = CreateDefaultSubobject<UARPGCombatAttributeSet>(TEXT("CombatAttributes"));
}

void UMyCharacterAbilitySystemSComponent::InitializeCombat(AActor* InAvatarActor)
{
    const APlayerState* OwnerState = Cast<APlayerState>(GetOwner());
    const APawn* AvatarPawn = Cast<APawn>(InAvatarActor);
    if (!IsValid(OwnerState) || !IsValid(AvatarPawn) || AvatarPawn->GetPlayerState() != OwnerState)
    {
        return;
    }
    if (GetAvatarActor() && GetAvatarActor() != InAvatarActor)
    {
        CancelAllAbilities();
    }
    InitAbilityActorInfo(GetOwner(), InAvatarActor);
    AddAttributeSetSubobject(CombatAttributes.Get());
    // Once per PlayerState, not once per pawn; possession may precede BeginPlay.
    if (GetOwner()->HasAuthority() && !bStartupGranted)
    {
        GrantStartupConfiguration();
    }
}

void UMyCharacterAbilitySystemSComponent::ReleaseCombatAvatar(const AActor* ExpectedAvatarActor)
{
    if (!ExpectedAvatarActor || GetAvatarActor() != ExpectedAvatarActor)
    {
        return;
    }
    CancelAllAbilities();
    // Preserve the persistent owner while clearing pawn-specific cached actor information.
    if (IsValid(GetOwner()))
    {
        InitAbilityActorInfo(GetOwner(), nullptr);
    }
    else
    {
        ClearActorInfo();
    }
}

void UMyCharacterAbilitySystemSComponent::GrantStartupConfiguration()
{
    if (!CombatDefinition)
    {
        return;
    }
    if (CombatDefinition->InitialAttributesEffect)
    {
        const UGameplayEffect* Effect = CombatDefinition->InitialAttributesEffect->GetDefaultObject<UGameplayEffect>();
        if (!ensureMsgf(Effect->DurationPolicy == EGameplayEffectDurationType::Instant,
            TEXT("Combat initial attributes must use an instant GameplayEffect.")))
        {
            return;
        }
        FGameplayEffectContextHandle Context = MakeEffectContext();
        Context.AddSourceObject(CombatDefinition);
        const FGameplayEffectSpecHandle Spec = MakeOutgoingSpec(CombatDefinition->InitialAttributesEffect, 1.0f, Context);
        if (!Spec.IsValid())
        {
            return;
        }
        ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    }
    for (const TSubclassOf<UARPGCombatAbility>& AbilityClass : CombatDefinition->StartupAbilities)
    {
        if (!AbilityClass || AbilityClass->HasAnyClassFlags(CLASS_Abstract))
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: skipped an empty or abstract startup combat ability."), *GetName());
            continue;
        }
        GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, GetOwner()));
    }
    bStartupGranted = true;
}

bool UMyCharacterAbilitySystemSComponent::RequestCombatAction(FGameplayTag ActionTag)
{
    using namespace ARPGCombatTags;
    const bool bInputAction = ActionTag == Ability_Attack_Normal || ActionTag == Ability_Attack_Special
        || ActionTag == Ability_Parry || ActionTag == Ability_Dodge;
    if (!bInputAction || !IsValid(GetAvatarActor()) || !AbilityActorInfo.IsValid()
        || (!AbilityActorInfo->IsLocallyControlled() && !AbilityActorInfo->IsNetAuthority()))
    {
        return false;
    }
    return TryActivateAbilitiesByTag(FGameplayTagContainer(ActionTag), false);
}

int32 UMyCharacterAbilitySystemSComponent::DispatchCombatEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValid(GetAvatarActor()) || !AbilityActorInfo.IsValid()
        || !EventTag.MatchesTag(ARPGCombatTags::Event_Root))
    {
        return 0;
    }
    FGameplayEventData EventData = Payload;
    EventData.EventTag = EventTag;
    return HandleGameplayEvent(EventTag, &EventData);
}

void UMyCharacterAbilitySystemSComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelAllAbilities();
    Super::EndPlay(EndPlayReason);
    ClearActorInfo();
}

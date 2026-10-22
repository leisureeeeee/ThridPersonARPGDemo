#include "Combat/ARPGAbilitySystemComponent.h"
#include "Combat/ARPGCombatAbility.h"
#include "Combat/ARPGCombatAttributeSet.h"
#include "Combat/ARPGCombatDefinition.h"
#include "Combat/ARPGCombatTags.h"
#include "GameplayEffect.h"

UARPGAbilitySystemComponent::UARPGAbilitySystemComponent()
{
    SetIsReplicatedByDefault(true);
    SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
    CombatAttributes = CreateDefaultSubobject<UARPGCombatAttributeSet>(TEXT("CombatAttributes"));
}

void UARPGAbilitySystemComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeCombat();
}

void UARPGAbilitySystemComponent::InitializeCombat()
{
    if (!IsValid(GetOwner()))
    {
        return;
    }
    InitAbilityActorInfo(GetOwner(), GetOwner());
    // AddSpawnedAttribute uses AddUnique; repeated possession cannot duplicate the set.
    AddAttributeSetSubobject(CombatAttributes.Get());
    if (HasBegunPlay() && GetOwner()->HasAuthority() && !bStartupGranted)
    {
        GrantStartupConfiguration();
    }
}

void UARPGAbilitySystemComponent::GrantStartupConfiguration()
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

bool UARPGAbilitySystemComponent::RequestCombatAction(FGameplayTag ActionTag)
{
    using namespace ARPGCombatTags;
    const bool bInputAction = ActionTag == Ability_Attack_Normal || ActionTag == Ability_Attack_Special
        || ActionTag == Ability_Parry || ActionTag == Ability_Dodge;
    if (!bInputAction || !AbilityActorInfo.IsValid() || (!AbilityActorInfo->IsLocallyControlled()
        && !AbilityActorInfo->IsNetAuthority()))
    {
        return false;
    }
    return TryActivateAbilitiesByTag(FGameplayTagContainer(ActionTag), false);
}

int32 UARPGAbilitySystemComponent::DispatchCombatEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !AbilityActorInfo.IsValid()
        || !EventTag.MatchesTag(ARPGCombatTags::Event_Root))
    {
        return 0;
    }
    FGameplayEventData EventData = Payload;
    EventData.EventTag = EventTag;
    return HandleGameplayEvent(EventTag, &EventData);
}

void UARPGAbilitySystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelAllAbilities();
    Super::EndPlay(EndPlayReason);
    ClearActorInfo();
}

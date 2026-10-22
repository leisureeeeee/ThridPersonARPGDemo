#include "PlayerState/ARPGPlayerState.h"
#include "AbilitySystem/MyCharacterAbilitySystemComponent.h"
#include "AbilitySystem/AttributeSetBase.h"

AARPGPlayerState::AARPGPlayerState()
{
    AbilitySystemComponent = CreateDefaultSubobject<UMyCharacterAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
    AttributeSet = CreateDefaultSubobject<UAttributeSetBase>(TEXT("AttributeSet"));
    SetNetUpdateFrequency(100.f);
}

UAbilitySystemComponent* AARPGPlayerState::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

#include "PlayerState/ARPGPlayerState.h"
#include "Combat/MyCharacterAbilitySystemSComponent.h"

AARPGPlayerState::AARPGPlayerState()
{
    MyCharacterAbilitySystem = CreateDefaultSubobject<UMyCharacterAbilitySystemSComponent>(TEXT("MyCharacterAbilitySystemSComponent"));
}

UAbilitySystemComponent* AARPGPlayerState::GetAbilitySystemComponent() const
{
    return MyCharacterAbilitySystem;
}

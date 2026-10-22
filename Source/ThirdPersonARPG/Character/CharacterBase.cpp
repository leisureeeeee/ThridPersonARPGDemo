#include "Character/CharacterBase.h"
#include "AbilitySystem/AbilitySystemComponentBase.h"

ACharacterBase::ACharacterBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

UAbilitySystemComponent* ACharacterBase::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

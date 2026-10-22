#include "Character/BossCharacter.h"
#include "AbilitySystem/AbilitySystemComponentBase.h"
#include "AbilitySystem/AttributeSetBase.h"

ABossCharacter::ABossCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponentBase>(TEXT("AbilitySystemComponent"));
    AttributeSet = CreateDefaultSubobject<UAttributeSetBase>(TEXT("AttributeSet"));
}

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemComponentBase.generated.h"

// Shared extension point; concrete components define ownership, replication and abilities.
UCLASS(Abstract, BlueprintType)
class THIRDPERSONARPG_API UAbilitySystemComponentBase : public UAbilitySystemComponent
{
    GENERATED_BODY()
};

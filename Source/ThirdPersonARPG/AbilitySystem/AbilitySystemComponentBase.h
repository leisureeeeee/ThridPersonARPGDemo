#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemComponentBase.generated.h"

// Instantiable foundation; derived components may add replication policies and abilities.
UCLASS(BlueprintType)
class THIRDPERSONARPG_API UAbilitySystemComponentBase : public UAbilitySystemComponent
{
    GENERATED_BODY()
};

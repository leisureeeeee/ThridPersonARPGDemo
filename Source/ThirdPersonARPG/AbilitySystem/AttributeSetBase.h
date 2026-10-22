#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AttributeSetBase.generated.h"

// Concrete sets will declare their attributes and attribute-specific rules.
UCLASS(Abstract, BlueprintType)
class THIRDPERSONARPG_API UAttributeSetBase : public UAttributeSet
{
    GENERATED_BODY()
};

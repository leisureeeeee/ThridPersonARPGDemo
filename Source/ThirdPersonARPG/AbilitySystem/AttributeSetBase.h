#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AttributeSetBase.generated.h"

// Instantiable foundation; derived sets may declare attributes and their rules.
UCLASS(BlueprintType)
class THIRDPERSONARPG_API UAttributeSetBase : public UAttributeSet
{
    GENERATED_BODY()
};

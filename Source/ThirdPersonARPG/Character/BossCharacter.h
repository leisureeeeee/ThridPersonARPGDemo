#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "BossCharacter.generated.h"

UCLASS()
class THIRDPERSONARPG_API ABossCharacter : public ACharacterBase
{
    GENERATED_BODY()

public:
    ABossCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

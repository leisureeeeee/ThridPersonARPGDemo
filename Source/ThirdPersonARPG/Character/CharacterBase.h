#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "CharacterBase.generated.h"

class UAbilitySystemComponentBase;
class UAttributeSetBase;

UCLASS(Abstract)
class THIRDPERSONARPG_API ACharacterBase : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    ACharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    UFUNCTION(BlueprintPure, Category = "Abilities")
    const UAttributeSetBase* GetAttributeSet() const { return AttributeSet; }

protected:
    // References only: derived characters can obtain these from a persistent owner
    // such as PlayerState, or assign their own concrete implementations later.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Abilities")
    TObjectPtr<UAbilitySystemComponentBase> AbilitySystemComponent = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Abilities")
    TObjectPtr<UAttributeSetBase> AttributeSet = nullptr;
};

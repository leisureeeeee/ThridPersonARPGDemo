#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "ARPGPlayerState.generated.h"

class UMyCharacterAbilitySystemComponent;
class UAttributeSetBase;

UCLASS()
class THIRDPERSONARPG_API AARPGPlayerState : public APlayerState, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AARPGPlayerState();
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    UMyCharacterAbilitySystemComponent* GetPlayerASC() const { return AbilitySystemComponent; }
    UAttributeSetBase* GetPlayerAS() const { return AttributeSet; }

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UMyCharacterAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAttributeSetBase> AttributeSet;
};

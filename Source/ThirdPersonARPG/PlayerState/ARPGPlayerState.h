#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "ARPGPlayerState.generated.h"

class UMyCharacterAbilitySystemSComponent;

// Player abilities and attributes outlive an individual possessed character.
UCLASS()
class THIRDPERSONARPG_API AARPGPlayerState : public APlayerState, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AARPGPlayerState();

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    UFUNCTION(BlueprintPure, Category = "Player|Abilities")
    UMyCharacterAbilitySystemSComponent* GetMyCharacterAbilitySystem() const { return MyCharacterAbilitySystem; }

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Abilities", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UMyCharacterAbilitySystemSComponent> MyCharacterAbilitySystem;
};

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ARPGAbilitySystemComponent.generated.h"

class UARPGCombatAttributeSet;
class UARPGCombatDefinition;

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class THIRDPERSONARPG_API UARPGAbilitySystemComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()

public:
    UARPGAbilitySystemComponent();

    // Character-owned ASC: owner and avatar are the same actor, for players and enemies.
    void InitializeCombat();

    UFUNCTION(BlueprintPure, Category = "Combat")
    const UARPGCombatAttributeSet* GetCombatAttributes() const { return CombatAttributes; }

    UFUNCTION(BlueprintPure, Category = "Combat")
    const UARPGCombatDefinition* GetCombatDefinition() const { return CombatDefinition; }

    // True means GAS accepted activation, not that an animation/hit/skill has completed.
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool RequestCombatAction(FGameplayTag ActionTag);

    // Local server event dispatch only; this is deliberately not an RPC or a hit resolver.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Combat")
    int32 DispatchCombatEvent(FGameplayTag EventTag, const FGameplayEventData& Payload);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void GrantStartupConfiguration();

    UPROPERTY(VisibleAnywhere, Category = "Combat")
    TObjectPtr<UARPGCombatAttributeSet> CombatAttributes;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TObjectPtr<UARPGCombatDefinition> CombatDefinition;

    bool bStartupGranted = false;
};

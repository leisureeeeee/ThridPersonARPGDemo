#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "MyCharacterAbilitySystemSComponent.generated.h"

class UARPGCombatAttributeSet;
class UARPGCombatDefinition;

// Player-only ASC. AARPGPlayerState owns it; the possessed character is its avatar.
UCLASS(ClassGroup = (Player))
class THIRDPERSONARPG_API UMyCharacterAbilitySystemSComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()

public:
    UMyCharacterAbilitySystemSComponent();

    // Called after possession on the server and PlayerState replication on clients.
    void InitializeCombat(AActor* InAvatarActor);

    // An old character cannot detach the avatar of its replacement.
    void ReleaseCombatAvatar(const AActor* ExpectedAvatarActor);

    UFUNCTION(BlueprintPure, Category = "Combat")
    const UARPGCombatAttributeSet* GetCombatAttributes() const { return CombatAttributes; }

    UFUNCTION(BlueprintPure, Category = "Combat")
    const UARPGCombatDefinition* GetCombatDefinition() const { return CombatDefinition; }

    // Acceptance by GAS does not mean the action completed or hit a target.
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool RequestCombatAction(FGameplayTag ActionTag);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Combat")
    int32 DispatchCombatEvent(FGameplayTag EventTag, const FGameplayEventData& Payload);

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void GrantStartupConfiguration();

    // Nested under this ASC, these attributes share the PlayerState lifetime.
    UPROPERTY(VisibleAnywhere, Category = "Combat")
    TObjectPtr<UARPGCombatAttributeSet> CombatAttributes;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TObjectPtr<UARPGCombatDefinition> CombatDefinition;

    bool bStartupGranted = false;
};

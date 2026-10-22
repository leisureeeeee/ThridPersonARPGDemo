#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "LightAttackAbility.generated.h"

class UAnimMontage;

// Asset references live in DefaultCombat.ini, not hard-coded resource paths.
UCLASS(Config = Combat, DefaultConfig)
class THIRDPERSONARPG_API ULightAttackAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    ULightAttackAbility();

protected:
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

private:
    UFUNCTION()
    void OnAttackCompleted();

    UFUNCTION()
    void OnAttackCancelled();

    UPROPERTY(Config, EditDefaultsOnly, Category = "Attack")
    TSoftObjectPtr<UAnimMontage> AttackMontage;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Attack", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float PlayRate = 1.f;
};

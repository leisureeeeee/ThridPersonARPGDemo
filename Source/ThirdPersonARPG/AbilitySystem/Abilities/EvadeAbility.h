#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "EvadeAbility.generated.h"

class UAnimMontage;

UCLASS(Config = Combat, DefaultConfig)
class THIRDPERSONARPG_API UEvadeAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UEvadeAbility();

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
    void OnEvadeCompleted();

    UFUNCTION()
    void OnEvadeCancelled();

    // Clockwise from forward: F, FR, R, BR, B, BL, L, FL.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Evade")
    TArray<TSoftObjectPtr<UAnimMontage>> EvadeMontages;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Evade", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float PlayRate = 1.f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Evade", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float RootMotionTranslationScale = 1.f;
};

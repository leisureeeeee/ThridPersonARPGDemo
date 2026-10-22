#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "LightAttackAbility.generated.h"

class UAnimMontage;
class UAttackTraceComponent;

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

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility, bool bWasCancelled) override;

private:
    UFUNCTION()
    void OnWindowBegin(FGameplayEventData Event);

    UFUNCTION()
    void OnWindowEnd(FGameplayEventData Event);

    void UpdateWindow(const FGameplayEventData& Event, bool bOpen);
    void OnMoveCancel();
    uint8 OpenWindows = 0;
    bool bEnding = false;
    TWeakObjectPtr<UAttackTraceComponent> TraceComponent;

    UFUNCTION()
    void OnAttackCompleted();

    UFUNCTION()
    void OnAttackCancelled();

    UPROPERTY(Config, EditDefaultsOnly, Category = "Attack")
    TSoftObjectPtr<UAnimMontage> AttackMontage;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Attack", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float PlayRate = 1.f;
};

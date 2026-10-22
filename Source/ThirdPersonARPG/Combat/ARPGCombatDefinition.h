#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ARPGCombatDefinition.generated.h"

class UARPGCombatAbility;
class UGameplayEffect;
class UARPGAbilitySystemComponent;

// Contract only: timing and balance are intentionally unset and not consumed yet.
USTRUCT(BlueprintType)
struct THIRDPERSONARPG_API FARPGCombatTuning
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", Units = "s"))
    float PerfectParryWindow = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", Units = "s"))
    float ParryRecovery = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", Units = "s"))
    float PerfectParryRecovery = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", Units = "s"))
    float PostureDecayDelay = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
    float PostureDecayPerSecond = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
    float PerfectParryPostureDamage = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
    float ExecutionEnergyReward = 0.0f;
};

// Separate player/enemy assets can share the same C++ framework.
UCLASS(BlueprintType)
class THIRDPERSONARPG_API UARPGCombatDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    const FARPGCombatTuning& GetTuning() const { return Tuning; }
    TSubclassOf<UGameplayEffect> GetHitEffect() const { return HitEffect; }
    TSubclassOf<UGameplayEffect> GetPerfectParryEffect() const { return PerfectParryEffect; }
    TSubclassOf<UGameplayEffect> GetPostureDecayEffect() const { return PostureDecayEffect; }
    TSubclassOf<UGameplayEffect> GetExecutionPostureResetEffect() const { return ExecutionPostureResetEffect; }
    TSubclassOf<UGameplayEffect> GetExecutionEnergyRewardEffect() const { return ExecutionEnergyRewardEffect; }
    TSubclassOf<UGameplayEffect> GetSpecialAttackCostEffect() const { return SpecialAttackCostEffect; }

private:
    friend class UARPGAbilitySystemComponent;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Startup")
    TArray<TSubclassOf<UARPGCombatAbility>> StartupAbilities;

    // Must be instant. Configure maximum attributes before current values.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Startup")
    TSubclassOf<UGameplayEffect> InitialAttributesEffect;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Rules")
    FARPGCombatTuning Tuning;

    // Extension slots only; none of these effects is automatically applied by this scaffold.
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Effects")
    TSubclassOf<UGameplayEffect> HitEffect;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Effects")
    TSubclassOf<UGameplayEffect> PerfectParryEffect;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Effects")
    TSubclassOf<UGameplayEffect> PostureDecayEffect;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Effects")
    TSubclassOf<UGameplayEffect> ExecutionPostureResetEffect;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Effects")
    TSubclassOf<UGameplayEffect> ExecutionEnergyRewardEffect;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Effects")
    TSubclassOf<UGameplayEffect> SpecialAttackCostEffect;
};

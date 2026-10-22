#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ARPGCombatAttributeSet.generated.h"

#define ARPG_ATTRIBUTE_ACCESSORS(PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UARPGCombatAttributeSet, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

// Posture measures pressure: zero is recovered, maximum is broken.
// Enemies may leave Energy unused; no player-only rules live in this shared set.
UCLASS()
class THIRDPERSONARPG_API UARPGCombatAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UARPGCombatAttributeSet();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
    ARPG_ATTRIBUTE_ACCESSORS(Health)
    ARPG_ATTRIBUTE_ACCESSORS(MaxHealth)
    ARPG_ATTRIBUTE_ACCESSORS(Posture)
    ARPG_ATTRIBUTE_ACCESSORS(MaxPosture)
    ARPG_ATTRIBUTE_ACCESSORS(Energy)
    ARPG_ATTRIBUTE_ACCESSORS(MaxEnergy)

private:
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Combat|Attributes", meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData Health;

    UFUNCTION()
    void OnRep_Health(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Combat|Attributes", meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData MaxHealth;

    UFUNCTION()
    void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Posture, Category = "Combat|Attributes", meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData Posture;

    UFUNCTION()
    void OnRep_Posture(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxPosture, Category = "Combat|Attributes", meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData MaxPosture;

    UFUNCTION()
    void OnRep_MaxPosture(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Energy, Category = "Combat|Attributes", meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData Energy;

    UFUNCTION()
    void OnRep_Energy(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxEnergy, Category = "Combat|Attributes", meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData MaxEnergy;

    UFUNCTION()
    void OnRep_MaxEnergy(const FGameplayAttributeData& OldValue);
};

#undef ARPG_ATTRIBUTE_ACCESSORS

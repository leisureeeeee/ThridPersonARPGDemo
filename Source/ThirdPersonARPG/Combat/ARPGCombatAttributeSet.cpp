#include "Combat/ARPGCombatAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UARPGCombatAttributeSet::UARPGCombatAttributeSet()
{
    // Safe placeholders. A configured instant GameplayEffect supplies actual starting values.
    InitMaxHealth(100.0f);
    InitHealth(100.0f);
    InitMaxPosture(100.0f);
    InitPosture(0.0f);
    InitMaxEnergy(100.0f);
    InitEnergy(0.0f);
}

void UARPGCombatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION_NOTIFY(UARPGCombatAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UARPGCombatAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UARPGCombatAttributeSet, Posture, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UARPGCombatAttributeSet, MaxPosture, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UARPGCombatAttributeSet, Energy, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UARPGCombatAttributeSet, MaxEnergy, COND_None, REPNOTIFY_Always);
}

void UARPGCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    if (Attribute == GetMaxHealthAttribute() || Attribute == GetMaxPostureAttribute() || Attribute == GetMaxEnergyAttribute())
    {
        NewValue = FMath::Max(1.0f, NewValue);
    }
    else if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }
    else if (Attribute == GetPostureAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxPosture());
    }
    else if (Attribute == GetEnergyAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxEnergy());
    }
}

void UARPGCombatAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);
    // Also preserve bounds when a duration effect changes a maximum.
    if (Attribute == GetMaxHealthAttribute() && GetHealth() > NewValue)
    {
        SetHealth(NewValue);
    }
    else if (Attribute == GetMaxPostureAttribute() && GetPosture() > NewValue)
    {
        SetPosture(NewValue);
    }
    else if (Attribute == GetMaxEnergyAttribute() && GetEnergy() > NewValue)
    {
        SetEnergy(NewValue);
    }
}

void UARPGCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);
    // Numeric safety only. No death, posture break, execution or regeneration is resolved here.
    SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
    SetPosture(FMath::Clamp(GetPosture(), 0.0f, GetMaxPosture()));
    SetEnergy(FMath::Clamp(GetEnergy(), 0.0f, GetMaxEnergy()));
}

void UARPGCombatAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UARPGCombatAttributeSet, Health, OldValue);
}


void UARPGCombatAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UARPGCombatAttributeSet, MaxHealth, OldValue);
}


void UARPGCombatAttributeSet::OnRep_Posture(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UARPGCombatAttributeSet, Posture, OldValue);
}


void UARPGCombatAttributeSet::OnRep_MaxPosture(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UARPGCombatAttributeSet, MaxPosture, OldValue);
}


void UARPGCombatAttributeSet::OnRep_Energy(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UARPGCombatAttributeSet, Energy, OldValue);
}


void UARPGCombatAttributeSet::OnRep_MaxEnergy(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UARPGCombatAttributeSet, MaxEnergy, OldValue);
}

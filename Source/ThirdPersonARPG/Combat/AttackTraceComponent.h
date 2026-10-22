#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/AttackWindow.h"
#include "AttackTraceComponent.generated.h"

class UAnimMontage;

struct FAttackTraceSample
{
    FVector Base = FVector::ZeroVector;
    FVector Tip = FVector::ZeroVector;
    float Time = 0.f;
    uint8 Windows = 0;
};

DECLARE_MULTICAST_DELEGATE(FOnAttackMoveCancel);

// Local diagnostic data; the ability owns its lifetime and authoritative window tags.
UCLASS(ClassGroup = Combat)
class THIRDPERSONARPG_API UAttackTraceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAttackTraceComponent();
    void BeginAttack(UAnimMontage* Montage);
    void EndAttack(bool bCompleted);
    void SetWindow(EAttackWindow Window, bool bOpen);
    UFUNCTION(BlueprintPure, Category = "Combat|Debug")
    bool IsWindowOpen(EAttackWindow Window) const;
    bool BufferEvade(const FVector& WorldDirection);
    FOnAttackMoveCancel OnMoveCancel;

    UFUNCTION(BlueprintPure, Category = "Combat|Debug")
    bool IsActiveAttack() const { return bAttackActive; }
    bool HasBladeSample() const { return bBladeAvailable; }
    bool IsDisplayVisible() const;
    UFUNCTION(BlueprintPure, Category = "Combat|Debug")
    float GetAttackTime() const { return AttackTime; }
    float GetAttackLength() const { return AttackLength; }
    float GetFrameRate() const { return FrameRate; }
    UFUNCTION(BlueprintPure, Category = "Combat|Debug")
    int32 GetContactCount() const { return Contacts.Num(); }
    UFUNCTION(BlueprintPure, Category = "Combat|Debug")
    int32 GetSampleCount() const { return Samples.Num(); }
    float GetWindowStart(EAttackWindow Window) const { return WindowStarts[static_cast<int32>(Window)]; }
    float GetWindowEnd(EAttackWindow Window) const { return WindowEnds[static_cast<int32>(Window)]; }
    const TArray<FAttackTraceSample>& GetSamples() const { return Samples; }
    const TArray<FVector>& GetContactPoints() const { return ContactPoints; }

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    void TraceBlade(const FAttackTraceSample& Previous, const FAttackTraceSample& Current);

    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> ActiveMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Trace", meta = (ClampMin = "0.1"))
    float TraceRadius = 2.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Input", meta = (ClampMin = "0.0", ClampMax = "0.25"))
    float EvadeBufferSeconds = .1f;

    TArray<FAttackTraceSample> Samples;
    TSet<TWeakObjectPtr<AActor>> Contacts;
    TArray<FVector> ContactPoints;
    FVector BufferedEvadeDirection = FVector::ZeroVector;
    double BufferedEvadeExpiry = -1.;
    double DisplayUntil = -1.;
    float AttackTime = 0.f;
    float AttackLength = 0.f;
    float FrameRate = 60.f;
    float WindowStarts[3] = {-1.f, -1.f, -1.f};
    float WindowEnds[3] = {-1.f, -1.f, -1.f};
    uint8 Windows = 0;
    bool bAttackActive = false;
    bool bBladeAvailable = false;
};

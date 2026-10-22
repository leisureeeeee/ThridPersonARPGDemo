#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ARPGCharacterMovementComponent.generated.h"

class UARPGDirectionalStartSet;
class UAnimSequence;
struct FARPGDirectionalStartProfile;

/** Grounded turn-to-move policy. Input bindings stay in the PlayerController. */
UCLASS()
class THIRDPERSONARPG_API UARPGCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	void SetSprintRequested(bool bRequested);
	bool IsSprinting() const;
	float GetSprintTurnTime() const { return FMath::Min(PivotElapsed * 2.0f, 1.1f); }
	virtual float GetMaxSpeed() const override;
	bool IsPivoting() const { return bPivotBraking || bPivotTurning; }
	bool HasDirectionalStart() const;
	bool IsDirectionalStartComplete() const;
	float GetDirectionalStartTime() const { return DirectionalStartTime; }
	float GetDirectionalMeshYaw() const;
	UAnimSequence* GetDirectionalStartAnimation() const;

protected:
	virtual void OnRegister() override;
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
	virtual void PhysicsRotation(float DeltaTime) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

private:
	void ResetPivot();
	bool BeginDirectionalStart(const FVector& DesiredDirection);
	void UpdateDirectionalStart(float DeltaTime, const FVector& DesiredDirection);
	const FARPGDirectionalStartProfile* GetDirectionalProfile() const;

	UPROPERTY(EditDefaultsOnly, Category = "Ground Locomotion|Pivot", meta = (ClampMin = "100", ClampMax = "175", Units = "deg"))
	float PivotEnterAngle = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ground Locomotion|Pivot", meta = (ClampMin = "0", Units = "cm/s"))
	float PivotMinimumSpeed = 75.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ground Locomotion|Pivot", meta = (ClampMin = "1", Units = "cm/s"))
	float PivotTurnSpeedThreshold = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ground Locomotion|Pivot", meta = (ClampMin = "5", ClampMax = "45", Units = "deg"))
	float PivotExitAngle = 15.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ground Locomotion|Pivot", meta = (ClampMin = "180", Units = "deg/s"))
	float PivotYawRate = 720.0f;

	bool bPivotBraking = false;
	bool bPivotTurning = false;
	float PivotElapsed = 0.0f;
	float PivotCooldown = 0.0f;

	// Append new instance storage to preserve offsets of existing movement settings.
	UPROPERTY(EditDefaultsOnly, Category = "Ground Locomotion|Directional Start")
	TObjectPtr<UARPGDirectionalStartSet> DirectionalStartSet;

	int32 DirectionalProfileIndex = INDEX_NONE;
	bool bDirectionalCancelled = false;
	bool bHadInput = false;
	float DirectionalStartTime = 0.0f;
	float DirectionalDistance = 0.0f;
	float DirectionalInitialYaw = 0.0f;
	float DirectionalDeltaYaw = 0.0f;
	float DirectionalTurnProgress = 0.0f;
	FVector DirectionalTarget = FVector::ZeroVector;
	FVector DirectionalPreviousLocation = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Ground Locomotion|Sprint", meta = (ClampMin = "1", Units = "cm/s"))
	float SprintSpeed = 600.0f;

	bool bSprintRequested = false;

};

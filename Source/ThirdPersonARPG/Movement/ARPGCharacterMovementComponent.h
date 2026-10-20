#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ARPGCharacterMovementComponent.generated.h"

/** Grounded turn-to-move policy. Input bindings stay in the PlayerController. */
UCLASS()
class THIRDPERSONARPG_API UARPGCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	bool IsPivoting() const { return bPivotBraking || bPivotTurning; }

protected:
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
	virtual void PhysicsRotation(float DeltaTime) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

private:
	void ResetPivot();

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
};

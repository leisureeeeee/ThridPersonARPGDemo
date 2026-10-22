#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MyCharacterAnimInstance.generated.h"

class UCharacterMovementComponent;
class UARPGCharacterMovementComponent;
class UAnimSequence;

/** Reads movement on the game thread; the animation graph only consumes pose data. */
UCLASS()
class THIRDPERSONARPG_API UMyCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	void ResetStopMatching();
	float FindStopTime(float RemainingDistance) const;
	static float PredictStopDistance(const UCharacterMovementComponent& Movement);
	void UpdateStartMatching(float DeltaSeconds, float DistanceMoved, const UARPGCharacterMovementComponent* GroundMovement);
	float FindStartTime(float Distance) const;
	void UpdateMovementBlocked(float DeltaSeconds, float DistanceMoved, const UCharacterMovementComponent* Movement,
		const UARPGCharacterMovementComponent* GroundMovement);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	float GroundSpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bShouldMove = false;

	// Actual travel from the previous movement update drives the start pose.
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Start Matching")
	TArray<FVector2D> StartDistanceSamples;

	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Start Matching", meta = (ClampMin = "0.1", ClampMax = "4"))
	float MaxStartPlayRate = 2.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	float StartAnimationTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bStartAnimationComplete = false;

	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Walk", meta = (ClampMin = "1", Units = "cm/s"))
	float WalkAuthoredSpeed = 167.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	float WalkPlayRate = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bPivoting = false;

	// Baked from the selected stop animation: X = seconds, Y = remaining cm.
	// Time must increase and remaining distance must decrease (plateaus allowed).
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Stop Matching")
	TArray<FVector2D> StopDistanceSamples;

	// Finish the stationary recovery tail promptly after the capsule stops.
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Stop Matching", meta = (ClampMin = "0.05", ClampMax = "0.3", Units = "s"))
	float StopSettleDuration = 0.12f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	float StopAnimationTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bStopAnimationComplete = true;

	bool bHadMovementIntent = false;
	bool bStopping = false;
	bool bStopProfileValid = false;
	float StopSettleElapsed = 0.0f;
	bool bStartProfileValid = false;
	bool bHasPreviousLocation = false;
	FVector PreviousLocation = FVector::ZeroVector;
	float StartDistanceTraveled = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Start Matching")
	TObjectPtr<UAnimSequence> ForwardStartAnimation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimSequence> ActiveStartAnimation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	float StartRootYawOffset = 0.0f;

	// Append runtime storage to preserve the existing reflected field layout.
	float BlockedMovementElapsed = 0.0f;
	bool bMovementBlocked = false;

	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Idle")
	TObjectPtr<UAnimSequence> SheathedIdleAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Idle")
	TObjectPtr<UAnimSequence> DrawnIdleAnimation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimSequence> ActiveIdleAnimation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion|Sprint", meta = (AllowPrivateAccess = "true"))
	bool bShouldWalk = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion|Sprint", meta = (AllowPrivateAccess = "true"))
	bool bShouldSprint = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion|Sprint", meta = (AllowPrivateAccess = "true"))
	bool bSprintTurning = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion|Sprint", meta = (AllowPrivateAccess = "true"))
	bool bSprintStopping = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion|Sprint", meta = (AllowPrivateAccess = "true"))
	bool bSprintStartComplete = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion|Sprint", meta = (AllowPrivateAccess = "true"))
	float SprintStartTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion|Sprint", meta = (AllowPrivateAccess = "true"))
	float SprintTurnTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion|Sprint", meta = (AllowPrivateAccess = "true"))
	float SprintPlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Sprint", meta = (ClampMin = "0.05", Units = "s"))
	float SprintStartDuration = 0.85f;

	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Sprint", meta = (ClampMin = "1", Units = "cm/s"))
	float SprintAuthoredSpeed = 600.0f;

	bool bHadSprintIntent = false;

};

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MyCharacterAnimInstance.generated.h"

class UCharacterMovementComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	float GroundSpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bShouldMove = false;

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
};

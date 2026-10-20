// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MyCharacterAnimInstance.generated.h"

/** Supplies velocity and movement intent for interruptible idle/start/walk/stop animation. */
UCLASS()
class THIRDPERSONARPG_API UMyCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	// Actual horizontal speed in cm/s, independent of the current input.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	float GroundSpeed = 0.0f;

	// Horizontal movement intent, not residual velocity while braking.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bShouldMove = false;
};

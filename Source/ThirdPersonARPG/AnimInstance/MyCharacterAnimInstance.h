// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MyCharacterAnimInstance.generated.h"

/** Supplies movement data for the character's idle and walk animation states. */
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bShouldMove = false;
};

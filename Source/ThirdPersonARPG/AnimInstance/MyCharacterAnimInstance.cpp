// Fill out your copyright notice in the Description page of Project Settings.

#include "MyCharacterAnimInstance.h"
#include "GameFramework/Pawn.h"

void UMyCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// Animation editor previews can have no owning pawn. Clear stale data too.
	const APawn* OwningPawn = TryGetPawnOwner();
	GroundSpeed = IsValid(OwningPawn) ? OwningPawn->GetVelocity().Size2D() : 0.0f;

	// Ignore tiny residual velocity near a complete stop.
	constexpr float MovementSpeedThreshold = 3.0f;
	bShouldMove = GroundSpeed > MovementSpeedThreshold;
}

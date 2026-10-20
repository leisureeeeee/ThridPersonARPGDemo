// Fill out your copyright notice in the Description page of Project Settings.

#include "MyCharacterAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

void UMyCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const APawn* OwningPawn = TryGetPawnOwner();
	GroundSpeed = IsValid(OwningPawn) ? OwningPawn->GetVelocity().Size2D() : 0.0f;

	// Input intent is separate from velocity: releasing input starts the stop
	// animation while CharacterMovement is still braking the character.
	const ACharacter* Character = Cast<ACharacter>(OwningPawn);
	const UCharacterMovementComponent* Movement = IsValid(Character) ? Character->GetCharacterMovement() : nullptr;
	bShouldMove = IsValid(Movement)
		&& Movement->GetCurrentAcceleration().SizeSquared2D() > UE_KINDA_SMALL_NUMBER;
}

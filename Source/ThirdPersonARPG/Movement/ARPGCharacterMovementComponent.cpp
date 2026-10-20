#include "ARPGCharacterMovementComponent.h"
#include "GameFramework/Character.h"

void UARPGCharacterMovementComponent::ResetPivot()
{
	bPivotBraking = false;
	bPivotTurning = false;
	PivotElapsed = 0.0f;
}

void UARPGCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	ResetPivot();
	PivotCooldown = 0.0f;
}

void UARPGCharacterMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	PivotCooldown = FMath::Max(0.0f, PivotCooldown - DeltaTime);
	const FVector RequestedAcceleration = Acceleration;
	if (!IsMovingOnGround() || !HasValidData() || HasAnimRootMotion()
		|| CurrentRootMotion.HasOverrideVelocity() || RequestedAcceleration.IsNearlyZero())
	{
		ResetPivot();
		Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
		return;
	}

	const FVector DesiredDirection = RequestedAcceleration.GetSafeNormal2D();
	const float Speed = Velocity.Size2D();
	const float VelocityDot = FVector::DotProduct(Velocity.GetSafeNormal2D(), DesiredDirection);
	if (!IsPivoting() && PivotCooldown <= 0.0f && Speed >= PivotMinimumSpeed
		&& VelocityDot < FMath::Cos(FMath::DegreesToRadians(PivotEnterAngle)))
	{
		bPivotBraking = true;
		PivotElapsed = 0.0f;
	}

	if (IsPivoting())
	{
		PivotElapsed += FMath::Max(0.0f, DeltaTime);
		// W->S->W should cancel the brake rather than finish an obsolete turn.
		if ((bPivotBraking && Speed > PivotTurnSpeedThreshold && VelocityDot > 0.5f)
			|| PivotElapsed > 0.75f)
		{
			ResetPivot();
			PivotCooldown = 0.15f;
		}
		else
		{
			if (Speed <= PivotTurnSpeedThreshold)
			{
				bPivotBraking = false;
				bPivotTurning = true;
			}
			if (bPivotTurning && FVector::DotProduct(CharacterOwner->GetActorForwardVector(), DesiredDirection)
				>= FMath::Cos(FMath::DegreesToRadians(PivotExitAngle)))
			{
				ResetPivot();
				PivotCooldown = 0.15f;
			}
		}
	}

	if (IsPivoting())
	{
		// Use the engine's actual braking, preserving input intent for animation/rotation.
		Acceleration = FVector::ZeroVector;
		Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
		Acceleration = RequestedAcceleration;
	}
	else
	{
		Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
	}
}

void UARPGCharacterMovementComponent::PhysicsRotation(float DeltaTime)
{
	if (!IsMovingOnGround() || !bOrientRotationToMovement || !HasValidData()
		|| HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity())
	{
		Super::PhysicsRotation(DeltaTime);
		return;
	}
	const FVector RequestedAcceleration = Acceleration;
	const FRotator SavedRotationRate = RotationRate;
	if (bPivotTurning)
	{
		RotationRate.Yaw = PivotYawRate;
	}
	else if (Velocity.SizeSquared2D() > FMath::Square(PivotTurnSpeedThreshold))
	{
		// Ordinary corners follow actual travel; a reversing brake keeps facing travel.
		Acceleration = Velocity.GetSafeNormal2D();
	}
	Super::PhysicsRotation(DeltaTime);
	Acceleration = RequestedAcceleration;
	RotationRate = SavedRotationRate;
}

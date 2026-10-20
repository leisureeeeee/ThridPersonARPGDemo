#include "MyCharacterAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UMyCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	GroundSpeed = 0.0f;
	bShouldMove = false;
	bHadMovementIntent = false;
	ResetStopMatching();
	bStopProfileValid = StopDistanceSamples.Num() >= 2;
	for (int32 Index = 0; Index < StopDistanceSamples.Num(); ++Index)
	{
		const FVector2D& Sample = StopDistanceSamples[Index];
		bStopProfileValid &= FMath::IsFinite(Sample.X) && FMath::IsFinite(Sample.Y)
			&& Sample.X >= 0.0 && Sample.Y >= 0.0;
		if (Index > 0)
		{
			const FVector2D& Previous = StopDistanceSamples[Index - 1];
			bStopProfileValid &= Sample.X > Previous.X && Sample.Y <= Previous.Y;
		}
	}
	if (bStopProfileValid)
	{
		bStopProfileValid = StopDistanceSamples[0].Y > UE_KINDA_SMALL_NUMBER
			&& StopDistanceSamples.Last().Y <= UE_KINDA_SMALL_NUMBER;
	}
}

void UMyCharacterAnimInstance::ResetStopMatching()
{
	bStopping = false;
	StopAnimationTime = 0.0f;
	StopSettleElapsed = 0.0f;
	bStopAnimationComplete = true;
}

float UMyCharacterAnimInstance::PredictStopDistance(const UCharacterMovementComponent& Movement)
{
	// Continuous approximation of walking braking: dv/dt = -(Friction * v + Deceleration).
	// Prediction does not move the pawn; collisions/slopes remain CharacterMovement's job.
	const double Speed = Movement.Velocity.Size2D();
	const double Friction = FMath::Max(0.0f, Movement.bUseSeparateBrakingFriction
		? Movement.BrakingFriction : Movement.GroundFriction)
		* FMath::Max(0.0f, Movement.BrakingFrictionFactor);
	const double Deceleration = FMath::Max(0.0f, Movement.BrakingDecelerationWalking);
	if (Friction > UE_KINDA_SMALL_NUMBER)
	{
		if (Deceleration <= UE_KINDA_SMALL_NUMBER)
		{
			return static_cast<float>(Speed / Friction);
		}
		return static_cast<float>(FMath::Max(0.0,
			Speed / Friction - Deceleration / (Friction * Friction)
			* FMath::Loge(1.0 + Friction * Speed / Deceleration)));
	}
	return Deceleration > UE_KINDA_SMALL_NUMBER
		? static_cast<float>(Speed * Speed / (2.0 * Deceleration)) : 0.0f;
}

float UMyCharacterAnimInstance::FindStopTime(float RemainingDistance) const
{
	if (RemainingDistance >= StopDistanceSamples[0].Y)
	{
		return static_cast<float>(StopDistanceSamples[0].X);
	}
	for (int32 Index = 1; Index < StopDistanceSamples.Num(); ++Index)
	{
		const FVector2D& Previous = StopDistanceSamples[Index - 1];
		const FVector2D& Sample = StopDistanceSamples[Index];
		if (RemainingDistance >= Sample.Y)
		{
			const double DistanceDelta = Previous.Y - Sample.Y;
			const double Alpha = DistanceDelta > UE_SMALL_NUMBER
				? FMath::Clamp((Previous.Y - RemainingDistance) / DistanceDelta, 0.0, 1.0) : 1.0;
			return static_cast<float>(FMath::Lerp(Previous.X, Sample.X, Alpha));
		}
	}
	return static_cast<float>(StopDistanceSamples.Last().X);
}

void UMyCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	const ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner());
	const UCharacterMovementComponent* Movement = IsValid(Character)
		? Character->GetCharacterMovement() : nullptr;
	GroundSpeed = IsValid(Character) ? Character->GetVelocity().Size2D() : 0.0f;
	bShouldMove = IsValid(Movement)
		&& Movement->GetCurrentAcceleration().SizeSquared2D() > UE_KINDA_SMALL_NUMBER;
	if (!IsValid(Movement) || !Movement->IsMovingOnGround() || !bStopProfileValid)
	{
		ResetStopMatching();
		bHadMovementIntent = bShouldMove;
		return;
	}
	if (bShouldMove)
	{
		ResetStopMatching();
	}
	else
	{
		if (bHadMovementIntent)
		{
			bStopping = true;
			bStopAnimationComplete = false;
			StopSettleElapsed = 0.0f;
			StopAnimationTime = FindStopTime(PredictStopDistance(*Movement));
		}
		if (bStopping)
		{
			// Never rewind when a changing floor/contact perturbs the prediction.
			StopAnimationTime = FMath::Max(StopAnimationTime,
				FindStopTime(PredictStopDistance(*Movement)));
			if (GroundSpeed <= 3.0f)
			{
				StopSettleElapsed += FMath::Max(0.0f, DeltaSeconds);
				const float Alpha = FMath::Clamp(StopSettleElapsed
					/ FMath::Clamp(StopSettleDuration, 0.05f, 0.3f), 0.0f, 1.0f);
				const float TailTime = FMath::Lerp(FindStopTime(0.0f),
					static_cast<float>(StopDistanceSamples.Last().X), Alpha);
				StopAnimationTime = FMath::Max(StopAnimationTime, TailTime);
				bStopAnimationComplete = Alpha >= 1.0f;
				bStopping = !bStopAnimationComplete;
			}
		}
	}
	bHadMovementIntent = bShouldMove;
}

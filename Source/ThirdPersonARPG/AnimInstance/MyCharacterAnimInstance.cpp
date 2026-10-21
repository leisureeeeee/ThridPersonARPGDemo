#include "MyCharacterAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Movement/ARPGCharacterMovementComponent.h"

void UMyCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	GroundSpeed = 0.0f;
	bShouldMove = false;
	bHadMovementIntent = false;
	bPivoting = false;
	WalkPlayRate = 1.0f;
	StartAnimationTime = 0.0f;
	StartDistanceTraveled = 0.0f;
	bStartAnimationComplete = false;
	bHasPreviousLocation = false;
	ResetStopMatching();
	bStartProfileValid = StartDistanceSamples.Num() >= 2;
	for (int32 Index = 0; Index < StartDistanceSamples.Num(); ++Index)
	{
		const FVector2D& Sample = StartDistanceSamples[Index];
		bStartProfileValid &= FMath::IsFinite(Sample.X) && FMath::IsFinite(Sample.Y)
			&& Sample.X >= 0.0 && Sample.Y >= 0.0;
		if (Index > 0)
		{
			const FVector2D& Previous = StartDistanceSamples[Index - 1];
			bStartProfileValid &= Sample.X > Previous.X && Sample.Y >= Previous.Y;
		}
	}
	if (bStartProfileValid)
	{
		bStartProfileValid = StartDistanceSamples[0].Y <= UE_KINDA_SMALL_NUMBER
			&& StartDistanceSamples.Last().Y > UE_KINDA_SMALL_NUMBER;
	}
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

float UMyCharacterAnimInstance::FindStartTime(float Distance) const
{
	for (int32 Index = 1; Index < StartDistanceSamples.Num(); ++Index)
	{
		const FVector2D& Previous = StartDistanceSamples[Index - 1];
		const FVector2D& Sample = StartDistanceSamples[Index];
		if (Distance <= Sample.Y)
		{
			const double DistanceDelta = Sample.Y - Previous.Y;
			const double Alpha = DistanceDelta > UE_SMALL_NUMBER
				? FMath::Clamp((Distance - Previous.Y) / DistanceDelta, 0.0, 1.0) : 0.0;
			return static_cast<float>(FMath::Lerp(Previous.X, Sample.X, Alpha));
		}
	}
	return static_cast<float>(StartDistanceSamples.Last().X);
}

void UMyCharacterAnimInstance::UpdateStartMatching(float DeltaSeconds, float DistanceMoved)
{
	if (!bShouldMove)
	{
		StartAnimationTime = 0.0f;
		StartDistanceTraveled = 0.0f;
		bStartAnimationComplete = false;
		return;
	}
	if (bHadMovementIntent && bStartAnimationComplete)
	{
		return;
	}
	if (!bHadMovementIntent)
	{
		StartAnimationTime = 0.0f;
		StartDistanceTraveled = 0.0f;
		bStartAnimationComplete = false;
	}
	if (bStartProfileValid)
	{
		StartDistanceTraveled += DistanceMoved;
		const float TargetTime = FindStartTime(StartDistanceTraveled);
		StartAnimationTime = FMath::Clamp(TargetTime, StartAnimationTime,
			StartAnimationTime + DeltaSeconds * FMath::Max(0.1f, MaxStartPlayRate));
		bStartAnimationComplete = StartAnimationTime >= StartDistanceSamples.Last().X - UE_KINDA_SMALL_NUMBER;
	}
	else
	{
		// Missing configuration must not trap the character in Start indefinitely.
		bStartAnimationComplete = true;
	}
}

void UMyCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	const ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner());
	const UCharacterMovementComponent* Movement = IsValid(Character)
		? Character->GetCharacterMovement() : nullptr;
	DeltaSeconds = FMath::IsFinite(DeltaSeconds) ? FMath::Max(0.0f, DeltaSeconds) : 0.0f;
	float DistanceMoved = 0.0f;
	if (IsValid(Character))
	{
		const FVector Location = Character->GetActorLocation();
		if (bHasPreviousLocation && IsValid(Movement) && !Movement->bJustTeleported)
		{
			DistanceMoved = FVector::Dist2D(Location, PreviousLocation);
			// Ignore teleports/corrections that are not plausible frame travel.
			if (DistanceMoved > FMath::Max(10.0f, Movement->GetMaxSpeed() * DeltaSeconds * 2.0f))
			{
				DistanceMoved = 0.0f;
			}
		}
		PreviousLocation = Location;
		bHasPreviousLocation = true;
	}
	else
	{
		bHasPreviousLocation = false;
	}
	GroundSpeed = IsValid(Character) ? Character->GetVelocity().Size2D() : 0.0f;
	const UARPGCharacterMovementComponent* GroundMovement = Cast<UARPGCharacterMovementComponent>(Movement);
	bPivoting = IsValid(GroundMovement) && GroundMovement->IsPivoting();
	bShouldMove = IsValid(Movement)
		&& Movement->GetCurrentAcceleration().SizeSquared2D() > UE_KINDA_SMALL_NUMBER && !bPivoting;
	const float TargetWalkRate = GroundSpeed > 3.0f
		? FMath::Clamp(GroundSpeed / FMath::Max(1.0f, WalkAuthoredSpeed), 0.0f, 2.0f) : 0.0f;
	WalkPlayRate = TargetWalkRate <= UE_KINDA_SMALL_NUMBER ? 0.0f
		: FMath::FInterpTo(WalkPlayRate, TargetWalkRate, DeltaSeconds, 12.0f);
	if (!IsValid(Movement) || !Movement->IsMovingOnGround())
	{
		bShouldMove = false;
		UpdateStartMatching(DeltaSeconds, 0.0f);
		ResetStopMatching();
		bHadMovementIntent = false;
		return;
	}
	UpdateStartMatching(DeltaSeconds, DistanceMoved);
	if (bShouldMove)
	{
		ResetStopMatching();
	}
	else
	{
		if (bHadMovementIntent && bStopProfileValid)
		{
			bStopping = true;
			bStopAnimationComplete = false;
			StopSettleElapsed = 0.0f;
			// Strong gameplay braking can predict almost zero distance immediately.
			// Keep a short visible approach to the planted pose instead of entering
			// the stationary tail directly from an arbitrary walk-cycle pose.
			constexpr float VisiblePlantLead = 0.30f;
			const float LatestEntryTime = FMath::Max(0.0f, FindStopTime(0.0f) - VisiblePlantLead);
			StopAnimationTime = FMath::Min(FindStopTime(PredictStopDistance(*Movement)), LatestEntryTime);
		}
		if (bStopping)
		{
			// Relax exact distance matching when braking is faster than the clip:
			// never rewind, but do not skip the remaining foot-plant in one update.
			constexpr float MaxStopPlayRate = 2.0f;
			const float PlantTime = FindStopTime(0.0f);
			const float TargetTime = GroundSpeed <= 3.0f ? PlantTime
				: FindStopTime(PredictStopDistance(*Movement));
			StopAnimationTime = FMath::Max(StopAnimationTime,
				FMath::Min(TargetTime, StopAnimationTime + DeltaSeconds * MaxStopPlayRate));
			if (GroundSpeed <= 3.0f && StopAnimationTime >= PlantTime - UE_KINDA_SMALL_NUMBER)
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

#include "ARPGCharacterMovementComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/WeaponGameplayTags.h"
#include "GameFramework/Character.h"
#include "ARPGDirectionalStartSet.h"

void UARPGCharacterMovementComponent::OnRegister()
{
	Super::OnRegister();
	// Older placed components can serialize an empty value for a newly added
	// defaults-only setting. Inherit the Blueprint's configured movement template.
	if (!IsValid(DirectionalStartSet))
	{
		const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
		const ACharacter* Defaults = OwnerCharacter
			? OwnerCharacter->GetClass()->GetDefaultObject<ACharacter>() : nullptr;
		const UARPGCharacterMovementComponent* DefaultMovement = Defaults
			? Cast<UARPGCharacterMovementComponent>(Defaults->GetCharacterMovement()) : nullptr;
		if (DefaultMovement && DefaultMovement != this)
		{
			DirectionalStartSet = DefaultMovement->DirectionalStartSet;
		}
	}
}

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
	DirectionalProfileIndex = INDEX_NONE;
	bHadInput = false;
}

const FARPGDirectionalStartProfile* UARPGCharacterMovementComponent::GetDirectionalProfile() const
{
	return IsValid(DirectionalStartSet) && DirectionalStartSet->Profiles.IsValidIndex(DirectionalProfileIndex)
		? &DirectionalStartSet->Profiles[DirectionalProfileIndex] : nullptr;
}

bool UARPGCharacterMovementComponent::HasDirectionalStart() const
{
	return GetDirectionalProfile() != nullptr;
}

bool UARPGCharacterMovementComponent::IsDirectionalStartComplete() const
{
	const FARPGDirectionalStartProfile* Profile = GetDirectionalProfile();
	return !Profile || bDirectionalCancelled || DirectionalStartTime >= Profile->MotionSamples.Last().X - UE_KINDA_SMALL_NUMBER;
}

UAnimSequence* UARPGCharacterMovementComponent::GetDirectionalStartAnimation() const
{
	const FARPGDirectionalStartProfile* Profile = GetDirectionalProfile();
	return Profile ? Profile->Animation.Get() : nullptr;
}

float UARPGCharacterMovementComponent::GetDirectionalMeshYaw() const
{
	const FARPGDirectionalStartProfile* Profile = GetDirectionalProfile();
	return Profile && !IsDirectionalStartComplete()
		? -static_cast<float>(Profile->SampleAtTime(DirectionalStartTime).Z) : 0.0f;
}

bool UARPGCharacterMovementComponent::BeginDirectionalStart(const FVector& DesiredDirection)
{
	if (IsSprinting() || !IsValid(DirectionalStartSet))
	{
		return false;
	}
	const float DesiredYaw = FMath::FindDeltaAngleDegrees(CharacterOwner->GetActorRotation().Yaw, DesiredDirection.Rotation().Yaw);
	const FARPGDirectionalStartProfile* Profile = DirectionalStartSet->SelectProfile(DesiredYaw);
	if (!Profile)
	{
		return false;
	}
	DirectionalProfileIndex = static_cast<int32>(Profile - DirectionalStartSet->Profiles.GetData());
	DirectionalInitialYaw = CharacterOwner->GetActorRotation().Yaw;
	DirectionalDeltaYaw = DesiredYaw;
	// A near-180 turn follows the chosen clip's authored side, rather than twisting it backwards.
	if (DirectionalDeltaYaw * Profile->AuthoredYaw < 0.0f)
	{
		DirectionalDeltaYaw += Profile->AuthoredYaw > 0.0f ? 360.0f : -360.0f;
	}
	DirectionalTarget = DesiredDirection;
	DirectionalPreviousLocation = UpdatedComponent->GetComponentLocation();
	DirectionalStartTime = 0.0f;
	DirectionalDistance = 0.0f;
	DirectionalTurnProgress = 0.0f;
	bDirectionalCancelled = false;
	ResetPivot();
	return true;
}

void UARPGCharacterMovementComponent::UpdateDirectionalStart(float DeltaTime, const FVector& DesiredDirection)
{
	const FARPGDirectionalStartProfile* Profile = GetDirectionalProfile();
	if (!Profile || IsDirectionalStartComplete())
	{
		return;
	}
	if (FVector::DotProduct(DirectionalTarget, DesiredDirection) < FMath::Cos(FMath::DegreesToRadians(50.0f)))
	{
		// Finish Start and hand control to the ordinary movement loop when intent changes sharply.
		bDirectionalCancelled = true;
		return;
	}
	const FVector Location = UpdatedComponent->GetComponentLocation();
	const float Distance = FVector::Dist2D(Location, DirectionalPreviousLocation);
	if (!bJustTeleported && Distance <= FMath::Max(10.0f, GetMaxSpeed() * DeltaTime * 2.0f))
	{
		DirectionalDistance += Distance;
	}
	DirectionalPreviousLocation = Location;
	const float TargetTime = Profile->FindTimeAtDistance(DirectionalDistance);
	// Allow the planted turning lead-in to advance even before translation begins.
	DirectionalStartTime = FMath::Min(static_cast<float>(Profile->MotionSamples.Last().X),
		FMath::Clamp(TargetTime, DirectionalStartTime + DeltaTime * 0.6f, DirectionalStartTime + DeltaTime * 1.5f));
	const float BodyYaw = static_cast<float>(Profile->SampleAtTime(DirectionalStartTime).Z);
	DirectionalTurnProgress = FMath::Max(DirectionalTurnProgress, FMath::Clamp(BodyYaw / Profile->AuthoredYaw, 0.0f, 1.0f));
}

void UARPGCharacterMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	PivotCooldown = FMath::Max(0.0f, PivotCooldown - DeltaTime);
	const FVector RequestedAcceleration = Acceleration;
	if (!IsMovingOnGround() || !HasValidData() || HasAnimRootMotion()
		|| CurrentRootMotion.HasOverrideVelocity() || RequestedAcceleration.IsNearlyZero())
	{
		ResetPivot();
		DirectionalProfileIndex = INDEX_NONE;
		bHadInput = false;
		Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
		return;
	}

	const FVector DesiredDirection = RequestedAcceleration.GetSafeNormal2D();
	const float Speed = Velocity.Size2D();
	if (!bHadInput && Speed < PivotMinimumSpeed)
	{
		BeginDirectionalStart(DesiredDirection);
	}
	bHadInput = true;
	UpdateDirectionalStart(DeltaTime, DesiredDirection);
	const float VelocityDot = FVector::DotProduct(Velocity.GetSafeNormal2D(), DesiredDirection);
	if (!IsPivoting() && IsDirectionalStartComplete() && PivotCooldown <= 0.0f && Speed >= PivotMinimumSpeed
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
			|| PivotElapsed > (IsSprinting() ? 1.0f : 0.75f))
		{
			ResetPivot();
			PivotCooldown = 0.15f;
		}
		else
		{
			if (Speed <= PivotTurnSpeedThreshold)
			{
				if (BeginDirectionalStart(DesiredDirection))
				{
					Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
					return;
				}
				bPivotBraking = false;
				bPivotTurning = true;
			}
			if (bPivotTurning && (!IsSprinting() || PivotElapsed >= 0.55f)
				&& FVector::DotProduct(CharacterOwner->GetActorForwardVector(), DesiredDirection)
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
	if (IsActionMovementLocked())
	{
		return;
	}
	if (!IsMovingOnGround() || !bOrientRotationToMovement || !HasValidData()
		|| HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity())
	{
		Super::PhysicsRotation(DeltaTime);
		return;
	}
	if (HasDirectionalStart() && !bDirectionalCancelled)
	{
		const float Yaw = DirectionalInitialYaw + DirectionalDeltaYaw * DirectionalTurnProgress;
		MoveUpdatedComponent(FVector::ZeroVector, FRotator(0.0f, Yaw, 0.0f), false);
		if (!IsDirectionalStartComplete())
		{
			return;
		}
		// Keep the selected clip/time for outgoing evaluation, but release rotation ownership.
		bDirectionalCancelled = true;
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

void UARPGCharacterMovementComponent::SetSprintRequested(bool bRequested)
{
	if (bSprintRequested != bRequested)
	{
		// A walk turn must not keep ownership of rotation during a gait change.
		DirectionalProfileIndex = INDEX_NONE;
		ResetPivot();
		bSprintRequested = bRequested;
	}
}

bool UARPGCharacterMovementComponent::IsSprinting() const
{
	return bSprintRequested && !IsActionMovementLocked() && IsMovingOnGround() && !IsCrouching();
}

float UARPGCharacterMovementComponent::GetMaxSpeed() const
{
	if (IsActionMovementLocked())
	{
		return 0.f;
	}
	return IsSprinting() ? FMath::Max(Super::GetMaxSpeed(), SprintSpeed) : Super::GetMaxSpeed();
}

bool UARPGCharacterMovementComponent::IsActionMovementLocked() const
{
	const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(CharacterOwner);
	const UAbilitySystemComponent* ASC = AbilityOwner ? AbilityOwner->GetAbilitySystemComponent() : nullptr;
	return ASC && (ASC->HasMatchingGameplayTag(WeaponGameplayTags::State_Action_Attacking)
		|| ASC->HasMatchingGameplayTag(WeaponGameplayTags::State_Action_Evading));
}

float UARPGCharacterMovementComponent::GetMaxAcceleration() const
{
	return IsActionMovementLocked() ? 0.f : Super::GetMaxAcceleration();
}

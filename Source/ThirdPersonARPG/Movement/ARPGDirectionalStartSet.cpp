#include "ARPGDirectionalStartSet.h"
#include "Animation/AnimSequence.h"

bool FARPGDirectionalStartProfile::IsUsable() const
{
	if (!IsValid(Animation) || MotionSamples.Num() < 2 || !FMath::IsFinite(AuthoredYaw)
		|| FMath::Abs(AuthoredYaw) < 45.0f || MotionSamples[0].X != 0.0)
	{
		return false;
	}
	for (int32 Index = 0; Index < MotionSamples.Num(); ++Index)
	{
		const FVector& Sample = MotionSamples[Index];
		if (Sample.ContainsNaN() || Sample.X < 0.0 || Sample.Y < 0.0
			|| (Index > 0 && (Sample.X <= MotionSamples[Index - 1].X || Sample.Y < MotionSamples[Index - 1].Y)))
		{
			return false;
		}
	}
	return FMath::Abs(MotionSamples.Last().X - Animation->GetPlayLength()) < 0.02
		&& FMath::Abs(MotionSamples.Last().Z - AuthoredYaw) < 5.0;
}

FVector FARPGDirectionalStartProfile::SampleAtTime(float Time) const
{
	if (MotionSamples.IsEmpty())
	{
		return FVector::ZeroVector;
	}
	for (int32 Index = 1; Index < MotionSamples.Num(); ++Index)
	{
		const FVector& Previous = MotionSamples[Index - 1];
		const FVector& Next = MotionSamples[Index];
		if (Time <= Next.X)
		{
			const double Alpha = FMath::Clamp((Time - Previous.X) / (Next.X - Previous.X), 0.0, 1.0);
			return FMath::Lerp(Previous, Next, Alpha);
		}
	}
	return MotionSamples.Last();
}

float FARPGDirectionalStartProfile::FindTimeAtDistance(float Distance) const
{
	if (MotionSamples.IsEmpty())
	{
		return 0.0f;
	}
	for (int32 Index = 1; Index < MotionSamples.Num(); ++Index)
	{
		const FVector& Previous = MotionSamples[Index - 1];
		const FVector& Next = MotionSamples[Index];
		if (Distance <= Next.Y)
		{
			const double Span = Next.Y - Previous.Y;
			const double Alpha = Span > UE_SMALL_NUMBER
				? FMath::Clamp((Distance - Previous.Y) / Span, 0.0, 1.0) : 0.0;
			return static_cast<float>(FMath::Lerp(Previous.X, Next.X, Alpha));
		}
	}
	return static_cast<float>(MotionSamples.Last().X);
}

const FARPGDirectionalStartProfile* UARPGDirectionalStartSet::SelectProfile(float DesiredYaw) const
{
	if (FMath::Abs(DesiredYaw) < 45.0f)
	{
		return nullptr;
	}
	const FARPGDirectionalStartProfile* Best = nullptr;
	float BestError = TNumericLimits<float>::Max();
	for (const FARPGDirectionalStartProfile& Profile : Profiles)
	{
		if (Profile.IsUsable())
		{
			const float Error = FMath::Abs(FMath::FindDeltaAngleDegrees(DesiredYaw, Profile.AuthoredYaw));
			if (Error < BestError)
			{
				BestError = Error;
				Best = &Profile;
			}
		}
	}
	return Best;
}

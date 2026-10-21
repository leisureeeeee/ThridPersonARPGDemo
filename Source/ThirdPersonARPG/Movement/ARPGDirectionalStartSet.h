#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ARPGDirectionalStartSet.generated.h"

class UAnimSequence;

/** X=time, Y=horizontal travel, Z=body yaw relative to the forward start pose. */
USTRUCT(BlueprintType)
struct FARPGDirectionalStartProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Directional Start")
	TObjectPtr<UAnimSequence> Animation;

	UPROPERTY(EditAnywhere, Category = "Directional Start")
	float AuthoredYaw = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Directional Start")
	TArray<FVector> MotionSamples;

	bool IsUsable() const;
	FVector SampleAtTime(float Time) const;
	float FindTimeAtDistance(float Distance) const;
};

/** Explicit animation configuration shared by movement and animation, with no hardcoded asset paths. */
UCLASS(BlueprintType)
class THIRDPERSONARPG_API UARPGDirectionalStartSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Directional Start")
	TArray<FARPGDirectionalStartProfile> Profiles;

	const FARPGDirectionalStartProfile* SelectProfile(float DesiredYaw) const;
};

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGAnimationAssetTools.generated.h"

class UAnimSequence;
class USkeleton;

/** Editor asset maintenance bridge. Never changes animation keyframes or root-motion settings. */
UCLASS()
class THIRDPERSONARPG_API UARPGAnimationAssetTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Empty string means every track maps, or is explicitly allowed to remain unused. */
	UFUNCTION(BlueprintCallable, Category = "Editor|Animation Assets", meta = (DevelopmentOnly))
	static FString ValidateSkeletonAssignment(UAnimSequence* Animation, USkeleton* Skeleton, const TArray<FName>& AllowedUnmappedTracks);

	/** Caller owns backups and saving; returns an error string, or empty on success. */
	UFUNCTION(BlueprintCallable, Category = "Editor|Animation Assets", meta = (DevelopmentOnly))
	static FString AssignCompatibleSkeleton(UAnimSequence* Animation, USkeleton* Skeleton, const TArray<FName>& AllowedUnmappedTracks);
};

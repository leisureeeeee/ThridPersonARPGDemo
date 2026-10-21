#include "ARPGAnimationAssetTools.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"

#if WITH_EDITOR
#include "Animation/AnimData/IAnimationDataModel.h"
#endif

FString UARPGAnimationAssetTools::ValidateSkeletonAssignment(UAnimSequence* Animation, USkeleton* Skeleton, const TArray<FName>& AllowedUnmappedTracks)
{
#if WITH_EDITOR
	if (!IsValid(Animation) || !IsValid(Skeleton))
	{
		return TEXT("Animation and target skeleton must be valid.");
	}
	const IAnimationDataModel* Model = Animation->GetDataModel();
	if (!Model)
	{
		return TEXT("Missing source animation data model.");
	}
	TArray<FName> Tracks;
	Model->GetBoneTrackNames(Tracks);
	if (Tracks.IsEmpty())
	{
		return TEXT("No bone animation tracks; inspect this asset manually.");
	}
	TArray<FString> Missing;
	for (FName Track : Tracks)
	{
		if (Skeleton->GetReferenceSkeleton().FindBoneIndex(Track) == INDEX_NONE && !AllowedUnmappedTracks.Contains(Track))
		{
			Missing.Add(Track.ToString());
		}
	}
	return Missing.IsEmpty() ? FString() : TEXT("Missing animated bones: ") + FString::Join(Missing, TEXT(", "));
#else
	return TEXT("Skeleton assignment is editor-only.");
#endif
}

FString UARPGAnimationAssetTools::AssignCompatibleSkeleton(UAnimSequence* Animation, USkeleton* Skeleton, const TArray<FName>& AllowedUnmappedTracks)
{
#if WITH_EDITOR
	const FString Error = ValidateSkeletonAssignment(Animation, Skeleton, AllowedUnmappedTracks);
	if (!Error.IsEmpty() || Animation->GetSkeleton() == Skeleton)
	{
		return Error;
	}
	Animation->Modify();
	TArray<FName> OriginalTracks;
	Animation->GetDataModel()->GetBoneTrackNames(OriginalTracks);
	const int32 OriginalKeys = Animation->GetDataModel()->GetNumberOfKeys();
	Animation->SetSkeleton(Skeleton);
	Animation->PostEditChange();
	TArray<FName> ResultTracks;
	Animation->GetDataModel()->GetBoneTrackNames(ResultTracks);
	if (OriginalTracks != ResultTracks || OriginalKeys != Animation->GetDataModel()->GetNumberOfKeys())
	{
		return TEXT("Source track layout changed; do not save. Restore from backup and inspect.");
	}
	Animation->MarkPackageDirty();
	return Animation->GetSkeleton() == Skeleton ? FString() : TEXT("Skeleton assignment did not persist in memory.");
#else
	return TEXT("Skeleton assignment is editor-only.");
#endif
}

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_DrawWeapon.generated.h"

/** Commits a draw at the authored grip frame. Stores no per-character state. */
UCLASS()
class THIRDPERSONARPG_API UAnimNotify_DrawWeapon : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};

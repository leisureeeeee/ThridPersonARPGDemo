#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_SheatheWeapon.generated.h"

/** Commits the return to the holster at an authored frame. */
UCLASS()
class THIRDPERSONARPG_API UAnimNotify_SheatheWeapon : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};

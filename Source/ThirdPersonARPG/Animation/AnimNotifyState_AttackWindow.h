#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Combat/AttackWindow.h"
#include "AnimNotifyState_AttackWindow.generated.h"

// Shared animation asset objects hold configuration only, never per-character state.
UCLASS(meta = (DisplayName = "Attack Window"))
class THIRDPERSONARPG_API UAnimNotifyState_AttackWindow : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    UAnimNotifyState_AttackWindow();
    EAttackWindow GetWindow() const { return Window; }
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

private:
    void SendEvent(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, bool bBegin) const;

    UPROPERTY(EditAnywhere, Category = "Attack")
    EAttackWindow Window = EAttackWindow::Hit;
};

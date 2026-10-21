#include "AnimNotify_DrawWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "MyCharacter/MyCharacter.h"

void UAnimNotify_DrawWeapon::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (MeshComp)
	{
		if (AMyCharacter* Character = Cast<AMyCharacter>(MeshComp->GetOwner()))
		{
			Character->HandleWeaponGripNotify(Animation);
		}
	}
}

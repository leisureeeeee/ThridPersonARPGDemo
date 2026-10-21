#include "AnimNotify_SheatheWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "MyCharacter/MyCharacter.h"

void UAnimNotify_SheatheWeapon::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (MeshComp)
	{
		if (AMyCharacter* Character = Cast<AMyCharacter>(MeshComp->GetOwner()))
		{
			Character->HandleWeaponSheathNotify(Animation);
		}
	}
}

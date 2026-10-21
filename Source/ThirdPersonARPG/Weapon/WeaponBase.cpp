#include "WeaponBase.h"
#include "Components/SceneComponent.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	SetRootComponent(WeaponRoot);
}

bool AWeaponBase::ActivateWeapon()
{
	return true;
}

void AWeaponBase::DeactivateWeapon()
{
}

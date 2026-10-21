#include "LightsaberWeapon.h"
#include "Components/StaticMeshComponent.h"

ALightsaberWeapon::ALightsaberWeapon()
{
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetRootComponent());
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
}

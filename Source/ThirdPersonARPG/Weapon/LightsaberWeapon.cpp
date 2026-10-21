#include "LightsaberWeapon.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

ALightsaberWeapon::ALightsaberWeapon()
{
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetRootComponent());
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
}

void ALightsaberWeapon::BeginPlay()
{
	Super::BeginPlay();
	// No ignition sound or automatic activation when the weapon is spawned.
	Extinguish();
}

bool ALightsaberWeapon::InitializeBladeMaterial()
{
	if (IsValid(BladeMaterialInstance.Get()))
	{
		return true;
	}

	const int32 SlotIndex = WeaponMesh->GetMaterialIndex(BladeMaterialSlot);
	if (SlotIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Blade material slot '%s' is missing."),
			*GetName(), *BladeMaterialSlot.ToString());
		return false;
	}

	UMaterialInterface* Source = WeaponMesh->GetMaterial(SlotIndex);
	float EnabledValue = 0.0f;
	if (!IsValid(Source) || Source->GetBlendMode() != BLEND_Masked
		|| !Source->GetScalarParameterValue(FMaterialParameterInfo(TEXT("BladeEnabled")), EnabledValue))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Assign a masked blade material with a BladeEnabled scalar parameter."), *GetName());
		return false;
	}

	BladeMaterialInstance = WeaponMesh->CreateDynamicMaterialInstance(SlotIndex, Source);
	return IsValid(BladeMaterialInstance.Get());
}

bool ALightsaberWeapon::SetBladeEnabled(bool bEnabled)
{
	if (!InitializeBladeMaterial())
	{
		return false;
	}

	// Change only this actor's blade slot, preserving its hilt and other weapons.
	BladeMaterialInstance->SetScalarParameterValue(TEXT("BladeEnabled"), bEnabled ? 1.0f : 0.0f);
	bIsIgnited = bEnabled;
	return true;
}

bool ALightsaberWeapon::Ignite()
{
	return SetBladeEnabled(true);
}

bool ALightsaberWeapon::Extinguish()
{
	return SetBladeEnabled(false);
}

bool ALightsaberWeapon::ActivateWeapon()
{
	return Ignite();
}

void ALightsaberWeapon::DeactivateWeapon()
{
	Extinguish();
}

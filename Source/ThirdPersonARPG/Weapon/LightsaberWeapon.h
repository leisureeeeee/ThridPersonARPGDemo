#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "LightsaberWeapon.generated.h"

class UStaticMeshComponent;

/** Static-mesh lightsaber; appearance is configured by its Blueprint subclass. */
UCLASS(Blueprintable)
class THIRDPERSONARPG_API ALightsaberWeapon : public AWeaponBase
{
	GENERATED_BODY()

public:
	ALightsaberWeapon();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> WeaponMesh;
};

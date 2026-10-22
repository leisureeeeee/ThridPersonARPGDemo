#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

class USceneComponent;

/** Common actor identity and attachment root for weapon types. */
UCLASS(Abstract, Blueprintable)
class THIRDPERSONARPG_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

	virtual bool ActivateWeapon();
	virtual bool DeactivateWeapon();

	// World-space blade endpoints supplied by the concrete weapon.
	virtual bool GetBladeSegment(FVector& OutBase, FVector& OutTip) const { return false; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> WeaponRoot;
};

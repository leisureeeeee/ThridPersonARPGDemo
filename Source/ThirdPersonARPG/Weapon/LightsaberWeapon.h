#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "LightsaberWeapon.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/** Static-mesh lightsaber; appearance is configured by its Blueprint subclass. */
UCLASS(Blueprintable)
class THIRDPERSONARPG_API ALightsaberWeapon : public AWeaponBase
{
	GENERATED_BODY()

public:
	ALightsaberWeapon();
	virtual bool ActivateWeapon() override;
	virtual void DeactivateWeapon() override;

	UFUNCTION(BlueprintCallable, Category = "Weapon|Lightsaber")
	bool Ignite();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Lightsaber")
	bool Extinguish();

	UFUNCTION(BlueprintPure, Category = "Weapon|Lightsaber")
	bool IsIgnited() const { return bIsIgnited; }

protected:
	virtual void BeginPlay() override;

private:
	bool SetBladeEnabled(bool bEnabled);
	bool InitializeBladeMaterial();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Lightsaber")
	FName BladeMaterialSlot = TEXT("SaberBlade");

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BladeMaterialInstance;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Weapon|Lightsaber", meta = (AllowPrivateAccess = "true"))
	bool bIsIgnited = false;
};

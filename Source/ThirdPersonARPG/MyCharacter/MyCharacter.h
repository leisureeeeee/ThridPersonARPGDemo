// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MyCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UARPGCameraRigComponent;
class AWeaponBase;

UCLASS()
class THIRDPERSONARPG_API AMyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "Weapon")
	AWeaponBase* GetCurrentWeapon() const;

protected:
	virtual void PostInitializeComponents() override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UARPGCameraRigComponent> CameraRig;

	// An actor instance reference, not a weapon class or an automatically spawned weapon.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AWeaponBase> CurrentWeapon = nullptr;
};

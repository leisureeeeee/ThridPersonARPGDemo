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
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void InitializeWeapon();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UARPGCameraRigComponent> CameraRig;

	// Optional placed weapon; otherwise BeginPlay creates DefaultWeaponClass.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AWeaponBase> CurrentWeapon = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AWeaponBase> DefaultWeaponClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName HolsteredWeaponSocket = TEXT("scabbard1");

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FTransform HolsteredWeaponOffset = FTransform::Identity;

	// Only an actor spawned by this character is destroyed with it.
	UPROPERTY(Transient)
	TObjectPtr<AWeaponBase> SpawnedWeapon;
};

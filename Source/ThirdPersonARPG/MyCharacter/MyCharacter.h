// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "MyCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UARPGCameraRigComponent;
class AWeaponBase;
class UAnimSequence;
class UAnimSequenceBase;
class UAnimMontage;
class UAnimNotify_DrawWeapon;
class UAnimNotify_SheatheWeapon;
class UMyCharacterAbilitySystemComponent;

UCLASS()
class THIRDPERSONARPG_API AMyCharacter : public ACharacterBase
{
	GENERATED_BODY()

public:
	AMyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Input-independent request; equipment rules stay on the character.
	void SetSprintRequested(bool bRequested);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	AWeaponBase* GetCurrentWeapon() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool DrawWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool ToggleWeapon();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool RequestLightAttack();

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAttacking() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool SheatheWeapon();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsSheathingWeapon() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsDrawingWeapon() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsWeaponDrawn() const;

protected:
	virtual void PostInitializeComponents() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void OnRep_Controller() override;
	virtual void UnPossessed() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	friend class UAnimNotify_DrawWeapon;
	friend class UAnimNotify_SheatheWeapon;
	void InitializeAbilitySystem();
	void ReleaseAbilitySystem();
	UMyCharacterAbilitySystemComponent* GetWeaponASC() const;
	void StopWeaponMontages();
	void SyncWeaponStateFromAttachment();
	void InitializeWeapon();
	void HandleWeaponGripNotify(UAnimSequenceBase* Animation);
	void HandleDrawMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void HandleWeaponSheathNotify(UAnimSequenceBase* Animation);
	void HandleSheatheMontageEnded(UAnimMontage* Montage, bool bInterrupted);

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

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Draw")
	TObjectPtr<UAnimSequence> DrawWeaponAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Draw")
	FName DrawWeaponSlot = TEXT("DefaultSlot");

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Draw")
	FName DrawnWeaponSocket = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Draw")
	FTransform DrawnWeaponOffset = FTransform::Identity;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveDrawMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Draw", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float DrawWeaponPlayRate = 1.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Sheathe")
	TObjectPtr<UAnimSequence> SheatheWeaponAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Sheathe", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float SheatheWeaponPlayRate = 1.3f;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveSheatheMontage;
};

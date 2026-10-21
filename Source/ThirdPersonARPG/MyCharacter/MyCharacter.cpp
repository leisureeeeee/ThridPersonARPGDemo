// Fill out your copyright notice in the Description page of Project Settings.

#include "MyCharacter.h"
#include "Weapon/WeaponBase.h"
#include "Engine/World.h"
#include "Camera/ARPGCameraRigComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Movement/ARPGCharacterMovementComponent.h"

AMyCharacter::AMyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UARPGCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	// The capsule origin is its center; this mesh's origin is at the feet.
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f,
		-GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));

	// The mannequin mesh faces +Y; CharacterMovement turns the capsule's +X.
	// Keep this asset alignment on the mesh so movement and camera axes stay intact.
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	CameraRig = CreateDefaultSubobject<UARPGCameraRigComponent>(TEXT("CameraRig"));

	AutoPossessPlayer = EAutoReceiveInput::Player0;
	bUseControllerRotationYaw = false;

	// Match the walk cycle's average travel speed (178.42 cm over 1.0667 s).
	GetCharacterMovement()->MaxWalkSpeed = 167.0f;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
}

void AMyCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ensure(CameraRig->InitializeRig(SpringArm, FollowCamera));
}

AWeaponBase* AMyCharacter::GetCurrentWeapon() const
{
	return IsValid(CurrentWeapon.Get()) ? CurrentWeapon.Get() : nullptr;
}

void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeWeapon();
}

void AMyCharacter::InitializeWeapon()
{
	// Older placed actors can retain an empty value for newly introduced defaults.
	// This is defaults-only configuration; read the Blueprint template without editing it.
	if (!DefaultWeaponClass)
	{
		const AMyCharacter* Defaults = GetClass()->GetDefaultObject<AMyCharacter>();
		if (Defaults && Defaults != this)
		{
			DefaultWeaponClass = Defaults->DefaultWeaponClass;
		}
	}
	if (!IsValid(CurrentWeapon.Get()) && !DefaultWeaponClass)
	{
		return;
	}

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!IsValid(CharacterMesh) || HolsteredWeaponSocket.IsNone()
		|| !CharacterMesh->DoesSocketExist(HolsteredWeaponSocket))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Weapon socket '%s' is missing; weapon was not equipped."),
			*GetName(), *HolsteredWeaponSocket.ToString());
		return;
	}

	AWeaponBase* Weapon = GetCurrentWeapon();
	if (Weapon && ((Weapon->GetOwner() && Weapon->GetOwner() != this)
		|| (Weapon->GetAttachParentActor() && Weapon->GetAttachParentActor() != this)))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Configured weapon is already owned or attached to another actor."), *GetName());
		CurrentWeapon = nullptr;
		return;
	}

	if (!Weapon)
	{
		UWorld* World = GetWorld();
		if (!World || DefaultWeaponClass->HasAnyClassFlags(CLASS_Abstract))
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: Default weapon class cannot be spawned."), *GetName());
			return;
		}

		FActorSpawnParameters Parameters;
		Parameters.Owner = this;
		Parameters.Instigator = this;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Weapon = World->SpawnActor<AWeaponBase>(DefaultWeaponClass,
			CharacterMesh->GetSocketTransform(HolsteredWeaponSocket), Parameters);
		if (!IsValid(Weapon))
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: Failed to spawn the default weapon."), *GetName());
			return;
		}
		SpawnedWeapon = Weapon;
	}

	if (!Weapon->AttachToComponent(CharacterMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale, HolsteredWeaponSocket))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Failed to attach weapon to '%s'."),
			*GetName(), *HolsteredWeaponSocket.ToString());
		if (Weapon == SpawnedWeapon.Get())
		{
			Weapon->Destroy();
			SpawnedWeapon = nullptr;
		}
		CurrentWeapon = nullptr;
		return;
	}

	Weapon->SetActorRelativeTransform(HolsteredWeaponOffset);
	CurrentWeapon = Weapon;
}

void AMyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(SpawnedWeapon.Get()))
	{
		SpawnedWeapon->Destroy();
	}
	// A level-placed weapon is borrowed, so release its attachment rather than destroy it.
	if (AWeaponBase* Weapon = GetCurrentWeapon(); Weapon && Weapon != SpawnedWeapon.Get()
		&& Weapon->GetAttachParentActor() == this)
	{
		Weapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	CurrentWeapon = nullptr;
	SpawnedWeapon = nullptr;
	Super::EndPlay(EndPlayReason);
}

// Fill out your copyright notice in the Description page of Project Settings.

#include "MyCharacter.h"
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
	SpringArm->TargetArmLength = 300.0f;
	SpringArm->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

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

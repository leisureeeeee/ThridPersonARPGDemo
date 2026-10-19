// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacter.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"

// Sets default values
AMyCharacter::AMyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 300.0f;
	SpringArm->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	AutoPossessPlayer = EAutoReceiveInput::Player0;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
}

// Called when the game starts or when spawned
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (ensure(EnhancedInput))
	{
		CreateInputMappingContext();
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCharacter::Move);
	}
}

void AMyCharacter::CreateInputMappingContext()
{
	if (CharacterMappingContext)
	{
		return;
	}

	// Each character owns its runtime input objects; no Content assets are required.
	MoveAction = NewObject<UInputAction>(this, TEXT("MoveAction"), RF_Transient);
	MoveAction->ValueType = EInputActionValueType::Axis2D;
	MoveAction->AccumulationBehavior = EInputActionAccumulationBehavior::Cumulative;
	CharacterMappingContext = NewObject<UInputMappingContext>(this, TEXT("CharacterMappingContext"), RF_Transient);

	CharacterMappingContext->MapKey(MoveAction, EKeys::D);
	CharacterMappingContext->MapKey(MoveAction, EKeys::A).Modifiers.Add(
		NewObject<UInputModifierNegate>(CharacterMappingContext, NAME_None, RF_Transient));

	UInputModifierSwizzleAxis* ForwardSwizzle = NewObject<UInputModifierSwizzleAxis>(
		CharacterMappingContext, NAME_None, RF_Transient);
	ForwardSwizzle->Order = EInputAxisSwizzle::YXZ;
	CharacterMappingContext->MapKey(MoveAction, EKeys::W).Modifiers.Add(ForwardSwizzle);

	FEnhancedActionKeyMapping& BackwardMapping = CharacterMappingContext->MapKey(MoveAction, EKeys::S);
	BackwardMapping.Modifiers.Add(NewObject<UInputModifierNegate>(CharacterMappingContext, NAME_None, RF_Transient));
	UInputModifierSwizzleAxis* BackwardSwizzle = NewObject<UInputModifierSwizzleAxis>(
		CharacterMappingContext, NAME_None, RF_Transient);
	BackwardSwizzle->Order = EInputAxisSwizzle::YXZ;
	BackwardMapping.Modifiers.Add(BackwardSwizzle);
}

void AMyCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	RemoveInputMappingContext();

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController && PlayerController->IsLocalController() && CharacterMappingContext)
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			MappingSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
			if (MappingSubsystem.IsValid())
			{
				MappingSubsystem->AddMappingContext(CharacterMappingContext, 0);
			}
		}
	}
}

void AMyCharacter::UnPossessed()
{
	RemoveInputMappingContext();
	Super::UnPossessed();
}

void AMyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveInputMappingContext();
	Super::EndPlay(EndPlayReason);
}

void AMyCharacter::RemoveInputMappingContext()
{
	if (MappingSubsystem.IsValid() && CharacterMappingContext)
	{
		MappingSubsystem->RemoveMappingContext(CharacterMappingContext);
	}
	MappingSubsystem.Reset();
}

void AMyCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	const FVector2D MovementInput = Value.Get<FVector2D>().GetClampedToMaxSize(1.0f);
	const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), MovementInput.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), MovementInput.X);
}

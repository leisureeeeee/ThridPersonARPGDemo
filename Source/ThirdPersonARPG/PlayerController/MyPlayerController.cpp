#include "MyPlayerController.h"
#include "MyCharacter/MyCharacter.h"
#include "Camera/ARPGCameraRigComponent.h"
#include "Camera/ARPGPlayerCameraManager.h"
#include "Camera/CameraTypes.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

AMyPlayerController::AMyPlayerController()
{
	PlayerCameraManagerClass = AARPGPlayerCameraManager::StaticClass();
}

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalPlayerController())
	{
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(false);
		SetInputMode(InputMode);
		bShowMouseCursor = false;
	}
}

void AMyPlayerController::AutoManageActiveCameraTarget(AActor* SuggestedTarget)
{
	if (bAutoManageActiveCameraTarget)
	{
		AARPGPlayerCameraManager* CameraManager = Cast<AARPGPlayerCameraManager>(PlayerCameraManager);
		if (CameraManager && CameraManager->TryUseControlledPawnCamera(Cast<APawn>(SuggestedTarget)))
		{
			return;
		}
	}

	Super::AutoManageActiveCameraTarget(SuggestedTarget);
}

void AMyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!ensure(EnhancedInput))
	{
		return;
	}

	CreateInputMappingContext();
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyPlayerController::HandleMoveInput);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyPlayerController::HandleLookInput);
	EnhancedInput->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AMyPlayerController::HandleZoomInput);
	EnhancedInput->BindAction(DrawWeaponAction, ETriggerEvent::Started, this, &AMyPlayerController::HandleDrawWeaponInput);
	EnhancedInput->BindAction(SheatheWeaponAction, ETriggerEvent::Started, this, &AMyPlayerController::HandleSheatheWeaponInput);
	RegisterInputMappingContext();
}

void AMyPlayerController::CreateInputMappingContext()
{
	if (PlayerMappingContext)
	{
		return;
	}

	MoveAction = NewObject<UInputAction>(this, TEXT("MoveAction"), RF_Transient);
	MoveAction->ValueType = EInputActionValueType::Axis2D;
	MoveAction->AccumulationBehavior = EInputActionAccumulationBehavior::Cumulative;
	PlayerMappingContext = NewObject<UInputMappingContext>(this, TEXT("PlayerMappingContext"), RF_Transient);

	PlayerMappingContext->MapKey(MoveAction, EKeys::D);
	PlayerMappingContext->MapKey(MoveAction, EKeys::A).Modifiers.Add(
		NewObject<UInputModifierNegate>(PlayerMappingContext, NAME_None, RF_Transient));

	// Keyboard keys report on X; forward and backward need to report on Y.
	UInputModifierSwizzleAxis* ForwardSwizzle = NewObject<UInputModifierSwizzleAxis>(
		PlayerMappingContext, NAME_None, RF_Transient);
	ForwardSwizzle->Order = EInputAxisSwizzle::YXZ;
	PlayerMappingContext->MapKey(MoveAction, EKeys::W).Modifiers.Add(ForwardSwizzle);

	FEnhancedActionKeyMapping& BackwardMapping = PlayerMappingContext->MapKey(MoveAction, EKeys::S);
	BackwardMapping.Modifiers.Add(NewObject<UInputModifierNegate>(PlayerMappingContext, NAME_None, RF_Transient));
	UInputModifierSwizzleAxis* BackwardSwizzle = NewObject<UInputModifierSwizzleAxis>(
		PlayerMappingContext, NAME_None, RF_Transient);
	BackwardSwizzle->Order = EInputAxisSwizzle::YXZ;
	BackwardMapping.Modifiers.Add(BackwardSwizzle);

	LookAction = NewObject<UInputAction>(this, TEXT("LookAction"), RF_Transient);
	LookAction->ValueType = EInputActionValueType::Axis2D;
	// Pass mouse Y through without an additional pitch inversion.
	PlayerMappingContext->MapKey(LookAction, EKeys::Mouse2D);

	ZoomAction = NewObject<UInputAction>(this, TEXT("ZoomAction"), RF_Transient);
	ZoomAction->ValueType = EInputActionValueType::Axis1D;
	// Wheel up is positive (closer); wheel down is negative (farther).
	PlayerMappingContext->MapKey(ZoomAction, EKeys::MouseWheelAxis);

	DrawWeaponAction = NewObject<UInputAction>(this, TEXT("DrawWeaponAction"), RF_Transient);
	DrawWeaponAction->ValueType = EInputActionValueType::Boolean;
	PlayerMappingContext->MapKey(DrawWeaponAction, EKeys::LeftMouseButton);

	SheatheWeaponAction = NewObject<UInputAction>(this, TEXT("SheatheWeaponAction"), RF_Transient);
	SheatheWeaponAction->ValueType = EInputActionValueType::Boolean;
	PlayerMappingContext->MapKey(SheatheWeaponAction, EKeys::RightMouseButton);
}

void AMyPlayerController::RegisterInputMappingContext()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (Subsystem && MappingSubsystem.Get() != Subsystem)
	{
		RemoveInputMappingContext();
		Subsystem->AddMappingContext(PlayerMappingContext, 0);
		MappingSubsystem = Subsystem;
	}
}

void AMyPlayerController::RemoveInputMappingContext()
{
	if (MappingSubsystem.IsValid() && PlayerMappingContext)
	{
		MappingSubsystem->RemoveMappingContext(PlayerMappingContext);
	}
	MappingSubsystem.Reset();
}

void AMyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveInputMappingContext();
	Super::EndPlay(EndPlayReason);
}

void AMyPlayerController::HandleMoveInput(const FInputActionValue& Value)
{
	// Resolve the current pawn for every input, including after possession changes.
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !ControlledPawn->InputEnabled() || IsMoveInputIgnored())
	{
		return;
	}

	const FVector2D MovementInput = Value.Get<FVector2D>().GetClampedToMaxSize(1.0f);
	// Use this pawn's base camera, before CameraManager shakes/modifiers. Its
	// authored relative rotation can differ from the controller's rotation.
	FMinimalViewInfo CameraView;
	ControlledPawn->CalcCamera(0.0f, CameraView);
	const FRotator YawRotation(0.0f, CameraView.Rotation.Yaw, 0.0f);
	ControlledPawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), MovementInput.Y);
	ControlledPawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), MovementInput.X);
}

void AMyPlayerController::HandleLookInput(const FInputActionValue& Value)
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !ControlledPawn->InputEnabled() || IsLookInputIgnored())
	{
		return;
	}

	const FVector2D LookInput = Value.Get<FVector2D>();
	AddYawInput(LookInput.X * MouseSensitivity.X);
	AddPitchInput(LookInput.Y * MouseSensitivity.Y);
}

void AMyPlayerController::HandleZoomInput(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (!IsLocalPlayerController() || !ControlledPawn || !ControlledPawn->InputEnabled()
		|| IsLookInputIgnored() || GetViewTarget() != ControlledPawn)
	{
		return;
	}

	if (UARPGCameraRigComponent* Rig = ControlledPawn->FindComponentByClass<UARPGCameraRigComponent>())
	{
		// Wheel input is a delta, not a rate: do not multiply by DeltaTime.
		Rig->AdjustCameraDistance(Value.Get<float>());
	}
}

void AMyPlayerController::HandleDrawWeaponInput(const FInputActionValue& Value)
{
	AMyCharacter* ControlledCharacter = Cast<AMyCharacter>(GetPawn());
	if (IsLocalPlayerController() && ControlledCharacter && ControlledCharacter->InputEnabled()
		&& !IsMoveInputIgnored() && GetViewTarget() == ControlledCharacter && Value.Get<bool>())
	{
		ControlledCharacter->DrawWeapon();
	}
}

void AMyPlayerController::HandleSheatheWeaponInput(const FInputActionValue& Value)
{
	AMyCharacter* ControlledCharacter = Cast<AMyCharacter>(GetPawn());
	if (IsLocalPlayerController() && ControlledCharacter && ControlledCharacter->InputEnabled()
		&& !IsMoveInputIgnored() && GetViewTarget() == ControlledCharacter && Value.Get<bool>())
	{
		ControlledCharacter->SheatheWeapon();
	}
}

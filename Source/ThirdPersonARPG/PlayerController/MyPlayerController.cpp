#include "MyPlayerController.h"
#include "MyCharacter/MyCharacter.h"
#include "Camera/ARPGCameraRigComponent.h"
#include "Camera/ARPGPlayerCameraManager.h"
#include "Camera/CameraTypes.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
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
	EnhancedInput->BindAction(ToggleWeaponAction, ETriggerEvent::Started, this, &AMyPlayerController::HandleToggleWeaponInput);
	EnhancedInput->BindAction(SprintAction, ETriggerEvent::Triggered, this, &AMyPlayerController::HandleSprintInput);
	EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &AMyPlayerController::HandleSprintInput);
	EnhancedInput->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AMyPlayerController::HandleSprintInput);
	EnhancedInput->BindAction(LightAttackAction, ETriggerEvent::Started, this, &AMyPlayerController::HandleLightAttackInput);
	EnhancedInput->BindAction(EvadeAction, ETriggerEvent::Started, this, &AMyPlayerController::HandleEvadeInput);
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

	ToggleWeaponAction = NewObject<UInputAction>(this, TEXT("ToggleWeaponAction"), RF_Transient);
	ToggleWeaponAction->ValueType = EInputActionValueType::Boolean;
	PlayerMappingContext->MapKey(ToggleWeaponAction, EKeys::LeftAlt);
	PlayerMappingContext->MapKey(ToggleWeaponAction, EKeys::RightAlt);

	LightAttackAction = NewObject<UInputAction>(this, TEXT("LightAttackAction"), RF_Transient);
	LightAttackAction->ValueType = EInputActionValueType::Boolean;
	PlayerMappingContext->MapKey(LightAttackAction, EKeys::LeftMouseButton);

	EvadeAction = NewObject<UInputAction>(this, TEXT("EvadeAction"), RF_Transient);
	EvadeAction->ValueType = EInputActionValueType::Boolean;
	PlayerMappingContext->MapKey(EvadeAction, EKeys::SpaceBar);

	SprintAction = NewObject<UInputAction>(this, TEXT("SprintAction"), RF_Transient);
	SprintAction->ValueType = EInputActionValueType::Boolean;
	PlayerMappingContext->MapKey(SprintAction, EKeys::LeftShift);
	PlayerMappingContext->MapKey(SprintAction, EKeys::RightShift);
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
	if (AMyCharacter* ControlledCharacter = Cast<AMyCharacter>(GetPawn()))
	{
		ControlledCharacter->SetSprintRequested(false);
	}
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
	ControlledPawn->AddMovementInput(GetWorldMovementInput(MovementInput));
}

FVector AMyPlayerController::GetWorldMovementInput(const FVector2D& Input) const
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return FVector::ZeroVector;
	}
	FMinimalViewInfo CameraView;
	ControlledPawn->CalcCamera(0.0f, CameraView);
	const FRotator YawRotation(0.f, CameraView.Rotation.Yaw, 0.f);
	return FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X) * Input.Y
		+ FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y) * Input.X;
}

void AMyPlayerController::HandleEvadeInput(const FInputActionValue& Value)
{
	AMyCharacter* ControlledCharacter = Cast<AMyCharacter>(GetPawn());
	if (IsLocalPlayerController() && ControlledCharacter && ControlledCharacter->InputEnabled()
		&& !IsMoveInputIgnored() && GetViewTarget() == ControlledCharacter && Value.Get<bool>())
	{
		const UEnhancedPlayerInput* EnhancedPlayerInput = Cast<UEnhancedPlayerInput>(PlayerInput);
		const FVector2D MovementInput = EnhancedPlayerInput
			? EnhancedPlayerInput->GetActionValue(MoveAction).Get<FVector2D>().GetClampedToMaxSize(1.f)
			: FVector2D::ZeroVector;
		ControlledCharacter->RequestEvade(GetWorldMovementInput(MovementInput));
	}
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

void AMyPlayerController::HandleToggleWeaponInput(const FInputActionValue& Value)
{
	AMyCharacter* ControlledCharacter = Cast<AMyCharacter>(GetPawn());
	if (IsLocalPlayerController() && ControlledCharacter && ControlledCharacter->InputEnabled()
		&& !IsMoveInputIgnored() && GetViewTarget() == ControlledCharacter && Value.Get<bool>())
	{
		ControlledCharacter->ToggleWeapon();
	}
}

void AMyPlayerController::OnUnPossess()
{
	if (AMyCharacter* ControlledCharacter = Cast<AMyCharacter>(GetPawn()))
	{
		ControlledCharacter->SetSprintRequested(false);
	}
	Super::OnUnPossess();
}

void AMyPlayerController::HandleSprintInput(const FInputActionValue& Value)
{
	if (AMyCharacter* ControlledCharacter = Cast<AMyCharacter>(GetPawn()))
	{
		const bool bAllowed = IsLocalPlayerController() && ControlledCharacter->InputEnabled()
			&& !IsMoveInputIgnored() && GetViewTarget() == ControlledCharacter;
		ControlledCharacter->SetSprintRequested(bAllowed && Value.Get<bool>());
	}
}

void AMyPlayerController::HandleLightAttackInput(const FInputActionValue& Value)
{
	AMyCharacter* ControlledCharacter = Cast<AMyCharacter>(GetPawn());
	if (IsLocalPlayerController() && ControlledCharacter && ControlledCharacter->InputEnabled()
		&& !IsMoveInputIgnored() && GetViewTarget() == ControlledCharacter && Value.Get<bool>())
	{
		ControlledCharacter->RequestLightAttack();
	}
}

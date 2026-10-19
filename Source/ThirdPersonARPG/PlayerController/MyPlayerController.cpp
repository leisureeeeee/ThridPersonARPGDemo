#include "MyPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

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
	const FRotator YawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
	ControlledPawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), MovementInput.Y);
	ControlledPawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), MovementInput.X);
}

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EnhancedPlayerInput.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "MyCharacter/MyCharacter.h"
#include "PlayerController/MyPlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARPGPlayerInputTest, "ThirdPersonARPG.Input.CameraRelativeMoveAndMouseLook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext
	| EAutomationTestFlags::CommandletContext | EAutomationTestFlags::EngineFilter)

bool FARPGPlayerInputTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	World->InitializeActorsForPlay(FURL());
	AMyPlayerController* Controller = World->SpawnActor<AMyPlayerController>();
	AMyCharacter* Character = World->SpawnActor<AMyCharacter>();
	if (!TestNotNull(TEXT("Controller spawns"), Controller) || !TestNotNull(TEXT("Character spawns"), Character))
	{
		return false;
	}

	Controller->Possess(Character);
	Controller->InitInputSystem();
	UEnhancedPlayerInput* Input = Cast<UEnhancedPlayerInput>(Controller->PlayerInput);
	UInputMappingContext* Context = FindObject<UInputMappingContext>(Controller, TEXT("PlayerMappingContext"));
	UCameraComponent* Camera = Character->FindComponentByClass<UCameraComponent>();
	if (!TestNotNull(TEXT("Enhanced player input"), Input)
		|| !TestNotNull(TEXT("Runtime input context"), Context) || !TestNotNull(TEXT("Character camera"), Camera))
	{
		return false;
	}
	TestTrue(TEXT("Camera is active after component initialization"), Camera->IsActive());

	const TArray<UInputComponent*> InputStack = { Controller->InputComponent };
	constexpr float DeltaTime = 1.0f / 60.0f;
	auto InjectKey = [&](const FKey& Key, const FVector2D& RawValue)
	{
		const FEnhancedActionKeyMapping* Mapping = Context->GetMappings().FindByPredicate(
			[&](const FEnhancedActionKeyMapping& Candidate) { return Candidate.Key == Key; });
		if (!TestNotNull(*FString::Printf(TEXT("Mapping exists for %s"), *Key.ToString()), Mapping))
		{
			return;
		}
		// Exercise the production mapping's modifiers and the bound controller handler.
		Input->InjectInputForAction(Mapping->Action, FInputActionValue(RawValue), Mapping->Modifiers);
	};
	auto ProcessInput = [&] { Input->ProcessInputStack(InputStack, DeltaTime, false); };
	auto CheckMove = [&](const FKey& Key, const FVector& Expected)
	{
		Character->ConsumeMovementInputVector();
		InjectKey(Key, FVector2D(1.0f, 0.0f));
		ProcessInput();
		const FVector Actual = Character->ConsumeMovementInputVector();
		TestTrue(*FString::Printf(TEXT("%s follows camera axes: actual %s, expected %s"),
			*Key.ToString(), *Actual.ToString(), *Expected.ToString()), Actual.Equals(Expected, 0.001f));
		ProcessInput(); // Release the injected action before the next case.
	};

	Controller->SetControlRotation(FRotator::ZeroRotator);
	Camera->SetWorldRotation(FRotator(-35.0f, 90.0f, 0.0f));
	CheckMove(EKeys::W, FVector(0.0f, 1.0f, 0.0f));
	CheckMove(EKeys::S, FVector(0.0f, -1.0f, 0.0f));
	CheckMove(EKeys::D, FVector(-1.0f, 0.0f, 0.0f));
	CheckMove(EKeys::A, FVector(1.0f, 0.0f, 0.0f));

	Camera->SetWorldRotation(FRotator(60.0f, 180.0f, 0.0f));
	CheckMove(EKeys::W, FVector(-1.0f, 0.0f, 0.0f));
	InjectKey(EKeys::W, FVector2D(1.0f, 0.0f));
	InjectKey(EKeys::D, FVector2D(1.0f, 0.0f));
	ProcessInput();
	TestTrue(TEXT("Diagonal movement is normalized and stays horizontal"),
		Character->ConsumeMovementInputVector().Equals(FVector(-1.0f, -1.0f, 0.0f).GetSafeNormal(), 0.001f));
	ProcessInput();

	InjectKey(EKeys::W, FVector2D(1.0f, 0.0f));
	InjectKey(EKeys::S, FVector2D(1.0f, 0.0f));
	ProcessInput();
	TestTrue(TEXT("Opposite keys cancel"), Character->ConsumeMovementInputVector().IsNearlyZero());
	ProcessInput();

	Controller->RotationInput = FRotator::ZeroRotator;
	InjectKey(EKeys::Mouse2D, FVector2D(10.0f, -5.0f));
	ProcessInput();
	TestTrue(TEXT("Mouse right turns right without legacy yaw scaling"),
		FMath::IsNearlyEqual(Controller->RotationInput.Yaw, 1.5f));
	TestTrue(TEXT("Mouse Y reaches pitch without an extra inversion"),
		FMath::IsNearlyEqual(Controller->RotationInput.Pitch, -0.75f));
	ProcessInput();

	Controller->RotationInput = FRotator::ZeroRotator;
	Controller->SetIgnoreLookInput(true);
	InjectKey(EKeys::Mouse2D, FVector2D(10.0f, -5.0f));
	ProcessInput();
	TestTrue(TEXT("Ignored look input does not turn the camera"), Controller->RotationInput.IsNearlyZero());
	Controller->ResetIgnoreLookInput();
	ProcessInput();

	Controller->SetIgnoreMoveInput(true);
	CheckMove(EKeys::W, FVector::ZeroVector);
	Controller->ResetIgnoreMoveInput();
	return true;
}

#endif

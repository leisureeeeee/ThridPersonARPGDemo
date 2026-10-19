#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UCLASS()
class THIRDPERSONARPG_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void SetupInputComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void CreateInputMappingContext();
	void RegisterInputMappingContext();
	void RemoveInputMappingContext();
	void HandleMoveInput(const FInputActionValue& Value);

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> MappingSubsystem;
};

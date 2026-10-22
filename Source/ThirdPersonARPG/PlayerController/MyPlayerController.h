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

public:
	AMyPlayerController();
	virtual void AutoManageActiveCameraTarget(AActor* SuggestedTarget) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnUnPossess() override;
	virtual void SetupInputComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleSprintInput(const FInputActionValue& Value);
	void CreateInputMappingContext();
	void RegisterInputMappingContext();
	void RemoveInputMappingContext();
	void HandleMoveInput(const FInputActionValue& Value);
	void HandleLookInput(const FInputActionValue& Value);
	void HandleZoomInput(const FInputActionValue& Value);
	void HandleToggleWeaponInput(const FInputActionValue& Value);
	void HandleLightAttackInput(const FInputActionValue& Value);
	void HandleEvadeInput(const FInputActionValue& Value);
	FVector GetWorldMovementInput(const FVector2D& Input) const;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ZoomAction;

	// Degrees per mouse delta; mouse input is already a per-frame displacement.
	UPROPERTY(EditDefaultsOnly, Category = "Input|Look", meta = (ClampMin = "0.0"))
	FVector2D MouseSensitivity = FVector2D(0.15f, 0.15f);

	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> MappingSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ToggleWeaponAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LightAttackAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> EvadeAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SprintAction;
};

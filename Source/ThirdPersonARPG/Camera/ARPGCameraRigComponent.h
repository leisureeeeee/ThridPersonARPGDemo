#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ARPGCameraRigComponent.generated.h"

class UCameraComponent;
class USpringArmComponent;

USTRUCT(BlueprintType)
struct THIRDPERSONARPG_API FARPGCameraRigSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float ArmLength = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "5", ClampMax = "170"))
	float FieldOfView = 90.0f;

	// Offset at the camera end of the arm, in the arm's local space.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector SocketOffset = FVector::ZeroVector;

	// Offset at the start of the arm, in world space.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector TargetOffset = FVector::ZeroVector;
};

UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class THIRDPERSONARPG_API UARPGCameraRigComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGCameraRigComponent();

	// Bind and apply rig-owned follow behavior after the character's construction scripts.
	bool InitializeRig(USpringArmComponent* InSpringArm, UCameraComponent* InCamera);

	UFUNCTION(BlueprintPure, Category = "Camera")
	bool IsRigInitialized() const;

	UFUNCTION(BlueprintPure, Category = "Camera")
	FARPGCameraRigSettings GetCurrentCameraSettings() const;

	UFUNCTION(BlueprintCallable, Category = "Camera")
	bool SetCameraSettings(const FARPGCameraRigSettings& Settings, float BlendTime = 0.2f);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	bool ResetCameraSettings(float BlendTime = 0.2f);

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ConfigureCameraComponents();
	FARPGCameraRigSettings ReadCameraSettings() const;
	void ApplyCameraSettings(const FARPGCameraRigSettings& Settings);

	// Tune follow behavior on the rig; the spring arm performs the actual damping.
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Lag")
	bool bEnableCameraLag = true;

	// Higher values follow more tightly; zero disables position damping.
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Lag", meta = (ClampMin = "0", ClampMax = "1000", EditCondition = "bEnableCameraLag"))
	float CameraLagSpeed = 12.0f;

	// Maximum position lag in centimeters; zero removes the limit.
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Lag", meta = (ClampMin = "0", EditCondition = "bEnableCameraLag", Units = "cm"))
	float CameraLagMaxDistance = 40.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Lag")
	bool bEnableCameraRotationLag = true;

	// Keep rotation more responsive for camera-relative ARPG movement.
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Lag", meta = (ClampMin = "0", ClampMax = "1000", EditCondition = "bEnableCameraRotationLag"))
	float CameraRotationLagSpeed = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Lag", AdvancedDisplay)
	bool bUseCameraLagSubstepping = true;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Lag", AdvancedDisplay, meta = (ClampMin = "0.005", ClampMax = "0.5", EditCondition = "bUseCameraLagSubstepping", Units = "s"))
	float CameraLagMaxTimeStep = 1.0f / 120.0f;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> FollowCamera;

	FARPGCameraRigSettings DefaultSettings;
	FARPGCameraRigSettings BlendStartSettings;
	FARPGCameraRigSettings TargetSettings;
	float BlendDuration = 0.0f;
	float BlendElapsed = 0.0f;
};

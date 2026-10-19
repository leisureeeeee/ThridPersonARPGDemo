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

	// Bind after construction scripts have applied the character's camera defaults.
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
	FARPGCameraRigSettings ReadCameraSettings() const;
	void ApplyCameraSettings(const FARPGCameraRigSettings& Settings);

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

#include "ARPGCameraRigComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

UARPGCameraRigComponent::UARPGCameraRigComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

bool UARPGCameraRigComponent::InitializeRig(USpringArmComponent* InSpringArm, UCameraComponent* InCamera)
{
	if (!IsValid(InSpringArm) || !IsValid(InCamera)
		|| InSpringArm->GetOwner() != GetOwner() || InCamera->GetOwner() != GetOwner())
	{
		return false;
	}

	if (SpringArm == InSpringArm && FollowCamera == InCamera)
	{
		return true;
	}

	if (IsValid(SpringArm))
	{
		SpringArm->RemoveTickPrerequisiteComponent(this);
	}

	SpringArm = InSpringArm;
	FollowCamera = InCamera;
	ConfigureCameraComponents();
	DefaultSettings = ReadCameraSettings();
	TargetSettings = DefaultSettings;
	SetComponentTickEnabled(false);
	FollowCamera->Activate();

	// Apply rig parameters before the arm calculates camera position and collision.
	SpringArm->AddTickPrerequisiteComponent(this);
	return true;
}

void UARPGCameraRigComponent::ConfigureCameraComponents()
{
	// The arm follows control rotation; the camera inherits the arm's final pose.
	SpringArm->bUsePawnControlRotation = true;
	FollowCamera->bUsePawnControlRotation = false;

	// The rig owns follow policy; the engine arm owns damping and collision.
	SpringArm->bEnableCameraLag = bEnableCameraLag;
	SpringArm->CameraLagSpeed = CameraLagSpeed;
	SpringArm->CameraLagMaxDistance = CameraLagMaxDistance;
	SpringArm->bEnableCameraRotationLag = bEnableCameraRotationLag;
	SpringArm->CameraRotationLagSpeed = CameraRotationLagSpeed;
	SpringArm->bUseCameraLagSubstepping = bUseCameraLagSubstepping;
	SpringArm->CameraLagMaxTimeStep = CameraLagMaxTimeStep;
}

bool UARPGCameraRigComponent::IsRigInitialized() const
{
	return IsValid(SpringArm) && IsValid(FollowCamera);
}

FARPGCameraRigSettings UARPGCameraRigComponent::ReadCameraSettings() const
{
	FARPGCameraRigSettings Settings;
	Settings.ArmLength = SpringArm->TargetArmLength;
	Settings.FieldOfView = FollowCamera->FieldOfView;
	Settings.SocketOffset = SpringArm->SocketOffset;
	Settings.TargetOffset = SpringArm->TargetOffset;
	return Settings;
}

FARPGCameraRigSettings UARPGCameraRigComponent::GetCurrentCameraSettings() const
{
	return IsRigInitialized() ? ReadCameraSettings() : FARPGCameraRigSettings();
}

bool UARPGCameraRigComponent::SetCameraSettings(const FARPGCameraRigSettings& Settings, float BlendTime)
{
	if (!IsRigInitialized() || !FMath::IsFinite(BlendTime)
		|| !FMath::IsFinite(Settings.ArmLength) || !FMath::IsFinite(Settings.FieldOfView)
		|| Settings.SocketOffset.ContainsNaN() || Settings.TargetOffset.ContainsNaN())
	{
		return false;
	}

	BlendStartSettings = ReadCameraSettings();
	TargetSettings = Settings;
	TargetSettings.ArmLength = FMath::Max(0.0f, TargetSettings.ArmLength);
	TargetSettings.FieldOfView = FMath::Clamp(TargetSettings.FieldOfView, 5.0f, 170.0f);
	BlendDuration = FMath::Max(0.0f, BlendTime);
	BlendElapsed = 0.0f;

	if (BlendDuration <= UE_SMALL_NUMBER)
	{
		ApplyCameraSettings(TargetSettings);
		SetComponentTickEnabled(false);
	}
	else
	{
		SetComponentTickEnabled(true);
	}
	return true;
}

bool UARPGCameraRigComponent::ResetCameraSettings(float BlendTime)
{
	return SetCameraSettings(DefaultSettings, BlendTime);
}

bool UARPGCameraRigComponent::AdjustCameraDistance(float ZoomInput)
{
	if (!IsRigInitialized() || !FMath::IsFinite(ZoomInput) || FMath::IsNearlyZero(ZoomInput)
		|| !FMath::IsFinite(MinCameraDistance) || !FMath::IsFinite(MaxCameraDistance)
		|| !FMath::IsFinite(DistancePerScrollStep) || DistancePerScrollStep <= 0.0f
		|| !FMath::IsFinite(SpringArm->TargetArmLength))
	{
		return false;
	}

	const float MinDistance = FMath::Max(0.0f, MinCameraDistance);
	const float MaxDistance = FMath::Max(MinDistance, MaxCameraDistance);
	// Consume only this wheel delta at the currently applied distance. No queued zoom.
	const float NewDistance = FMath::Clamp(
		SpringArm->TargetArmLength - ZoomInput * DistancePerScrollStep, MinDistance, MaxDistance);

	SpringArm->TargetArmLength = NewDistance;
	// Wheel input takes over distance even at a limit. Prevent an existing rig blend
	// from pulling the arm again next tick; leave its FOV/offset timing untouched.
	BlendStartSettings.ArmLength = NewDistance;
	TargetSettings.ArmLength = NewDistance;
	return true;
}

void UARPGCameraRigComponent::ApplyCameraSettings(const FARPGCameraRigSettings& Settings)
{
	SpringArm->TargetArmLength = Settings.ArmLength;
	SpringArm->SocketOffset = Settings.SocketOffset;
	SpringArm->TargetOffset = Settings.TargetOffset;
	FollowCamera->SetFieldOfView(Settings.FieldOfView);
}

void UARPGCameraRigComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsRigInitialized() || BlendDuration <= UE_SMALL_NUMBER)
	{
		SetComponentTickEnabled(false);
		return;
	}

	BlendElapsed += FMath::Max(0.0f, DeltaTime);
	const float LinearAlpha = FMath::Clamp(BlendElapsed / BlendDuration, 0.0f, 1.0f);
	const float Alpha = FMath::SmoothStep(0.0f, 1.0f, LinearAlpha);
	FARPGCameraRigSettings Settings;
	Settings.ArmLength = FMath::Lerp(BlendStartSettings.ArmLength, TargetSettings.ArmLength, Alpha);
	Settings.FieldOfView = FMath::Lerp(BlendStartSettings.FieldOfView, TargetSettings.FieldOfView, Alpha);
	Settings.SocketOffset = FMath::Lerp(BlendStartSettings.SocketOffset, TargetSettings.SocketOffset, Alpha);
	Settings.TargetOffset = FMath::Lerp(BlendStartSettings.TargetOffset, TargetSettings.TargetOffset, Alpha);
	ApplyCameraSettings(Settings);

	if (LinearAlpha >= 1.0f)
	{
		SetComponentTickEnabled(false);
	}
}

void UARPGCameraRigComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetComponentTickEnabled(false);
	if (IsValid(SpringArm))
	{
		SpringArm->RemoveTickPrerequisiteComponent(this);
	}
	SpringArm = nullptr;
	FollowCamera = nullptr;
	Super::EndPlay(EndPlayReason);
}

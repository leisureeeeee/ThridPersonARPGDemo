// Fill out your copyright notice in the Description page of Project Settings.

#include "MyCharacter.h"
#include "AbilitySystem/MyCharacterAbilitySystemComponent.h"
#include "AbilitySystem/WeaponGameplayTags.h"
#include "PlayerState/ARPGPlayerState.h"
#include "Weapon/WeaponBase.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Engine/World.h"
#include "Camera/ARPGCameraRigComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Movement/ARPGCharacterMovementComponent.h"

AMyCharacter::AMyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UARPGCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	// The capsule origin is its center; this mesh's origin is at the feet.
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f,
		-GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));

	// The mannequin mesh faces +Y; CharacterMovement turns the capsule's +X.
	// Keep this asset alignment on the mesh so movement and camera axes stay intact.
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	CameraRig = CreateDefaultSubobject<UARPGCameraRigComponent>(TEXT("CameraRig"));

	AutoPossessPlayer = EAutoReceiveInput::Player0;
	bUseControllerRotationYaw = false;

	// Match the walk cycle's average travel speed (178.42 cm over 1.0667 s).
	GetCharacterMovement()->MaxWalkSpeed = 167.0f;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
}

void AMyCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ensure(CameraRig->InitializeRig(SpringArm, FollowCamera));
}

void AMyCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    InitializeAbilitySystem();
}

void AMyCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    InitializeAbilitySystem();
}

void AMyCharacter::OnRep_Controller()
{
    Super::OnRep_Controller();
    InitializeAbilitySystem();
}

void AMyCharacter::UnPossessed()
{
    ReleaseAbilitySystem();
    Super::UnPossessed();
}

UMyCharacterAbilitySystemComponent* AMyCharacter::GetWeaponASC() const
{
    UMyCharacterAbilitySystemComponent* ASC = Cast<UMyCharacterAbilitySystemComponent>(AbilitySystemComponent);
    return ASC && ASC->GetAvatarActor() == this ? ASC : nullptr;
}

void AMyCharacter::InitializeAbilitySystem()
{
    AARPGPlayerState* PS = GetPlayerState<AARPGPlayerState>();
    if (!PS)
    {
        ReleaseAbilitySystem();
        return;
    }
    if (AbilitySystemComponent != PS->GetPlayerASC())
    {
        ReleaseAbilitySystem();
        AbilitySystemComponent = PS->GetPlayerASC();
        AttributeSet = PS->GetPlayerAS();
    }
    UMyCharacterAbilitySystemComponent* ASC = PS->GetPlayerASC();
    const bool bAlreadyBound = ASC->GetAvatarActor() == this;
    ASC->BindWeaponAvatar(this);
    if (!bAlreadyBound)
    {
        SyncWeaponStateFromAttachment();
    }
}

void AMyCharacter::ReleaseAbilitySystem()
{
    StopWeaponMontages();
    if (UMyCharacterAbilitySystemComponent* ASC = GetWeaponASC())
    {
        ASC->ReleaseWeaponAvatar(this);
    }
    AbilitySystemComponent = nullptr;
    AttributeSet = nullptr;
}

void AMyCharacter::SyncWeaponStateFromAttachment()
{
    AWeaponBase* Weapon = GetCurrentWeapon();
    if (UMyCharacterAbilitySystemComponent* ASC = GetWeaponASC(); ASC && Weapon
        && Weapon->GetAttachParentActor() == this && Weapon->GetRootComponent())
    {
        ASC->SetWeaponDrawn(this, Weapon->GetRootComponent()->GetAttachSocketName() == DrawnWeaponSocket);
    }
}

bool AMyCharacter::IsWeaponDrawn() const
{
    const UMyCharacterAbilitySystemComponent* ASC = GetWeaponASC();
    return ASC && ASC->HasMatchingGameplayTag(WeaponGameplayTags::State_Weapon_Drawn);
}

bool AMyCharacter::IsDrawingWeapon() const
{
    const UMyCharacterAbilitySystemComponent* ASC = GetWeaponASC();
    return ASC && ASC->HasMatchingGameplayTag(WeaponGameplayTags::State_Action_Drawing);
}

bool AMyCharacter::IsSheathingWeapon() const
{
    const UMyCharacterAbilitySystemComponent* ASC = GetWeaponASC();
    return ASC && ASC->HasMatchingGameplayTag(WeaponGameplayTags::State_Action_Sheathing);
}

AWeaponBase* AMyCharacter::GetCurrentWeapon() const
{
	return IsValid(CurrentWeapon.Get()) ? CurrentWeapon.Get() : nullptr;
}

void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeAbilitySystem();
	// Defaults-only draw settings must follow the Blueprint template, including old placed actors.
	const AMyCharacter* Defaults = GetClass()->GetDefaultObject<AMyCharacter>();
	if (Defaults && Defaults != this)
	{
		DrawnWeaponSocket = Defaults->DrawnWeaponSocket;
		DrawnWeaponOffset = Defaults->DrawnWeaponOffset;
	}
	InitializeWeapon();
}

void AMyCharacter::InitializeWeapon()
{
	// Older placed actors can retain an empty value for newly introduced defaults.
	// This is defaults-only configuration; read the Blueprint template without editing it.
	if (!DefaultWeaponClass)
	{
		const AMyCharacter* Defaults = GetClass()->GetDefaultObject<AMyCharacter>();
		if (Defaults && Defaults != this)
		{
			DefaultWeaponClass = Defaults->DefaultWeaponClass;
		}
	}
	if (!IsValid(CurrentWeapon.Get()) && !DefaultWeaponClass)
	{
		return;
	}

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!IsValid(CharacterMesh) || HolsteredWeaponSocket.IsNone()
		|| !CharacterMesh->DoesSocketExist(HolsteredWeaponSocket))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Weapon socket '%s' is missing; weapon was not equipped."),
			*GetName(), *HolsteredWeaponSocket.ToString());
		return;
	}

	AWeaponBase* Weapon = GetCurrentWeapon();
	if (Weapon && ((Weapon->GetOwner() && Weapon->GetOwner() != this)
		|| (Weapon->GetAttachParentActor() && Weapon->GetAttachParentActor() != this)))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Configured weapon is already owned or attached to another actor."), *GetName());
		CurrentWeapon = nullptr;
		return;
	}

	if (!Weapon)
	{
		UWorld* World = GetWorld();
		if (!World || DefaultWeaponClass->HasAnyClassFlags(CLASS_Abstract))
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: Default weapon class cannot be spawned."), *GetName());
			return;
		}

		FActorSpawnParameters Parameters;
		Parameters.Owner = this;
		Parameters.Instigator = this;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Weapon = World->SpawnActor<AWeaponBase>(DefaultWeaponClass,
			CharacterMesh->GetSocketTransform(HolsteredWeaponSocket), Parameters);
		if (!IsValid(Weapon))
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: Failed to spawn the default weapon."), *GetName());
			return;
		}
		SpawnedWeapon = Weapon;
	}

	if (!Weapon->AttachToComponent(CharacterMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale, HolsteredWeaponSocket))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Failed to attach weapon to '%s'."),
			*GetName(), *HolsteredWeaponSocket.ToString());
		if (Weapon == SpawnedWeapon.Get())
		{
			Weapon->Destroy();
			SpawnedWeapon = nullptr;
		}
		CurrentWeapon = nullptr;
		return;
	}

	Weapon->SetActorRelativeTransform(HolsteredWeaponOffset);
	CurrentWeapon = Weapon;
	SyncWeaponStateFromAttachment();
}

void AMyCharacter::StopWeaponMontages()
{
	if (UAnimInstance* Animation = GetMesh()->GetAnimInstance(); Animation && ActiveDrawMontage)
	{
		FOnMontageEnded EmptyDelegate;
		Animation->Montage_SetEndDelegate(EmptyDelegate, ActiveDrawMontage);
		Animation->Montage_Stop(0.0f, ActiveDrawMontage);
	}
	if (UAnimInstance* Animation = GetMesh()->GetAnimInstance(); Animation && ActiveSheatheMontage)
	{
		FOnMontageEnded EmptyDelegate;
		Animation->Montage_SetEndDelegate(EmptyDelegate, ActiveSheatheMontage);
		Animation->Montage_Stop(0.0f, ActiveSheatheMontage);
	}
	ActiveSheatheMontage = nullptr;
	ActiveDrawMontage = nullptr;
}

void AMyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseAbilitySystem();
	if (IsValid(SpawnedWeapon.Get()))
	{
		SpawnedWeapon->Destroy();
	}
	// A level-placed weapon is borrowed, so release its attachment rather than destroy it.
	if (AWeaponBase* Weapon = GetCurrentWeapon(); Weapon && Weapon != SpawnedWeapon.Get()
		&& Weapon->GetAttachParentActor() == this)
	{
		Weapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	CurrentWeapon = nullptr;
	SpawnedWeapon = nullptr;
	Super::EndPlay(EndPlayReason);
}

bool AMyCharacter::RequestLightAttack()
{
	UMyCharacterAbilitySystemComponent* ASC = GetWeaponASC();
	return ASC && ASC->RequestLightAttack(this);
}

bool AMyCharacter::IsAttacking() const
{
	const UMyCharacterAbilitySystemComponent* ASC = GetWeaponASC();
	return ASC && ASC->HasMatchingGameplayTag(WeaponGameplayTags::State_Action_Attacking);
}

bool AMyCharacter::RequestEvade(const FVector& WorldDirection)
{
	UMyCharacterAbilitySystemComponent* ASC = GetWeaponASC();
	return ASC && ASC->RequestEvade(this, WorldDirection);
}

bool AMyCharacter::IsEvading() const
{
	const UMyCharacterAbilitySystemComponent* ASC = GetWeaponASC();
	return ASC && ASC->HasMatchingGameplayTag(WeaponGameplayTags::State_Action_Evading);
}

bool AMyCharacter::ToggleWeapon()
{
	// Draw/Sheathe already reject transitions while either montage is active.
	return IsWeaponDrawn() ? SheatheWeapon() : DrawWeapon();
}

bool AMyCharacter::DrawWeapon()
{
	UMyCharacterAbilitySystemComponent* ASC = GetWeaponASC();
	if (!ASC || !ASC->CanChangeWeaponState(this))
	{
		return false;
	}
	if (IsAttacking() || IsEvading() || IsDrawingWeapon() || IsSheathingWeapon() || IsWeaponDrawn() || !GetCurrentWeapon()
		|| !GetCharacterMovement()->IsMovingOnGround())
	{
		return false;
	}
	if (!DrawWeaponAnimation)
	{
		const AMyCharacter* Defaults = GetClass()->GetDefaultObject<AMyCharacter>();
		DrawWeaponAnimation = Defaults ? Defaults->DrawWeaponAnimation : nullptr;
	}
	UAnimInstance* Animation = GetMesh()->GetAnimInstance();
	if (!Animation || !DrawWeaponAnimation || DrawWeaponSlot.IsNone()
		|| !GetMesh()->DoesSocketExist(DrawnWeaponSocket) || Animation->IsAnyMontagePlaying())
	{
		return false;
	}
	const float PlayRate = FMath::IsFinite(DrawWeaponPlayRate) ? FMath::Clamp(DrawWeaponPlayRate, 0.1f, 3.0f) : 1.3f;
	ActiveDrawMontage = Animation->PlaySlotAnimationAsDynamicMontage(
		DrawWeaponAnimation, DrawWeaponSlot, 0.12f, 0.15f, PlayRate, 1);
	if (!ActiveDrawMontage)
	{
		return false;
	}
	if (!ASC->SetWeaponAction(this, WeaponGameplayTags::State_Action_Drawing))
	{
		Animation->Montage_Stop(0.0f, ActiveDrawMontage);
		ActiveDrawMontage = nullptr;
		return false;
	}
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AMyCharacter::HandleDrawMontageEnded);
	Animation->Montage_SetEndDelegate(EndDelegate, ActiveDrawMontage);
	return true;
}

void AMyCharacter::HandleWeaponGripNotify(UAnimSequenceBase* Animation)
{
	UMyCharacterAbilitySystemComponent* ASC = GetWeaponASC();
	if (!ASC || !ASC->CanChangeWeaponState(this))
	{
		return;
	}
	AWeaponBase* Weapon = GetCurrentWeapon();
	UAnimInstance* Instance = GetMesh()->GetAnimInstance();
	if (!IsDrawingWeapon() || IsWeaponDrawn() || !Weapon || Animation != DrawWeaponAnimation.Get()
		|| !Instance || !ActiveDrawMontage || !Instance->Montage_IsPlaying(ActiveDrawMontage))
	{
		return;
	}
	const FTransform PreviousRelative = Weapon->GetRootComponent()->GetRelativeTransform();
	const FName PreviousSocket = Weapon->GetRootComponent()->GetAttachSocketName();
	if (!Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, DrawnWeaponSocket))
	{
		Instance->Montage_Stop(0.1f, ActiveDrawMontage);
		return;
	}
	Weapon->SetActorRelativeTransform(DrawnWeaponOffset);
	if (!Weapon->ActivateWeapon())
	{
		Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, PreviousSocket);
		Weapon->SetActorRelativeTransform(PreviousRelative);
		Instance->Montage_Stop(0.1f, ActiveDrawMontage);
		return;
	}
	if (!ASC->SetWeaponDrawn(this, true))
	{
		Weapon->DeactivateWeapon();
		Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, PreviousSocket);
		Weapon->SetActorRelativeTransform(PreviousRelative);
		Instance->Montage_Stop(0.1f, ActiveDrawMontage);
	}
}

void AMyCharacter::HandleDrawMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveDrawMontage.Get())
	{
		return;
	}
	// Before the grip notify, an interruption keeps the weapon holstered.
	// After the grip notify, retain the committed hand attachment and drawn state.
	if (!bInterrupted && !IsWeaponDrawn())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Draw animation ended without a successful weapon grip notify."), *GetName());
	}
	if (UMyCharacterAbilitySystemComponent* ASC = GetWeaponASC())
	{
		ASC->SetWeaponAction(this, FGameplayTag());
	}
	ActiveDrawMontage = nullptr;
}

bool AMyCharacter::SheatheWeapon()
{
	UMyCharacterAbilitySystemComponent* ASC = GetWeaponASC();
	if (!ASC || !ASC->CanChangeWeaponState(this))
	{
		return false;
	}
	if (IsAttacking() || IsEvading() || IsDrawingWeapon() || IsSheathingWeapon() || !IsWeaponDrawn() || !GetCurrentWeapon()
		|| !GetCharacterMovement()->IsMovingOnGround())
	{
		return false;
	}
	if (!SheatheWeaponAnimation)
	{
		const AMyCharacter* Defaults = GetClass()->GetDefaultObject<AMyCharacter>();
		SheatheWeaponAnimation = Defaults ? Defaults->SheatheWeaponAnimation : nullptr;
	}
	UAnimInstance* Animation = GetMesh()->GetAnimInstance();
	if (!Animation || !SheatheWeaponAnimation || DrawWeaponSlot.IsNone()
		|| !GetMesh()->DoesSocketExist(HolsteredWeaponSocket) || Animation->IsAnyMontagePlaying())
	{
		return false;
	}
	const float PlayRate = FMath::IsFinite(SheatheWeaponPlayRate) ? FMath::Clamp(SheatheWeaponPlayRate, 0.1f, 3.0f) : 1.3f;
	ActiveSheatheMontage = Animation->PlaySlotAnimationAsDynamicMontage(
		SheatheWeaponAnimation, DrawWeaponSlot, 0.12f, 0.15f, PlayRate, 1);
	if (!ActiveSheatheMontage)
	{
		return false;
	}
	if (!ASC->SetWeaponAction(this, WeaponGameplayTags::State_Action_Sheathing))
	{
		Animation->Montage_Stop(0.0f, ActiveSheatheMontage);
		ActiveSheatheMontage = nullptr;
		return false;
	}
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AMyCharacter::HandleSheatheMontageEnded);
	Animation->Montage_SetEndDelegate(EndDelegate, ActiveSheatheMontage);
	return true;
}

void AMyCharacter::HandleWeaponSheathNotify(UAnimSequenceBase* Animation)
{
	UMyCharacterAbilitySystemComponent* ASC = GetWeaponASC();
	if (!ASC || !ASC->CanChangeWeaponState(this))
	{
		return;
	}
	AWeaponBase* Weapon = GetCurrentWeapon();
	UAnimInstance* Instance = GetMesh()->GetAnimInstance();
	if (!IsSheathingWeapon() || !IsWeaponDrawn() || !Weapon || Animation != SheatheWeaponAnimation.Get()
		|| !Instance || !ActiveSheatheMontage || !Instance->Montage_IsPlaying(ActiveSheatheMontage))
	{
		return;
	}
	const FTransform PreviousRelative = Weapon->GetRootComponent()->GetRelativeTransform();
	const FName PreviousSocket = Weapon->GetRootComponent()->GetAttachSocketName();
	if (!Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, HolsteredWeaponSocket))
	{
		Instance->Montage_Stop(0.1f, ActiveSheatheMontage);
		return;
	}
	Weapon->SetActorRelativeTransform(HolsteredWeaponOffset);
	if (!Weapon->DeactivateWeapon())
	{
		Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, PreviousSocket);
		Weapon->SetActorRelativeTransform(PreviousRelative);
		Instance->Montage_Stop(0.1f, ActiveSheatheMontage);
		return;
	}
	if (!ASC->SetWeaponDrawn(this, false))
	{
		Weapon->ActivateWeapon();
		Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, PreviousSocket);
		Weapon->SetActorRelativeTransform(PreviousRelative);
		Instance->Montage_Stop(0.1f, ActiveSheatheMontage);
	}
}

void AMyCharacter::HandleSheatheMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveSheatheMontage.Get())
	{
		return;
	}
	// Only the notify commits the equipment state. Interruptions preserve that result.
	if (!bInterrupted && IsWeaponDrawn())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Sheathe animation ended without a successful weapon sheath notify."), *GetName());
	}
	if (UMyCharacterAbilitySystemComponent* ASC = GetWeaponASC())
	{
		ASC->SetWeaponAction(this, FGameplayTag());
	}
	ActiveSheatheMontage = nullptr;
}

void AMyCharacter::SetSprintRequested(bool bRequested)
{
	if (UARPGCharacterMovementComponent* Movement = Cast<UARPGCharacterMovementComponent>(GetCharacterMovement()))
	{
		// Equipment montages only own the upper body; movement intent stays independent.
		Movement->SetSprintRequested(bRequested);
	}
}

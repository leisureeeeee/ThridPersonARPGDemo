#include "Combat/AttackTraceComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifyState_AttackWindow.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "MyCharacter/MyCharacter.h"
#include "Weapon/WeaponBase.h"

UAttackTraceComponent::UAttackTraceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UAttackTraceComponent::BeginPlay()
{
    Super::BeginPlay();
    if (const AMyCharacter* Character = Cast<AMyCharacter>(GetOwner()))
    {
        // Read the finished skeletal pose, including root motion and socket attachments.
        AddTickPrerequisiteComponent(Character->GetMesh());
    }
}

void UAttackTraceComponent::BeginAttack(UAnimMontage* Montage)
{
    ActiveMontage = Montage;
    Samples.Reset();
    Contacts.Reset();
    ContactPoints.Reset();
    Windows = 0;
    AttackTime = 0.f;
    AttackLength = Montage ? Montage->GetPlayLength() : 0.f;
    FrameRate = Montage ? static_cast<float>(Montage->GetSamplingFrameRate().AsDecimal()) : 60.f;
    FrameRate = FrameRate > 0.f ? FrameRate : 60.f;
    for (int32 Index = 0; Index != 3; ++Index)
    {
        WindowStarts[Index] = WindowEnds[Index] = -1.f;
    }
    if (Montage)
    {
        for (const FAnimNotifyEvent& Notify : Montage->Notifies)
        {
            if (const UAnimNotifyState_AttackWindow* State = Cast<UAnimNotifyState_AttackWindow>(Notify.NotifyStateClass))
            {
                const int32 Index = static_cast<int32>(State->GetWindow());
                WindowStarts[Index] = Notify.GetTriggerTime();
                WindowEnds[Index] = Notify.GetEndTriggerTime();
            }
        }
    }
    bBladeAvailable = false;
    BufferedEvadeExpiry = -1.;
    bAttackActive = Montage != nullptr;
    SetComponentTickEnabled(bAttackActive);
}

void UAttackTraceComponent::EndAttack(bool bCompleted)
{
    if (bCompleted) AttackTime = AttackLength;
    bAttackActive = false;
    ActiveMontage = nullptr;
    Windows = 0;
    BufferedEvadeExpiry = -1.;
    DisplayUntil = GetWorld()->GetTimeSeconds() + 2.;
    SetComponentTickEnabled(false);
}

bool UAttackTraceComponent::IsDisplayVisible() const
{
    return bAttackActive || GetWorld()->GetTimeSeconds() < DisplayUntil;
}

void UAttackTraceComponent::SetWindow(EAttackWindow Window, bool bOpen)
{
    const uint8 Mask = 1 << static_cast<uint8>(Window);
    Windows = bOpen ? Windows | Mask : Windows & ~Mask;
}

bool UAttackTraceComponent::IsWindowOpen(EAttackWindow Window) const
{
    return (Windows & (1 << static_cast<uint8>(Window))) != 0;
}

bool UAttackTraceComponent::BufferEvade(const FVector& WorldDirection)
{
    if (!bAttackActive || WorldDirection.ContainsNaN())
    {
        return false;
    }
    BufferedEvadeDirection = WorldDirection;
    BufferedEvadeExpiry = GetWorld()->GetTimeSeconds() + EvadeBufferSeconds;
    return true;
}

void UAttackTraceComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    AMyCharacter* Character = Cast<AMyCharacter>(GetOwner());
    UAnimInstance* Anim = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
    if (!bAttackActive || !Anim || !ActiveMontage)
    {
        return;
    }
    AttackTime = Anim->Montage_GetPosition(ActiveMontage);
    AWeaponBase* Weapon = Character->GetCurrentWeapon();
    FAttackTraceSample Current;
    Current.Time = AttackTime;
    Current.Windows = Windows;
    bBladeAvailable = Weapon && Weapon->GetBladeSegment(Current.Base, Current.Tip);
    if (bBladeAvailable)
    {
        if (!Samples.IsEmpty() && IsWindowOpen(EAttackWindow::Hit) && Character->HasAuthority())
        {
            TraceBlade(Samples.Last(), Current);
        }
        // Bounded history of this attack; HUD always projects world positions with the current camera.
        if (Samples.Num() == 512)
        {
            Samples.RemoveAt(0);
        }
        Samples.Add(Current);
    }

    if (Character->HasAuthority() && IsWindowOpen(EAttackWindow::EvadeCancel)
        && BufferedEvadeExpiry >= GetWorld()->GetTimeSeconds())
    {
        const FVector Direction = BufferedEvadeDirection;
        BufferedEvadeExpiry = -1.;
        if (Character->RequestEvade(Direction))
        {
            return; // This call may end this attack synchronously.
        }
    }
    if (Character->HasAuthority() && IsWindowOpen(EAttackWindow::MoveCancel)
        && !Character->GetLastMovementInputVector().IsNearlyZero())
    {
        OnMoveCancel.Broadcast();
    }
}

void UAttackTraceComponent::TraceBlade(const FAttackTraceSample& Previous, const FAttackTraceSample& Current)
{
    FCollisionQueryParams Query(SCENE_QUERY_STAT(AttackBlade), false, GetOwner());
    const AMyCharacter* Character = Cast<AMyCharacter>(GetOwner());
    Query.AddIgnoredActor(Character->GetCurrentWeapon());
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_Pawn);
    Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    // Sample along the entire blade, not only its tip. This is diagnostic contact detection, no damage.
    const int32 Steps = FMath::Clamp(FMath::CeilToInt(FVector::Distance(Current.Base, Current.Tip) / (TraceRadius * 2.f)), 1, 128);
    for (int32 Index = 0; Index <= Steps; ++Index)
    {
        const float Alpha = static_cast<float>(Index) / Steps;
        TArray<FHitResult> Hits;
        GetWorld()->SweepMultiByObjectType(Hits,
            FMath::Lerp(Previous.Base, Previous.Tip, Alpha),
            FMath::Lerp(Current.Base, Current.Tip, Alpha), FQuat::Identity, Objects,
            FCollisionShape::MakeSphere(TraceRadius), Query);
        for (const FHitResult& Hit : Hits)
        {
            AActor* Actor = Hit.GetActor();
            if (Actor && !Contacts.Contains(Actor))
            {
                Contacts.Add(Actor);
                ContactPoints.Add(Hit.ImpactPoint);
            }
        }
    }
}

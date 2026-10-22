#include "ARPGDebugHUD.h"
#include "Engine/Canvas.h"
#include "HAL/IConsoleManager.h"
#include "Combat/AttackTraceComponent.h"
#include "MyCharacter/MyCharacter.h"
#include "GameFramework/PlayerController.h"

#if !UE_BUILD_SHIPPING
namespace
{
	TAutoConsoleVariable<int32> CVarAttackTrace(TEXT("arpg.Debug.AttackTrace"), 1,
		TEXT("Draw attack frames, windows and the actual world-space blade trail. 0: off, 1: on."), ECVF_Cheat);

	FLinearColor SampleColor(uint8 Windows)
	{
		if (Windows & (1 << static_cast<uint8>(EAttackWindow::Hit))) return FLinearColor(1.f, .15f, .12f);
		if (Windows & (1 << static_cast<uint8>(EAttackWindow::MoveCancel))) return FLinearColor(.2f, 1.f, .35f);
		if (Windows & (1 << static_cast<uint8>(EAttackWindow::EvadeCancel))) return FLinearColor(.1f, .8f, 1.f);
		return FLinearColor(.7f, .7f, .7f, .65f);
	}

	TAutoConsoleVariable<int32> CVarScreenCenterDebug(
		TEXT("arpg.Debug.ScreenCenter"),
		1,
		TEXT("Draw the screen-center debug ball and green cross. 0: off, 1: on."),
		ECVF_Cheat);
}
#endif

void AARPGDebugHUD::DrawHUD()
{
	Super::DrawHUD();

#if !UE_BUILD_SHIPPING
	if (!bShowHUD || !Canvas || Canvas->SizeX <= 0 || Canvas->SizeY <= 0)
	{
		return;
	}
	DrawAttackTrace();
	if (CVarScreenCenterDebug.GetValueOnGameThread() == 0) return;

	// Use this player's current render area, so resizing keeps the cross centered.
	const FVector2D Center(Canvas->SizeX * 0.5f, Canvas->SizeY * 0.5f);
	const FLinearColor LineColor(0.0f, 1.0f, 0.0f, 0.65f);
	Canvas->K2_DrawLine(FVector2D(0.0f, Center.Y),
		FVector2D(Canvas->SizeX, Center.Y), 1.0f, LineColor);
	Canvas->K2_DrawLine(FVector2D(Center.X, 0.0f),
		FVector2D(Center.X, Canvas->SizeY), 1.0f, LineColor);

	// A small shaded screen marker; no world actor, collision, or texture asset.
	Canvas->K2_DrawPolygon(nullptr, Center, FVector2D(6.0f, 6.0f), 32,
		FLinearColor(0.12f, 0.06f, 0.0f, 1.0f));
	Canvas->K2_DrawPolygon(nullptr, Center, FVector2D(5.0f, 5.0f), 32,
		FLinearColor(1.0f, 0.35f, 0.0f, 1.0f));
	Canvas->K2_DrawPolygon(nullptr, Center - FVector2D(0.8f, 0.8f),
		FVector2D(3.5f, 3.5f), 24, FLinearColor(1.0f, 0.65f, 0.08f, 1.0f));
	Canvas->K2_DrawPolygon(nullptr, Center - FVector2D(1.7f, 1.7f),
		FVector2D(1.2f, 1.2f), 16, FLinearColor(1.0f, 0.95f, 0.75f, 1.0f));
#endif
}

void AARPGDebugHUD::DrawAttackTrace()
{
#if !UE_BUILD_SHIPPING
	if (CVarAttackTrace.GetValueOnGameThread() == 0 || !PlayerOwner) return;
	const AMyCharacter* Character = Cast<AMyCharacter>(PlayerOwner->GetPawn());
	const UAttackTraceComponent* Trace = Character ? Character->GetAttackTrace() : nullptr;
	if (!Trace) return;
	const float Width = FMath::Min(480.f, Canvas->SizeX - 32.f);
	const float X = 16.f, Y = 24.f;
	DrawRect(FLinearColor(0.015f, .025f, .04f, .86f), X, Y, Width, 148.f);
	const bool bActive = Trace->IsActiveAttack();
	const float FrameRate = Trace->GetFrameRate();
	const int32 Frame = FMath::FloorToInt(Trace->GetAttackTime() * FrameRate + .001f);
	const int32 Total = FMath::RoundToInt(Trace->GetAttackLength() * FrameRate);
	DrawText(FString::Printf(TEXT("LIGHT ATTACK   %s   Frame %d / %d (%.0f fps)"),
		bActive ? TEXT("ACTIVE") : TEXT("READY"), Frame, Total, FrameRate), FLinearColor::White, X + 12, Y + 8);
	DrawText(FString::Printf(TEXT("HIT %s   EVADE %s   MOVE %s"),
		Trace->IsWindowOpen(EAttackWindow::Hit) ? TEXT("OPEN") : TEXT("--"),
		Trace->IsWindowOpen(EAttackWindow::EvadeCancel) ? TEXT("OPEN") : TEXT("--"),
		Trace->IsWindowOpen(EAttackWindow::MoveCancel) ? TEXT("OPEN") : TEXT("--")),
		FLinearColor(.9f, .85f, .55f), X + 12, Y + 30);
	DrawText(FString::Printf(TEXT("Blade %s   Samples %d   Contacts %d (no damage)"),
		Trace->HasBladeSample() ? TEXT("OK") : TEXT("--"), Trace->GetSampleCount(), Trace->GetContactCount()),
		FLinearColor(.8f, .8f, .8f), X + 12, Y + 52);
	DrawText(TEXT("Alt: draw / sheath   LMB: attack   Space: evade"), FLinearColor(.7f, .75f, .8f), X + 12, Y + 74);
	const float Length = Trace->GetAttackLength();
	if (Length > 0.f)
	{
		const float BarX = X + 12, BarY = Y + 104, BarW = Width - 24;
		DrawRect(FLinearColor(.35f, .35f, .35f), BarX, BarY, BarW, 10.f);
		const EAttackWindow Types[] = {EAttackWindow::Hit, EAttackWindow::EvadeCancel, EAttackWindow::MoveCancel};
		const FLinearColor Colors[] = {FLinearColor(1.f, .15f, .12f), FLinearColor(.1f, .8f, 1.f), FLinearColor(.2f, 1.f, .35f)};
		for (int32 Index = 0; Index != 3; ++Index)
		{
			const float Start = Trace->GetWindowStart(Types[Index]);
			const float End = Trace->GetWindowEnd(Types[Index]);
			if (Start >= 0.f && End > Start)
			{
				DrawRect(Colors[Index], BarX + BarW * Start / Length, BarY, BarW * (End - Start) / Length, 10.f);
			}
		}
		const float Cursor = BarX + BarW * FMath::Clamp(Trace->GetAttackTime() / Length, 0.f, 1.f);
		DrawLine(Cursor, BarY - 3, Cursor, BarY + 13, FLinearColor::White, 2.f);
		DrawText(FString::Printf(TEXT("Hit %d-%d   Evade %d+   Move %d+"),
			FMath::RoundToInt(Trace->GetWindowStart(EAttackWindow::Hit) * FrameRate),
			FMath::RoundToInt(Trace->GetWindowEnd(EAttackWindow::Hit) * FrameRate) - 1,
			FMath::RoundToInt(Trace->GetWindowStart(EAttackWindow::EvadeCancel) * FrameRate),
			FMath::RoundToInt(Trace->GetWindowStart(EAttackWindow::MoveCancel) * FrameRate)),
			FLinearColor(.8f, .85f, .9f), X + 12, Y + 122);
	}
	if (!Trace->IsDisplayVisible()) return;
	const auto ProjectPoint = [this](const FVector& World, FVector2D& Screen)
	{
		return PlayerOwner->ProjectWorldLocationToScreen(World, Screen, true);
	};
	const auto Line = [this, &ProjectPoint](const FVector& A, const FVector& B, const FLinearColor& Color, float Thickness)
	{
		FVector2D ScreenA, ScreenB;
		if (ProjectPoint(A, ScreenA) && ProjectPoint(B, ScreenB))
		{
			Canvas->K2_DrawLine(ScreenA, ScreenB, Thickness, Color);
		}
	};
	const TArray<FAttackTraceSample>& Samples = Trace->GetSamples();
	for (int32 Index = 1; Index < Samples.Num(); ++Index)
	{
		const FAttackTraceSample& Prev = Samples[Index - 1];
		const FAttackTraceSample& Current = Samples[Index];
		const FLinearColor Color = SampleColor(Current.Windows);
		Line(Prev.Tip, Current.Tip, Color, 3.f);
		Line(Prev.Base, Current.Base, Color.CopyWithNewOpacity(.5f), 1.f);
		Line(Current.Base, Current.Tip, Color.CopyWithNewOpacity(.3f), 1.f);
	}
	if (!Samples.IsEmpty() && bActive)
	{
		Line(Samples.Last().Base, Samples.Last().Tip, FLinearColor::Yellow, 3.f);
	}
	for (const FVector& Point : Trace->GetContactPoints())
	{
		FVector2D Screen;
		if (ProjectPoint(Point, Screen))
		{
			Canvas->K2_DrawLine(Screen - FVector2D(5, 5), Screen + FVector2D(5, 5), 2.f, FLinearColor::Yellow);
			Canvas->K2_DrawLine(Screen + FVector2D(-5, 5), Screen + FVector2D(5, -5), 2.f, FLinearColor::Yellow);
		}
	}
#endif
}

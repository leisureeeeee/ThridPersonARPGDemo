#include "ARPGDebugHUD.h"
#include "Engine/Canvas.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING
namespace
{
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
	if (!bShowHUD || !Canvas || Canvas->SizeX <= 0 || Canvas->SizeY <= 0
		|| CVarScreenCenterDebug.GetValueOnGameThread() == 0)
	{
		return;
	}

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

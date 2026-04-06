#include "SpaceLeague/UI/SpaceLeagueHUD.h"

#include "Engine/Canvas.h"
#include "SpaceLeague/Characters/SpaceLeagueCharacterBase.h"

void ASpaceLeagueHUD::DrawHUD()
{
	Super::DrawHUD();

	APlayerController* PC = GetOwningPlayerController();
	if (!IsValid(PC) || !Canvas) return;

	ASpaceLeagueCharacterBase* Char = Cast<ASpaceLeagueCharacterBase>(PC->GetPawn());
	if (!IsValid(Char) || !Char->IsOrbitAimDataValid()) return;

	const TArray<FVector> WorldPoints = Char->GetOrbitAimSplineSamples(8);
	if (WorldPoints.Num() < 2) return;

	// Projeter les points en screen space
	TArray<FVector2D> RawScreen;
	RawScreen.Reserve(WorldPoints.Num());
	for (const FVector& WP : WorldPoints)
	{
		FVector2D SP;
		if (PC->ProjectWorldLocationToScreen(WP, SP, true))
		{
			RawScreen.Add(SP);
		}
	}
	if (RawScreen.Num() < 2) return;

	// --- Indicateur UI fixe en haut à droite ---
	const float BoxSize = 80.0f;
	const float BoxCX   = Canvas->SizeX - 90.0f; // centre X
	const float BoxCY   = 90.0f;                  // centre Y

	// Bounding box des points projetés pour normaliser
	FVector2D BBMin(FLT_MAX, FLT_MAX), BBMax(-FLT_MAX, -FLT_MAX);
	for (const FVector2D& P : RawScreen)
	{
		BBMin.X = FMath::Min(BBMin.X, P.X); BBMin.Y = FMath::Min(BBMin.Y, P.Y);
		BBMax.X = FMath::Max(BBMax.X, P.X); BBMax.Y = FMath::Max(BBMax.Y, P.Y);
	}
	const FVector2D BBRange = BBMax - BBMin;
	const float     BBScale = (BBRange.GetMax() > 1.0f) ? (BoxSize / BBRange.GetMax()) : 1.0f;

	// Convertir vers la boîte UI (centré)
	TArray<FVector2D> UIPoints;
	UIPoints.Reserve(RawScreen.Num());
	for (const FVector2D& P : RawScreen)
	{
		const FVector2D Norm = (P - BBMin) * BBScale;
		UIPoints.Add(FVector2D(BoxCX - BoxSize * 0.5f + Norm.X, BoxCY - BoxSize * 0.5f + Norm.Y));
	}

	const FLinearColor ArrowColor(0.2f, 0.85f, 1.0f, 1.0f);
	constexpr float    Thickness = 2.5f;

	// Tracer la courbe UI
	for (int32 i = 0; i < UIPoints.Num() - 1; ++i)
	{
		DrawLine(UIPoints[i].X, UIPoints[i].Y,
		         UIPoints[i + 1].X, UIPoints[i + 1].Y,
		         ArrowColor, Thickness);
	}

	// Pointe de flèche
	const FVector2D& EndPt  = UIPoints.Last();
	const FVector2D& PrevPt = UIPoints[UIPoints.Num() - 2];
	const FVector2D  Dir    = (EndPt - PrevPt).GetSafeNormal();
	const FVector2D  Perp(-Dir.Y, Dir.X);
	constexpr float  HeadSize = 10.0f;

	const FVector2D WingL = EndPt - Dir * HeadSize + Perp * HeadSize * 0.5f;
	const FVector2D WingR = EndPt - Dir * HeadSize - Perp * HeadSize * 0.5f;
	DrawLine(EndPt.X, EndPt.Y, WingL.X, WingL.Y, ArrowColor, Thickness);
	DrawLine(EndPt.X, EndPt.Y, WingR.X, WingR.Y, ArrowColor, Thickness);
}

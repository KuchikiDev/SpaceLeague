#include "ORA/Characters/ORASpeedStreaksComponent.h"

#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 SpeedStreakCount = 36;
	// Distance from the camera path: close enough to read the speed, never across the aim.
	constexpr float SpeedStreakMinRadius = 280.0f;
	constexpr float SpeedStreakMaxRadius = 950.0f;
	// Streaks fade in over this distance when they appear ahead, and out when they pass the camera.
	constexpr float SpeedStreakFarFadeDistance = 700.0f;
	constexpr float SpeedStreakBehindDistance = 400.0f;
}

UORASpeedStreaksComponent::UORASpeedStreaksComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetUsingAbsoluteLocation(true);
	SetUsingAbsoluteRotation(true);
	SetUsingAbsoluteScale(true);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	CastShadow = false;
	bReceivesDecals = false;
	bOnlyOwnerSee = true;
	bCanEverAffectNavigation = false;
	NumCustomDataFloats = 1;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> StreakMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> StreakMaterial(TEXT("/Game/VFX/SpeedStreaks/M_SpeedStreak.M_SpeedStreak"));
	if (StreakMesh.Succeeded())
	{
		SetStaticMesh(StreakMesh.Object);
	}
	if (StreakMaterial.Succeeded())
	{
		SetMaterial(0, StreakMaterial.Object);
	}
}

void UORASpeedStreaksComponent::UpdateStreaks(const FVector& ViewLocation, const FVector& Velocity, const float Alpha)
{
	const float Speed = static_cast<float>(Velocity.Size());
	if (Alpha <= 0.01f || Speed < 200.0f || GetStaticMesh() == nullptr)
	{
		if (bStreaksActive)
		{
			SetVisibility(false);
			bStreaksActive = false;
		}
		return;
	}

	const FVector Direction = Velocity / Speed;
	const float AheadDistance = FMath::Clamp(Speed * 0.55f, 1400.0f, 3600.0f);

	if (GetInstanceCount() != SpeedStreakCount)
	{
		ClearInstances();
		StreakLocations.SetNum(SpeedStreakCount);
		StreakSeeds.SetNum(SpeedStreakCount);
		Random.Initialize(static_cast<int32>(GetUniqueID()));
		for (int32 Index = 0; Index < SpeedStreakCount; ++Index)
		{
			AddInstance(FTransform::Identity, true);
		}
	}

	if (!bStreaksActive)
	{
		// Fill the whole path at once so the streaks do not arrive as a single wave.
		for (int32 Index = 0; Index < SpeedStreakCount; ++Index)
		{
			RespawnStreak(Index, ViewLocation, Direction, AheadDistance, true);
		}
		SetVisibility(true);
		bStreaksActive = true;
	}

	const FQuat Rotation = FRotationMatrix::MakeFromZ(Direction).ToQuat();
	const float BaseLength = FMath::Clamp(Speed * 0.07f, 200.0f, 750.0f);

	TArray<FTransform> Transforms;
	Transforms.Reserve(SpeedStreakCount);
	for (int32 Index = 0; Index < SpeedStreakCount; ++Index)
	{
		FVector Relative = StreakLocations[Index] - ViewLocation;
		float Along = static_cast<float>(Relative | Direction);
		float Radial = static_cast<float>((Relative - Along * Direction).Size());
		if (Along < -SpeedStreakBehindDistance || Along > AheadDistance * 1.25f
			|| Radial < SpeedStreakMinRadius * 0.6f || Radial > SpeedStreakMaxRadius * 1.4f)
		{
			RespawnStreak(Index, ViewLocation, Direction, AheadDistance, false);
			Relative = StreakLocations[Index] - ViewLocation;
			Along = static_cast<float>(Relative | Direction);
		}

		const float Seed = StreakSeeds[Index];
		const float Thickness = FMath::Lerp(0.9f, 1.8f, Seed);
		const float Length = BaseLength * FMath::Lerp(0.6f, 1.2f, Seed);
		// The engine cylinder is 100 cm wide and 100 cm tall along Z.
		Transforms.Add(FTransform(Rotation, StreakLocations[Index], FVector(Thickness, Thickness, Length) / 100.0f));

		const float FarFade = FMath::Clamp((AheadDistance - Along) / SpeedStreakFarFadeDistance, 0.0f, 1.0f);
		const float NearFade = FMath::Clamp((Along + SpeedStreakBehindDistance) / 500.0f, 0.0f, 1.0f);
		SetCustomDataValue(Index, 0, Alpha * FarFade * NearFade * FMath::Lerp(0.5f, 1.0f, Seed), false);
	}
	BatchUpdateInstancesTransforms(0, Transforms, true, true, true);
}

void UORASpeedStreaksComponent::RespawnStreak(
	const int32 Index,
	const FVector& ViewLocation,
	const FVector& Direction,
	const float AheadDistance,
	const bool bAnywhereAlongPath)
{
	FVector AxisA;
	FVector AxisB;
	Direction.FindBestAxisVectors(AxisA, AxisB);

	const float Angle = Random.FRandRange(0.0f, UE_TWO_PI);
	// Uniform over the ring area, not bunched on the inner edge.
	const float Radius = FMath::Sqrt(Random.FRandRange(
		SpeedStreakMinRadius * SpeedStreakMinRadius,
		SpeedStreakMaxRadius * SpeedStreakMaxRadius));
	const float Along = bAnywhereAlongPath
		? Random.FRandRange(-200.0f, AheadDistance)
		: Random.FRandRange(AheadDistance * 0.75f, AheadDistance);

	StreakLocations[Index] = ViewLocation
		+ Direction * Along
		+ (AxisA * FMath::Cos(Angle) + AxisB * FMath::Sin(Angle)) * Radius;
	StreakSeeds[Index] = Random.FRand();
}

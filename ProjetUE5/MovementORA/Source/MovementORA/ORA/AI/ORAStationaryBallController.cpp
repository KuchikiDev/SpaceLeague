#include "ORA/AI/ORAStationaryBallController.h"
#include "ORA/AI/ORABotTypes.h"

#include "Components/PrimitiveComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "ORA/Characters/ORACharacter.h"
#include "ORA/Interfaces/ORABallInterface.h"

AORAStationaryBallController::AORAStationaryBallController()
{
	PrimaryActorTick.bCanEverTick = true;
	bSetControlRotationFromPawnOrientation = false;
}

void AORAStationaryBallController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	StationaryCharacter = Cast<AORACharacter>(InPawn);
	FixedShotRotation = IsValid(InPawn) ? InPawn->GetActorRotation() : FRotator::ZeroRotator;
	FixedShotRotation.Pitch = 0.0f;
	FixedShotRotation.Roll = 0.0f;
	CapturedAtTime = -1.0f;
	CaptureAttemptElapsed = 0.0f;
	bBallWasInsideCaptureZone = false;
	TrackedBallPrimitive.Reset();
	PreviousBallLocation = FVector::ZeroVector;
	bHasPreviousBallLocation = false;
	SetControlRotation(FixedShotRotation);
}

void AORAStationaryBallController::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AORACharacter* TrainingCharacter = StationaryCharacter.Get();
	UWorld* World = GetWorld();
	if (!IsValid(TrainingCharacter) || !IsValid(World))
	{
		return;
	}

	StopMovement();
	TrainingCharacter->SetActorRotation(FixedShotRotation);
	SetControlRotation(FixedShotRotation);

	if (TrainingCharacter->IsOrbitBallActive())
	{
		if (CapturedAtTime < 0.0f)
		{
			CapturedAtTime = World->GetTimeSeconds();
			UE_LOG(LogORABot, Verbose, TEXT("[StationaryPlayer] %s stopped the ball; shooting in %.1f seconds."),
				*GetNameSafe(TrainingCharacter), HoldDuration);
		}

		if ((World->GetTimeSeconds() - CapturedAtTime) >= FMath::Max(0.1f, HoldDuration))
		{
			TrainingCharacter->ShootBall();
			if (!TrainingCharacter->IsOrbitBallActive())
			{
				UE_LOG(LogORABot, Verbose, TEXT("[StationaryPlayer] %s shot straight ahead."),
					*GetNameSafe(TrainingCharacter));
				CapturedAtTime = -1.0f;
			}
		}
		return;
	}

	CapturedAtTime = -1.0f;
	CaptureAttemptElapsed += DeltaSeconds;
	if (CaptureAttemptElapsed >= FMath::Max(0.02f, CaptureAttemptInterval))
	{
		CaptureAttemptElapsed = 0.0f;
		bool bBallInsideZoneThisCheck = false;
		TArray<AActor*> GameplayBalls;
		UGameplayStatics::GetAllActorsWithInterface(
			World, UORABallInterface::StaticClass(), GameplayBalls);
		for (AActor* GameplayBall : GameplayBalls)
		{
			if (!IsValid(GameplayBall))
			{
				continue;
			}

			TArray<UPrimitiveComponent*> BallPrimitives;
			GameplayBall->GetComponents<UPrimitiveComponent>(BallPrimitives);
			UPrimitiveComponent* PhysicalBallPrimitive = nullptr;
			UPrimitiveComponent* PhysicsFallbackPrimitive = nullptr;
			UPrimitiveComponent* CollisionFallbackPrimitive = nullptr;
			for (UPrimitiveComponent* BallPrimitive : BallPrimitives)
			{
				if (!IsValid(BallPrimitive))
				{
					continue;
				}

				// BP_Ball's real visible body is named Ballon. During a curved
				// shot it is moved kinematically, so IsSimulatingPhysics is false.
				if (BallPrimitive->GetFName() == TEXT("Ballon"))
				{
					PhysicalBallPrimitive = BallPrimitive;
					break;
				}
				if (!PhysicsFallbackPrimitive && BallPrimitive->IsSimulatingPhysics())
				{
					PhysicsFallbackPrimitive = BallPrimitive;
				}
				if (!CollisionFallbackPrimitive
					&& BallPrimitive->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
				{
					CollisionFallbackPrimitive = BallPrimitive;
				}
			}
			if (!PhysicalBallPrimitive)
			{
				PhysicalBallPrimitive = PhysicsFallbackPrimitive
					? PhysicsFallbackPrimitive
					: CollisionFallbackPrimitive;
			}
			if (!IsValid(PhysicalBallPrimitive))
			{
				continue;
			}

			const float SafeZoneRadius = FMath::Max(50.0f, CaptureZoneRadius);
			const FVector CurrentBallLocation = PhysicalBallPrimitive->GetComponentLocation();
			const FVector ReceiverLocation = TrainingCharacter->GetActorLocation();
			const bool bSameTrackedBall = TrackedBallPrimitive.Get() == PhysicalBallPrimitive;
			const FVector ClosestSweptPoint = bSameTrackedBall && bHasPreviousBallLocation
				? FMath::ClosestPointOnSegment(ReceiverLocation, PreviousBallLocation, CurrentBallLocation)
				: CurrentBallLocation;
			const bool bBallCrossedReceptionZone = FVector::DistSquared(
				ReceiverLocation, ClosestSweptPoint) <= FMath::Square(SafeZoneRadius);

			TrackedBallPrimitive = PhysicalBallPrimitive;
			PreviousBallLocation = CurrentBallLocation;
			bHasPreviousBallLocation = true;

			if (bBallCrossedReceptionZone)
			{
				bBallInsideZoneThisCheck = true;
				if (!bBallWasInsideCaptureZone)
				{
					UE_LOG(LogORABot, Verbose, TEXT("[StationaryPlayer] %s: physical ball entered %.0f-unit reception zone."),
						*GetNameSafe(TrainingCharacter), SafeZoneRadius);
				}
				if (TrainingCharacter->TryStopSpecificBall(GameplayBall, PhysicalBallPrimitive))
				{
					// The original shooter owns a kinematic spline while the shot is
					// curved. Relinquish it only after the receiver accepted the ball.
					for (TActorIterator<AORACharacter> CharacterIt(World); CharacterIt; ++CharacterIt)
					{
						CharacterIt->CancelSplineFollowForBall(GameplayBall);
					}
					bBallWasInsideCaptureZone = true;
					TrackedBallPrimitive.Reset();
					bHasPreviousBallLocation = false;
				}
				break;
			}
		}
		bBallWasInsideCaptureZone = bBallInsideZoneThisCheck;
	}
}

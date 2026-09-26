#include "ORA/AI/ORAGroundedBotController.h"

#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ORA/AI/ORAGroundedBotCharacter.h"
#include "ORA/Characters/ORACharacterBase.h"
#include "ORA/Core/ORAPlayerState.h"

AORAGroundedBotController::AORAGroundedBotController()
{
	PrimaryActorTick.bCanEverTick = true;
	bWantsPlayerState = false;
}

void AORAGroundedBotController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	BotCharacter = Cast<AORAGroundedBotCharacter>(InPawn);
	AimDuration = FMath::FRandRange(0.75f, 1.15f);

	if (AORAGroundedBotCharacter* BotPawn = BotCharacter.Get())
	{
		if (UCharacterMovementComponent* Movement = BotPawn->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
	}

	RefreshBall();
	RefreshGoal();
}

void AORAGroundedBotController::ConfigureBot(
	const EORABotTeam NewTeam,
	AActor* NewOpponentGoal,
	const FVector& NewCampCenter,
	const FVector& NewCampHalfExtent)
{
	(void)NewCampCenter;
	(void)NewCampHalfExtent;
	Team = NewTeam;
	OpponentGoal = NewOpponentGoal;
	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->Tags.AddUnique(Team == EORABotTeam::TeamA ? TEXT("TeamA") : TEXT("TeamB"));
	}
}

void AORAGroundedBotController::RefreshBall()
{
	TArray<AActor*> Balls;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("Ball"), Balls);
	BallActor = Balls.IsEmpty() ? nullptr : Balls[0];
}

void AORAGroundedBotController::RefreshGoal()
{
	if (OpponentGoal.IsValid())
	{
		return;
	}

	TArray<AActor*> Goals;
	UGameplayStatics::GetAllActorsWithTag(
		GetWorld(), Team == EORABotTeam::TeamA ? TEXT("GoalTeamB") : TEXT("GoalTeamA"), Goals);
	if (!Goals.IsEmpty())
	{
		OpponentGoal = Goals[0];
		return;
	}

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("TerrainGoal"), Goals);
	if (!Goals.IsEmpty())
	{
		OpponentGoal = Goals[0];
	}
}

bool AORAGroundedBotController::IsActorOnMyTeam(const AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return false;
	}
	if (const APawn* ActorPawn = Cast<APawn>(Actor))
	{
		if (const AORAPlayerState* ORAState = ActorPawn->GetPlayerState<AORAPlayerState>())
		{
			if (ORAState->Team != EORATeam::None)
			{
				return (Team == EORABotTeam::TeamA && ORAState->Team == EORATeam::TeamA)
					|| (Team == EORABotTeam::TeamB && ORAState->Team == EORATeam::TeamB);
			}
		}
	}
	if (Actor->ActorHasTag(TEXT("TeamB")))
	{
		return Team == EORABotTeam::TeamB;
	}
	if (Actor->ActorHasTag(TEXT("TeamA")))
	{
		return Team == EORABotTeam::TeamA;
	}
	return Team == EORABotTeam::TeamA;
}

bool AORAGroundedBotController::HasClearLane(
	const FVector& Start,
	const FVector& End,
	const AActor* AllowedTarget) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ORAStationaryBotLane), false, GetPawn());
	if (BallActor.IsValid())
	{
		Params.AddIgnoredActor(BallActor.Get());
	}
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return true;
	}
	return IsValid(AllowedTarget) && Hit.GetActor() == AllowedTarget;
}

void AORAGroundedBotController::ResetPossessionDecision()
{
	PossessionElapsed = 0.0f;
	bPossessionDecisionMade = false;
	bPossessionWantsPass = false;
	PlannedPassTarget = nullptr;
}

void AORAGroundedBotController::ChoosePossessionAction()
{
	bPossessionDecisionMade = true;
	bPossessionWantsPass = false;
	PlannedPassTarget = nullptr;

	AORAGroundedBotCharacter* BotPawn = BotCharacter.Get();
	if (!IsValid(BotPawn))
	{
		return;
	}

	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (TActorIterator<AORACharacterBase> It(GetWorld()); It; ++It)
	{
		AORACharacterBase* Candidate = *It;
		if (!IsValid(Candidate) || Candidate == BotPawn || !IsActorOnMyTeam(Candidate))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(
			BotPawn->GetActorLocation(), Candidate->GetActorLocation());
		if (DistanceSquared < FMath::Square(400.0f)
			|| DistanceSquared > FMath::Square(3600.0f)
			|| DistanceSquared >= BestDistanceSquared
			|| !HasClearLane(
				BotPawn->GetActorLocation() + FVector::UpVector * 80.0f,
				Candidate->GetActorLocation() + FVector::UpVector * 80.0f,
				Candidate))
		{
			continue;
		}

		BestDistanceSquared = DistanceSquared;
		PlannedPassTarget = Candidate;
	}

	if (!PlannedPassTarget.IsValid())
	{
		return;
	}

	const FVector GoalTarget = OpponentGoal.IsValid()
		? OpponentGoal->GetActorLocation() + FVector::UpVector * 80.0f
		: BotPawn->GetActorLocation() + BotPawn->GetActorForwardVector() * 5000.0f;
	const bool bShotLaneBlocked = !HasClearLane(
		BotPawn->GetActorLocation() + FVector::UpVector * 80.0f,
		GoalTarget,
		OpponentGoal.Get());

	// Prefer a pass when the direct shot is blocked; otherwise vary the action
	// so stationary bots do not repeat the same shot forever.
	bPossessionWantsPass = bShotLaneBlocked || FMath::FRand() < 0.48f;
}

void AORAGroundedBotController::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AORAGroundedBotCharacter* BotPawn = BotCharacter.Get();
	if (!IsValid(BotPawn))
	{
		return;
	}

	// This is intentionally enforced every frame: external animation, collision
	// or AI events must never reactivate locomotion for a stationary bot.
	StopMovement();
	if (UCharacterMovementComponent* Movement = BotPawn->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		if (Movement->MovementMode != MOVE_None)
		{
			Movement->DisableMovement();
		}
	}

	RefreshElapsed += DeltaSeconds;
	if (!BallActor.IsValid() || RefreshElapsed >= 0.5f)
	{
		RefreshElapsed = 0.0f;
		RefreshBall();
	}

	AActor* Ball = BallActor.Get();
	if (!IsValid(Ball))
	{
		ResetPossessionDecision();
		return;
	}

	if (!BotPawn->HasBall())
	{
		ResetPossessionDecision();
		if (FVector::DistSquared(BotPawn->GetActorLocation(), Ball->GetActorLocation())
			<= FMath::Square(AcceptanceRadius))
		{
			BotPawn->TryCaptureBall(Ball);
		}
		return;
	}

	PossessionElapsed += DeltaSeconds;
	RefreshGoal();
	if (!bPossessionDecisionMade)
	{
		ChoosePossessionAction();
	}

	FVector Target = OpponentGoal.IsValid()
		? OpponentGoal->GetActorLocation() + FVector::UpVector * 80.0f
		: BotPawn->GetActorLocation() + BotPawn->GetActorForwardVector() * 5000.0f;
	if (bPossessionWantsPass && PlannedPassTarget.IsValid())
	{
		Target = PlannedPassTarget->GetActorLocation() + FVector::UpVector * 65.0f;
	}

	FVector AimDirection = Target - BotPawn->GetActorLocation();
	AimDirection.Z = 0.0f;
	if (!AimDirection.IsNearlyZero())
	{
		const FRotator DesiredRotation(0.0f, AimDirection.Rotation().Yaw, 0.0f);
		BotPawn->SetActorRotation(FMath::RInterpTo(
			BotPawn->GetActorRotation(), DesiredRotation, DeltaSeconds, 8.0f));
		SetControlRotation(DesiredRotation);
	}

	const float FacingError = FMath::Abs(FMath::FindDeltaAngleDegrees(
		BotPawn->GetActorRotation().Yaw, AimDirection.Rotation().Yaw));
	if (PossessionElapsed < AimDuration || FacingError > 10.0f)
	{
		return;
	}

	UE_LOG(LogORABot, Verbose, TEXT("ORA Stationary Bot: %s %s toward %s."),
		*GetName(), bPossessionWantsPass ? TEXT("passes") : TEXT("shoots"), *Target.ToCompactString());
	BotPawn->ShootBallAt(Target, FMath::FRandRange(2500.0f, 3200.0f), bPossessionWantsPass);
	AimDuration = FMath::FRandRange(0.75f, 1.15f);
	ResetPossessionDecision();
}

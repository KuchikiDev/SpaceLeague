#include "ORA/Gameplay/ORAGameState.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameplayVariablesSettings.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "ORA/Characters/ORACharacter.h"

// Arena rotation: the server announces a rotation (alert), then every machine turns the arena around its
// center from the replicated server times. Level pieces are tagged "ArenaRotation" (terrain manager,
// walls, roof, prisons); the spawned goals carry "TerrainGoal". Attached actors (tiles, obstacles) follow.

static FAutoConsoleCommandWithWorldAndArgs GORARotateArenaCommand(
	TEXT("ORA.RotateArena"),
	TEXT("Announces an arena rotation now (server). Optional yaw: 90, -90, 180. Default: random."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		AORAGameState* GameState = World ? World->GetGameState<AORAGameState>() : nullptr;
		if (GameState && GameState->HasAuthority())
		{
			GameState->TriggerArenaRotation(Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.0f);
		}
	}));

void AORAGameState::TriggerArenaRotation(const float DeltaYaw)
{
	if (!HasAuthority() || GetWorld() == nullptr)
	{
		return;
	}

	// One rotation at a time.
	const float ServerNow = GetServerWorldTimeSeconds();
	if (ArenaRotation.Sequence > 0 && ServerNow < ArenaRotation.StartServerTime + ArenaRotation.DurationSeconds)
	{
		return;
	}

	float Delta = DeltaYaw;
	if (FMath::IsNearlyZero(Delta))
	{
		Delta = FMath::RandBool() ? 90.0f : 180.0f;
		if (FMath::RandBool())
		{
			Delta = -Delta;
		}
	}

	const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
	FORAArenaRotation NewRotation;
	NewRotation.Sequence = ArenaRotation.Sequence + 1;
	NewRotation.AlertServerTime = ServerNow;
	NewRotation.StartServerTime = ServerNow + FMath::Clamp(GameplayVariables->ArenaRotationWarningSeconds, 0.0f, 10.0f);
	NewRotation.DurationSeconds = FMath::Clamp(GameplayVariables->ArenaRotationDurationSeconds, 0.1f, 20.0f);
	NewRotation.FromYaw = ArenaRotation.Sequence > 0 ? ArenaRotation.FromYaw + ArenaRotation.DeltaYaw : 0.0f;
	NewRotation.DeltaYaw = Delta;
	// The center does not move when the arena turns around it: computed once.
	NewRotation.Pivot = ArenaRotation.Sequence > 0 ? ArenaRotation.Pivot : ComputeArenaPivot();
	ArenaRotation = NewRotation;
	ForceNetUpdate();

	UE_LOG(LogTemp, Display, TEXT("[Arena] Rotation %d announced: %.0f degrees in %.1f s, pivot %s."),
		ArenaRotation.Sequence, Delta, NewRotation.StartServerTime - ServerNow, *NewRotation.Pivot.ToCompactString());
}

FVector AORAGameState::ComputeArenaPivot() const
{
	// Center of the terrain (the manager with its tiles), else of every tagged arena piece.
	FBox TerrainBounds(ForceInit);
	FBox ArenaBounds(ForceInit);
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		const AActor* Actor = *It;
		if (!IsValid(Actor) || !Actor->ActorHasTag(TEXT("ArenaRotation")))
		{
			continue;
		}
		const FBox Bounds = Actor->GetComponentsBoundingBox(true, true);
		ArenaBounds += Bounds;
		if (Actor->GetClass()->GetName().Contains(TEXT("TerrainManager")))
		{
			TerrainBounds += Bounds;
		}
	}

	const FBox& Source = TerrainBounds.IsValid ? TerrainBounds : ArenaBounds;
	return Source.IsValid ? FVector(Source.GetCenter().X, Source.GetCenter().Y, 0.0f) : FVector::ZeroVector;
}

void AORAGameState::UpdateArenaRotation()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
	const float ServerNow = GetServerWorldTimeSeconds();

	// Server: random rotations during the match, the interval counted from the end of the previous one.
	if (HasAuthority())
	{
		const bool bPlaying = MatchPhase == EORAMatchPhase::InProgress || MatchPhase == EORAMatchPhase::Overtime;
		if (!bPlaying || !GameplayVariables->bArenaRotationEnabled)
		{
			NextArenaRotationServerTime = -1.0;
		}
		else
		{
			const float MinInterval = FMath::Max(5.0f, GameplayVariables->ArenaRotationMinIntervalSeconds);
			const float MaxInterval = FMath::Max(MinInterval, GameplayVariables->ArenaRotationMaxIntervalSeconds);
			const float RotationEnd = ArenaRotation.Sequence > 0
				? ArenaRotation.StartServerTime + ArenaRotation.DurationSeconds
				: ServerNow;
			if (NextArenaRotationServerTime < 0.0)
			{
				NextArenaRotationServerTime = FMath::Max(ServerNow, RotationEnd) + FMath::FRandRange(MinInterval, MaxInterval);
			}
			else if (ServerNow >= NextArenaRotationServerTime && ServerNow >= RotationEnd)
			{
				TriggerArenaRotation(0.0f);
				NextArenaRotationServerTime = -1.0;
			}
		}
	}

	if (ArenaRotation.Sequence <= 0)
	{
		return;
	}

	// Alert event on every machine (the alert sound will hook here).
	const float RotationEnd = ArenaRotation.StartServerTime + ArenaRotation.DurationSeconds;
	if (LocalArenaAlertSequence != ArenaRotation.Sequence && ServerNow >= ArenaRotation.AlertServerTime)
	{
		LocalArenaAlertSequence = ArenaRotation.Sequence;
		if (ServerNow < RotationEnd)
		{
			OnArenaRotationAlert.Broadcast(ArenaRotation.DeltaYaw, FMath::Max(0.0f, ArenaRotation.StartServerTime - ServerNow));
		}
	}

	// Smooth start and stop.
	const float Alpha = FMath::Clamp(
		(ServerNow - ArenaRotation.StartServerTime) / FMath::Max(0.01f, ArenaRotation.DurationSeconds), 0.0f, 1.0f);
	const float SmoothAlpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
	const float TargetYaw = ArenaRotation.FromYaw + ArenaRotation.DeltaYaw * SmoothAlpha;
	const float StepYaw = TargetYaw - LocalArenaYaw;
	// The last step can be tiny (the ease slows down): still applied so the rotation ends exactly.
	if (FMath::Abs(StepYaw) > KINDA_SMALL_NUMBER || (Alpha >= 1.0f && StepYaw != 0.0f))
	{
		ApplyArenaYawStep(StepYaw, ArenaRotation.Pivot);
		LocalArenaYaw = TargetYaw;
		if (Alpha >= 1.0f)
		{
			// The terrain and its walls must still line up (their bounds are checked in the tests).
			FBox TerrainBounds(ForceInit);
			FBox EnclosureBounds(ForceInit);
			for (TActorIterator<AActor> It(GetWorld()); It; ++It)
			{
				if (IsValid(*It) && It->ActorHasTag(TEXT("ArenaRotation")))
				{
					(It->GetClass()->GetName().Contains(TEXT("TerrainManager")) ? TerrainBounds : EnclosureBounds)
						+= It->GetComponentsBoundingBox(true, true);
				}
			}
			UE_LOG(LogTemp, Display, TEXT("[Arena] Rotation %d done: arena yaw %.0f, terrain %s +- %s, walls %s +- %s."),
				ArenaRotation.Sequence, LocalArenaYaw,
				*TerrainBounds.GetCenter().ToCompactString(), *TerrainBounds.GetExtent().ToCompactString(),
				*EnclosureBounds.GetCenter().ToCompactString(), *EnclosureBounds.GetExtent().ToCompactString());
		}
	}
}

void AORAGameState::ApplyArenaYawStep(const float StepYaw, const FVector& Pivot)
{
	UWorld* World = GetWorld();
	const FQuat Step(FVector::UpVector, FMath::DegreesToRadians(StepYaw));
	const auto RotatePoint = [&Step, &Pivot](const FVector& Point)
	{
		return Pivot + Step.RotateVector(Point - Pivot);
	};
	// Players and balls further than this from the center are not in the arena.
	constexpr float ArenaRadius = 13000.0f;
	const auto IsInArena = [&Pivot](const FVector& Point)
	{
		return FVector::DistSquared2D(Point, Pivot) <= FMath::Square(ArenaRadius);
	};

	// Arena pieces. Teleport: the terrain moves without pushing physics bodies, the balls and the
	// airborne players are turned below; players standing on a tile follow it (based movement).
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor)
			|| Actor->IsActorBeingDestroyed()
			|| Actor->GetAttachParentActor() != nullptr
			|| !(Actor->ActorHasTag(TEXT("ArenaRotation")) || Actor->ActorHasTag(TEXT("TerrainGoal"))))
		{
			continue;
		}
		// A replicated movement comes from the server; the local pieces turn on every machine.
		if (!Actor->HasAuthority() && Actor->IsReplicatingMovement())
		{
			continue;
		}
		Actor->SetActorLocationAndRotation(
			RotatePoint(Actor->GetActorLocation()), Step * Actor->GetActorQuat(), false, nullptr, ETeleportType::TeleportPhysics);
	}

	// Players in the air (or on a wall) turn with the arena, where their movement is simulated.
	for (TActorIterator<ACharacter> It(World); It; ++It)
	{
		ACharacter* Character = *It;
		UCharacterMovementComponent* Movement = IsValid(Character) ? Character->GetCharacterMovement() : nullptr;
		if (!IsValid(Movement)
			|| !(Character->HasAuthority() || Character->IsLocallyControlled())
			|| (Movement->IsMovingOnGround() && Character->GetMovementBaseObject() != nullptr)
			|| !IsInArena(Character->GetActorLocation()))
		{
			continue;
		}
		Character->SetActorLocationAndRotation(
			RotatePoint(Character->GetActorLocation()), Step * Character->GetActorQuat(), false, nullptr, ETeleportType::TeleportPhysics);
		Movement->Velocity = Step.RotateVector(Movement->Velocity);
		if (AController* Controller = Character->GetController(); IsValid(Controller) && Character->IsLocallyControlled())
		{
			FRotator ControlRotation = Controller->GetControlRotation();
			ControlRotation.Yaw += StepYaw;
			Controller->SetControlRotation(ControlRotation);
		}
	}

	if (!HasAuthority())
	{
		return;
	}

	// Free balls (server, their movement replicates). A held or curved-shot ball follows its player.
	for (const TWeakObjectPtr<AActor>& WeakBall : CachedContactBalls)
	{
		AActor* Ball = WeakBall.Get();
		if (!IsValid(Ball) || Ball->GetAttachParentActor() != nullptr || !IsInArena(Ball->GetActorLocation()))
		{
			continue;
		}
		bool bDriven = false;
		for (TActorIterator<AORACharacter> CharacterIt(World); CharacterIt && !bDriven; ++CharacterIt)
		{
			bDriven = CharacterIt->IsBallInOrbit(Ball) || CharacterIt->GetSplineFollowSpeedFor(Ball) > 0.0f;
		}
		if (bDriven)
		{
			continue;
		}

		// The physics body can be detached from the actor root: turn both.
		UPrimitiveComponent* BallBody = nullptr;
		TArray<UPrimitiveComponent*> Primitives;
		Ball->GetComponents<UPrimitiveComponent>(Primitives);
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			if (IsValid(Primitive) && (Primitive->GetFName() == TEXT("Ballon") || (!BallBody && Primitive->IsSimulatingPhysics())))
			{
				BallBody = Primitive;
			}
		}
		const FVector BodyLocation = IsValid(BallBody) ? BallBody->GetComponentLocation() : FVector::ZeroVector;
		Ball->SetActorLocation(RotatePoint(Ball->GetActorLocation()), false, nullptr, ETeleportType::TeleportPhysics);
		if (IsValid(BallBody))
		{
			BallBody->SetWorldLocation(RotatePoint(BodyLocation), false, nullptr, ETeleportType::TeleportPhysics);
			if (BallBody->IsSimulatingPhysics())
			{
				BallBody->SetPhysicsLinearVelocity(Step.RotateVector(BallBody->GetPhysicsLinearVelocity()));
			}
		}

		// The contact checks compare with the previous position: turn it too, or the jump would
		// count as the ball sweeping through players and goals.
		if (FVector* PreviousContact = PreviousBallContactLocations.Find(Ball))
		{
			*PreviousContact = RotatePoint(*PreviousContact);
		}
		if (FVector* PreviousGoal = PreviousBallGoalLocations.Find(Ball))
		{
			*PreviousGoal = RotatePoint(*PreviousGoal);
		}
	}

	// Prisoners come back to where they were hit, which turned with the arena.
	for (TPair<TWeakObjectPtr<APawn>, FTransform>& Entry : PrisonReturnTransforms)
	{
		if (IsInArena(Entry.Value.GetLocation()))
		{
			Entry.Value.SetLocation(RotatePoint(Entry.Value.GetLocation()));
			Entry.Value.SetRotation(Step * Entry.Value.GetRotation());
		}
	}
}

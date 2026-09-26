---
type: spec-technical
status: draft
last_review: 2026-02-19
owner: dylan
audience: humans-agents
---

# Spécifications Techniques - ORA

**Version:** 1.0
**Date:** 2026-01-25
**Auteur:** Claude (BMAD)

**Navigation Vault:** [[_ai/00_Home]] | [[_ai/01_Now]] | [[_ai/AGENT_CONTEXT]]
**Liens relies:** [[ARCHITECTURE]] | [[IMPACT_MIGRATION]] | [[WORKFLOW_PHASE0_BMAD]] | [[_maps/MOC_Architecture]]
**Note statut:** Spec cible `ORA`; verifier et synchroniser avec module reel `MovementParadoxe`.

---

## 1. Module C++ ORA

### 1.1 Configuration Build

**Fichier:** `Source/ORA/ORA.Build.cs`

```cpp
using UnrealBuildTool;

public class ORA : ModuleRules
{
    public ORA(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "UMG",
            "Slate",
            "SlateCore",
            "GameplayTags",
            "NetCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });
    }
}
```

### 1.2 Types Communs

**Fichier:** `Source/ORA/Data/ORATypes.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "ORATypes.generated.h"

// État du match
UENUM(BlueprintType)
enum class EMatchState : uint8
{
    WaitingForPlayers,
    Starting,
    InProgress,
    Paused,
    Ended
};

// État du joueur
UENUM(BlueprintType)
enum class EPlayerGameState : uint8
{
    InPlay,
    InPrison,
    Spectating
};

// Type d'événement terrain
UENUM(BlueprintType)
enum class ETerrainEventType : uint8
{
    None,
    Rotation90,
    Rotation180,
    Rotation360,
    AttackPhase,
    DefensePhase,
    ChaosPhase,
    GravityFlip
};

// Slot de capacité
UENUM(BlueprintType)
enum class EAbilitySlot : uint8
{
    Passive,
    Skill,
    Ultimate
};

// Type élémentaire
UENUM(BlueprintType)
enum class EElementType : uint8
{
    None,
    Foudre,
    Galaxie,
    Lumiere,
    Malediction,
    Observation,
    Origine,
    Polarite,
    Reflexion,
    Temps
};

// Équipe
UENUM(BlueprintType)
enum class ETeam : uint8
{
    None,
    TeamA,
    TeamB
};

// Structure pour les stats de personnage
USTRUCT(BlueprintType)
struct FCharacterStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MoveSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DashDistance = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DashCooldown = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float GrappleRange = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ThrowPower = 2000.0f;
};
```

---

## 2. AORAGameMode

**Fichier:** `Source/ORA/Core/ORAGameMode.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ORATypes.h"
#include "ORAGameMode.generated.h"

class AORACharacterBase;
class AORAGameState;

UCLASS()
class MOVEMENTPARADOXE_API AORAGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AORAGameMode();

    // Configuration match
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match")
    float MatchDuration = 300.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match")
    int32 PlayersPerTeam = 3;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scoring")
    int32 PointsForPrison = 2;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scoring")
    int32 PointsForGoal = 1;

    // Overrides
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

    // Fonctions gameplay
    UFUNCTION(BlueprintCallable, Category = "Match")
    void StartMatch();

    UFUNCTION(BlueprintCallable, Category = "Match")
    void EndMatch(ETeam WinningTeam);

    UFUNCTION(BlueprintCallable, Category = "Match")
    void OnPlayerEliminated(AORACharacterBase* EliminatedPlayer, AORACharacterBase* Eliminator);

    UFUNCTION(BlueprintCallable, Category = "Match")
    void OnGoalScored(ETeam ScoringTeam);

    UFUNCTION(BlueprintCallable, Category = "Match")
    void ReleaseFromPrison(AORACharacterBase* Player);

    // Équipes
    UFUNCTION(BlueprintCallable, Category = "Teams")
    ETeam AssignTeamToPlayer(AController* Player);

protected:
    UPROPERTY()
    TArray<AActor*> SpawnPointsTeamA;

    UPROPERTY()
    TArray<AActor*> SpawnPointsTeamB;

    int32 NextSpawnIndexA = 0;
    int32 NextSpawnIndexB = 0;

    void FindSpawnPoints();
    AActor* GetNextSpawnPoint(ETeam Team);
};
```

---

## 3. AORAGameState

**Fichier:** `Source/ORA/Core/ORAGameState.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ORATypes.h"
#include "ORAGameState.generated.h"

class AORAPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScoreChanged, ETeam, Team, int32, NewScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchStateChanged, EMatchState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTerrainEventStarted, ETerrainEventType, EventType);

UCLASS()
class MOVEMENTPARADOXE_API AORAGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AORAGameState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Scores
    UPROPERTY(ReplicatedUsing = OnRep_ScoreTeamA, BlueprintReadOnly, Category = "Score")
    int32 ScoreTeamA = 0;

    UPROPERTY(ReplicatedUsing = OnRep_ScoreTeamB, BlueprintReadOnly, Category = "Score")
    int32 ScoreTeamB = 0;

    // État
    UPROPERTY(ReplicatedUsing = OnRep_MatchState, BlueprintReadOnly, Category = "Match")
    EMatchState MatchState = EMatchState::WaitingForPlayers;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    float MatchTimeRemaining = 0.0f;

    // Prison
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Prison")
    TArray<AORAPlayerState*> PrisonedPlayersTeamA;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Prison")
    TArray<AORAPlayerState*> PrisonedPlayersTeamB;

    // Événement terrain
    UPROPERTY(ReplicatedUsing = OnRep_TerrainEvent, BlueprintReadOnly, Category = "Terrain")
    ETerrainEventType ActiveTerrainEvent = ETerrainEventType::None;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnScoreChanged OnScoreChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMatchStateChanged OnMatchStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnTerrainEventStarted OnTerrainEventStarted;

    // Fonctions
    UFUNCTION(BlueprintCallable, Category = "Score")
    void AddScore(ETeam Team, int32 Points);

    UFUNCTION(BlueprintCallable, Category = "Prison")
    void AddToPrison(AORAPlayerState* Player);

    UFUNCTION(BlueprintCallable, Category = "Prison")
    void RemoveFromPrison(AORAPlayerState* Player);

    UFUNCTION(BlueprintPure, Category = "Prison")
    int32 GetPrisonCount(ETeam Team) const;

    UFUNCTION(BlueprintCallable, Category = "Terrain")
    void TriggerTerrainEvent(ETerrainEventType EventType);

protected:
    UFUNCTION()
    void OnRep_ScoreTeamA();

    UFUNCTION()
    void OnRep_ScoreTeamB();

    UFUNCTION()
    void OnRep_MatchState();

    UFUNCTION()
    void OnRep_TerrainEvent();
};
```

---

## 4. AORACharacterBase

**Fichier:** `Source/ORA/Characters/ORACharacterBase.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ORATypes.h"
#include "ORACharacterBase.generated.h"

class UAbilityComponent;
class ABall;

UCLASS()
class MOVEMENTPARADOXE_API AORACharacterBase : public ACharacter
{
    GENERATED_BODY()

public:
    AORACharacterBase();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // Configuration
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Legend")
    UDataAsset* LegendData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Legend")
    EElementType ElementType = EElementType::None;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Team")
    ETeam Team = ETeam::None;

    UPROPERTY(ReplicatedUsing = OnRep_PlayerGameState, BlueprintReadOnly, Category = "State")
    EPlayerGameState PlayerGameState = EPlayerGameState::InPlay;

    // Stats
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
    FCharacterStats Stats;

    // Composants
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
    UAbilityComponent* AbilityComponent;

    // Balle
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ball")
    ABall* HeldBall;

    // Fonctions Blueprint exposées
    UFUNCTION(BlueprintCallable, Category = "Legend")
    void InitializeFromLegendData(UDataAsset* InLegendData);

    UFUNCTION(BlueprintCallable, Category = "Ball")
    void CatchBall(ABall* Ball);

    UFUNCTION(BlueprintCallable, Category = "Ball")
    void ThrowBall(FVector Direction, float Power, float Curve);

    UFUNCTION(BlueprintCallable, Category = "Ball")
    void PassBall(AORACharacterBase* Target);

    UFUNCTION(BlueprintCallable, Category = "Movement")
    void PerformDash(FVector Direction);

    UFUNCTION(BlueprintCallable, Category = "Movement")
    void StartGrapple(FVector TargetLocation);

    UFUNCTION(BlueprintCallable, Category = "State")
    void SendToPrison();

    UFUNCTION(BlueprintCallable, Category = "State")
    void ReleaseFromPrison();

    // Events
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnCaughtBall(ABall* Ball);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnThrewBall();

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnSentToPrison();

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnReleasedFromPrison();

protected:
    UFUNCTION()
    void OnRep_PlayerGameState();

    // Serveur
    UFUNCTION(Server, Reliable)
    void Server_ThrowBall(FVector Direction, float Power, float Curve);

    UFUNCTION(Server, Reliable)
    void Server_PerformDash(FVector Direction);
};
```

---

## 5. ABall

**Fichier:** `Source/ORA/Gameplay/Ball.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ORATypes.h"
#include "Ball.generated.h"

class AORACharacterBase;
class USphereComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBallHitPlayer, ABall*, Ball, AORACharacterBase*, HitPlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBallEnteredGoal, ABall*, Ball, ETeam, ScoringTeam);

UCLASS()
class MOVEMENTPARADOXE_API ABall : public AActor
{
    GENERATED_BODY()

public:
    ABall();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void Tick(float DeltaTime) override;

    // Composants
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* CollisionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    // État
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
    AORACharacterBase* CurrentHolder;

    UPROPERTY(ReplicatedUsing = OnRep_Velocity, BlueprintReadOnly, Category = "Physics")
    FVector ReplicatedVelocity;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
    ETeam LastTouchedByTeam = ETeam::None;

    // Configuration
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Physics")
    float MaxSpeed = 3000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Physics")
    float Bounciness = 0.8f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Physics")
    float AirResistance = 0.1f;

    // Timer inactivité
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay")
    float InactivityTimeBeforeAutoShoot = 10.0f;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnBallHitPlayer OnBallHitPlayer;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnBallEnteredGoal OnBallEnteredGoal;

    // Fonctions
    UFUNCTION(BlueprintCallable, Category = "Ball")
    void Launch(FVector Direction, float Power, float CurveIntensity);

    UFUNCTION(BlueprintCallable, Category = "Ball")
    void AttachToPlayer(AORACharacterBase* Player);

    UFUNCTION(BlueprintCallable, Category = "Ball")
    void Detach();

    UFUNCTION(BlueprintCallable, Category = "Ball")
    void ResetToCenter();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnRep_Velocity();

    UFUNCTION()
    void OnCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                        UPrimitiveComponent* OtherComp, FVector NormalImpulse,
                        const FHitResult& Hit);

    float TimeSinceLastTouch = 0.0f;
    FVector CurrentCurve = FVector::ZeroVector;

    void ApplyCurve(float DeltaTime);
    void CheckInactivity(float DeltaTime);
};
```

---

## 6. Configuration Projet

### 6.1 DefaultEngine.ini

```ini
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Game/Levels/L_MainArena
EditorStartupMap=/Game/Levels/L_MainArena
GlobalDefaultGameMode=/Script/MovementParadoxe.AORAGameMode
GameInstanceClass=/Game/Game/Core/GI_ORA.GI_ORA_C

[/Script/Engine.Engine]
+ActiveGameNameRedirects=(OldGameName="TP_BlankBP",NewGameName="/Script/MovementParadoxe")
```

### 6.2 DefaultGame.ini

```ini
[/Script/EngineSettings.GeneralProjectSettings]
ProjectID=ORA
ProjectName=ORA
CompanyName=Byakuya
```

---

## 7. Résumé des Classes à Créer

| Classe | Fichiers | Priorité |
|--------|----------|----------|
| ORATypes | Data/ORATypes.h | Phase 0 |
| AORAGameMode | Core/ORAGameMode.h/cpp | Phase 0 |
| AORAGameState | Core/ORAGameState.h/cpp | Phase 0 |
| AORAPlayerController | Core/ORAPlayerController.h/cpp | Phase 0 |
| AORAPlayerState | Core/ORAPlayerState.h/cpp | Phase 0 |
| AORACharacterBase | Characters/ORACharacterBase.h/cpp | Phase 0 |
| UAbilityComponent | Characters/Components/AbilityComponent.h/cpp | Phase 3 |
| ABall | Gameplay/Ball.h/cpp | Phase 0 |
| AGoal | Gameplay/Goal.h/cpp | Phase 1 |
| APrison | Gameplay/Prison.h/cpp | Phase 1 |
| ATerrainEventManager | Gameplay/TerrainEventManager.h/cpp | Phase 1 |


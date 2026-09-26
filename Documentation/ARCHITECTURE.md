---
type: architecture
status: draft
last_review: 2026-02-19
owner: dylan
audience: humans-agents
---

# Architecture Technique - ORA

**Version:** 1.0
**Date:** 2026-01-25
**Auteur:** Claude (BMAD)

**Navigation Vault:** [[_ai/00_Home]] | [[_ai/01_Now]] | [[_ai/AGENT_CONTEXT]]
**Liens relies:** [[SPECS_TECHNIQUES]] | [[IMPACT_MIGRATION]] | [[ROADMAP]] | [[_maps/MOC_Architecture]]
**Note statut:** Cible architecture a resynchroniser avec le code `MovementParadoxe`.

---

## Vue d'Ensemble

ORA utilise une **architecture hybride C++ / Blueprint** suivant les best practices Epic Games.

```
┌─────────────────────────────────────────────────────────────────┐
│                        ARCHITECTURE                              │
├─────────────────────────────────────────────────────────────────┤
│  C++ (Performance, Réplication, Maintenabilité)                 │
│  ├── AORAGameMode                                       │
│  ├── AORAGameState                                      │
│  ├── AORAPlayerController                               │
│  ├── AORAPlayerState                                    │
│  ├── AORACharacterBase                                  │
│  ├── UAbilitySystemComponent                                    │
│  ├── ABall                                                       │
│  ├── AGoal                                                       │
│  ├── APrison                                                     │
│  └── ATerrainEventManager                                       │
├─────────────────────────────────────────────────────────────────┤
│  Blueprint (Variations, Tuning, Polish)                         │
│  ├── BP_Raijin (hérite AORACharacterBase)              │
│  ├── BP_Keplar                                                  │
│  ├── WBP_HUD                                                    │
│  ├── WBP_Scoreboard                                             │
│  └── Animations, VFX, SFX                                       │
└─────────────────────────────────────────────────────────────────┘
```

---

## Structure des Dossiers UE5

### Structure Cible (Post-Migration)

```
/Content/
├── Game/
│   ├── Core/
│   │   └── GI_ORA.uasset          # GameInstance Blueprint
│   ├── Data/
│   │   ├── Abilities/
│   │   │   ├── DA_Ability_Raijin_Passive.uasset
│   │   │   ├── DA_Ability_Raijin_Skill.uasset
│   │   │   └── DA_Ability_Raijin_Ultimate.uasset
│   │   ├── Legends/
│   │   │   ├── DA_LegendBase_Raijin.uasset
│   │   │   └── DA_LegendBase_Keplar.uasset
│   │   ├── Skins/
│   │   │   └── DA_Skin_Raijin_Default.uasset
│   │   ├── Registry/
│   │   │   └── DA_LegendRegistry_Main.uasset
│   │   ├── Balancing/
│   │   │   ├── DA_TypeChart.uasset
│   │   │   └── DA_ElementTheme.uasset
│   │   ├── Structs/                        # Supprimé après migration C++
│   │   └── Enums/                          # Supprimé après migration C++
│   ├── Gameplay/
│   │   ├── Gamemodes/
│   │   │   └── BP_GameMode_ORA.uasset  # Hérite C++
│   │   ├── Components/
│   │   └── Player/
│   ├── UI/
│   │   ├── HUD/
│   │   ├── Menu/
│   │   └── Shared/
│   └── Materials/
├── Legends/
│   ├── BP_Paradoxe.uasset                  # Classe de base (hérite C++)
│   └── Blueprints/
│       ├── BP_ParadoxeRaijin.uasset
│       └── BP_ParadoxeKeplar.uasset
├── Terrain/
│   └── Blueprints/
│       ├── BP_Ball.uasset                  # Hérite ABall C++
│       ├── BP_Goal.uasset                  # Hérite AGoal C++
│       ├── BP_Prison.uasset                # Hérite APrison C++
│       ├── BP_Obstacle.uasset
│       └── BP_SpawnZone.uasset
├── Levels/
│   └── L_MainArena.umap
└── Maps/
    └── BPPC_Paradoxe.uasset               # Hérite C++

/Source/
└── ORA/
    ├── ORA.Build.cs
    ├── ORA.h
    ├── ORA.cpp
    ├── Core/
    │   ├── ORAGameMode.h
    │   ├── ORAGameMode.cpp
    │   ├── ORAGameState.h
    │   ├── ORAGameState.cpp
    │   ├── ORAPlayerController.h
    │   ├── ORAPlayerController.cpp
    │   ├── ORAPlayerState.h
    │   └── ORAPlayerState.cpp
    ├── Characters/
    │   ├── ORACharacterBase.h
    │   ├── ORACharacterBase.cpp
    │   └── Components/
    │       ├── AbilityComponent.h
    │       └── AbilityComponent.cpp
    ├── Gameplay/
    │   ├── Ball.h
    │   ├── Ball.cpp
    │   ├── Goal.h
    │   ├── Goal.cpp
    │   ├── Prison.h
    │   ├── Prison.cpp
    │   └── TerrainEventManager.h
    │   └── TerrainEventManager.cpp
    └── Data/
        ├── ORATypes.h              # Enums, Structs
        ├── LegendData.h
        └── AbilityData.h
```

---

## Classes C++ - Responsabilités

### AORAGameMode

**Fichiers:** `ORAGameMode.h/cpp`

**Responsabilités:**
- Gestion du spawn des joueurs
- Règles de la partie (temps, conditions victoire)
- Gestion des phases de jeu
- Attribution des équipes

**Variables Clés:**
```cpp
UPROPERTY(EditDefaultsOnly, Category = "Match")
float MatchDuration = 300.0f;  // 5 minutes

UPROPERTY(EditDefaultsOnly, Category = "Teams")
int32 PlayersPerTeam = 3;

UPROPERTY(EditDefaultsOnly, Category = "Scoring")
int32 PointsForPrison = 2;

UPROPERTY(EditDefaultsOnly, Category = "Scoring")
int32 PointsForGoal = 1;
```

**Fonctions Clés:**
```cpp
virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
void StartMatch();
void EndMatch();
void OnPlayerEliminated(AORACharacterBase* Player);
```

---

### AORAGameState

**Fichiers:** `ORAGameState.h/cpp`

**Responsabilités:**
- Score des équipes (répliqué)
- État du match (répliqué)
- Liste des joueurs en prison
- Événements terrain actifs

**Variables Clés:**
```cpp
UPROPERTY(ReplicatedUsing = OnRep_TeamScores)
TArray<int32> TeamScores;

UPROPERTY(Replicated)
EMatchState MatchState;

UPROPERTY(Replicated)
TArray<AORAPlayerState*> PrisonedPlayers;

UPROPERTY(ReplicatedUsing = OnRep_ActiveTerrainEvent)
ETerrainEventType ActiveTerrainEvent;
```

---

### AORACharacterBase

**Fichiers:** `ORACharacterBase.h/cpp`

**Responsabilités:**
- Mouvements de base (hérités, étendus)
- Interaction avec la balle
- Gestion des capacités
- État du joueur (en jeu, en prison)

**Variables Clés:**
```cpp
UPROPERTY(EditDefaultsOnly, Category = "Legend")
UDataAsset* LegendData;

UPROPERTY(Replicated)
EPlayerGameState PlayerGameState;  // InPlay, InPrison

UPROPERTY(VisibleAnywhere, Category = "Abilities")
UAbilityComponent* AbilityComponent;

UPROPERTY(EditDefaultsOnly, Category = "Movement")
float DashDistance = 500.0f;

UPROPERTY(EditDefaultsOnly, Category = "Movement")
float GrappleRange = 1500.0f;
```

---

### ABall

**Fichiers:** `Ball.h/cpp`

**Responsabilités:**
- Physique de la balle (répliquée)
- Détection des collisions
- Possession par joueur
- Trajectoire (tir, passe, enroulé)

**Variables Clés:**
```cpp
UPROPERTY(Replicated)
AORACharacterBase* CurrentHolder;

UPROPERTY(ReplicatedUsing = OnRep_Velocity)
FVector ReplicatedVelocity;

UPROPERTY(EditDefaultsOnly, Category = "Physics")
float MaxSpeed = 3000.0f;

UPROPERTY(EditDefaultsOnly, Category = "Physics")
float CurveIntensity = 0.5f;
```

---

## Flux de Données

### Sélection de Personnage → Spawn

```
1. Menu Sélection
   │
   ▼
2. GI_ORA::SetSelectedLegend(LegendId, Skin)
   │  Stocke: SelectedLegendId, SelectedSkin
   │
   ▼
3. Transition vers Map de jeu
   │
   ▼
4. AORAGameMode::SpawnDefaultPawnFor()
   │  Récupère: GI_ORA::GetSelectedLegendData()
   │  Spawn: CharacterClass depuis LegendData
   │
   ▼
5. AORACharacterBase::InitializeFromLegendData()
   │  Configure: Stats, Abilities, Skin
   │
   ▼
6. Joueur prêt
```

### Scoring - Prison

```
1. ABall::OnHit(Player)
   │
   ▼
2. AORAGameMode::OnPlayerEliminated(Player)
   │  Player.PlayerGameState = InPrison
   │  GameState.AddToPrison(Player)
   │
   ▼
3. AORAGameState::CheckPrisonScore()
   │  Si 2 joueurs même équipe en prison:
   │    TeamScores[AutreEquipe] += 2
   │    LibérerPrisonniers()
   │
   ▼
4. Multicast: OnScoreChanged
```

---

## Réplication Réseau

### Priorités de Réplication

| Classe | Priorité | Fréquence |
|--------|----------|-----------|
| ABall | Très Haute | 60 Hz |
| AORACharacterBase | Haute | 30 Hz |
| AORAGameState | Moyenne | Variable |
| AGoal | Basse | On Change |

### Variables Répliquées Critiques

```cpp
// GameState
UPROPERTY(Replicated)
TArray<int32> TeamScores;

// Character
UPROPERTY(Replicated)
EPlayerGameState PlayerGameState;

// Ball
UPROPERTY(ReplicatedUsing = OnRep_Velocity)
FVector ReplicatedVelocity;

UPROPERTY(Replicated)
AORACharacterBase* CurrentHolder;
```

---

## Conventions de Nommage

### Fichiers

| Type | Préfixe | Exemple |
|------|---------|---------|
| Blueprint Actor | BP_ | BP_Ball |
| Blueprint Widget | WBP_ | WBP_HUD |
| DataAsset | DA_ | DA_LegendBase_Raijin |
| Struct | ST_ | ST_AbilityData (Blueprint) |
| Enum | E | EMatchState (C++) |
| Interface | I | IInteractable |
| Component | UC | UAbilityComponent |
| GameMode | A...GameMode | AORAGameMode |

### Variables

| Type | Convention | Exemple |
|------|------------|---------|
| Bool | bNomVerbe | bIsInPrison |
| Pointeur | Nom | CurrentHolder |
| Array | NomPluriel | TeamScores |
| Event/Delegate | OnNom | OnScoreChanged |

---

## Dépendances

### Modules UE5 Requis

```cpp
// ORA.Build.cs
PublicDependencyModuleNames.AddRange(new string[] {
    "Core",
    "CoreUObject",
    "Engine",
    "InputCore",
    "EnhancedInput",
    "UMG",
    "Slate",
    "SlateCore",
    "GameplayAbilities",  // Optionnel, si GAS utilisé
    "GameplayTags",
    "NetCore",
    "OnlineSubsystem",
    "OnlineSubsystemUtils"
});
```

### Plugins Recommandés

| Plugin | Usage |
|--------|-------|
| Enhanced Input | Système d'input moderne |
| Common UI | UI cross-platform |
| Online Subsystem Steam | Multijoueur Steam (futur) |

---

## Notes de Migration

Voir [[IMPACT_MIGRATION]] pour le détail des impacts et le plan de migration sécurisé.


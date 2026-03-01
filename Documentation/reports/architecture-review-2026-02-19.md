---
type: architecture-review
status: active
last_review: 2026-02-19
owner: codex
audience: humans-agents
---

# Architecture Review - 2026-02-19

## Objective

Evaluer l'ecart entre architecture cible (`SpaceLeague`) et etat reel (`MovementParadoxe`), puis trancher la strategie.

## Decision

Decision recommandee: **consolider le projet `MovementParadoxe` comme conteneur, et introduire `SpaceLeague` comme module gameplay incrementale**, sans renommage global du projet en Phase 0.

Pourquoi:

1. Le projet reel est encore ancre sur `MovementParadoxe` (uproject/config/build).
2. Des erreurs Blueprint/Data bloquantes existent deja et doivent etre traitees avant une migration large.
3. Une migration incrementale par module limite le risque tout en convergeant vers la cible `SpaceLeague`.

## Evidence (docs vs code)

### 1. Identite projet / module

- UProject actuel:
  - `EngineAssociation` 5.5: `ProjetUE5/MovementParadoxe/MovementParadoxe.uproject:3`
  - Module unique `MovementParadoxe`: `ProjetUE5/MovementParadoxe/MovementParadoxe.uproject:8`
- Specs cible:
  - module `SpaceLeague`: `Documentation/SPECS_TECHNIQUES.md:21`

### 2. Cible config non alignee

- Config reelle:
  - `GameName=MovementParadoxe`: `ProjetUE5/MovementParadoxe/Config/DefaultEngine.ini:113`
  - `GlobalDefaultGameMode=/Game/Levels/HUB/BP_GM_Hub.BP_GM_Hub_C`: `ProjetUE5/MovementParadoxe/Config/DefaultEngine.ini:13`
  - `GameDefaultMap=/Game/Levels/FloatingIsland/FloatingIslands.FloatingIslands`: `ProjetUE5/MovementParadoxe/Config/DefaultEngine.ini:11`
- Specs techniques:
  - `GlobalDefaultGameMode=/Script/SpaceLeague.ASpaceLeagueGameMode`: `Documentation/SPECS_TECHNIQUES.md:547`
  - `GameDefaultMap=/Game/Levels/L_MainArena`: `Documentation/SPECS_TECHNIQUES.md:545`

### 3. Etat assets gameplay

- `BP_GM_Hub` existe: `ProjetUE5/MovementParadoxe/Content/Levels/HUB/BP_GM_Hub.uasset`
- `BP_GameState_SpaceLeague` existe: `ProjetUE5/MovementParadoxe/Content/Game/Gameplay/Gamemodes/BP_GameState_SpaceLeague.uasset`
- `BP_GameMode_SpaceLeague` non detecte dans Content (absence dans l'inventaire des assets critiques).
- Doublon `GI_SpaceLeague`:
  - `ProjetUE5/MovementParadoxe/Content/Instance/GI_SpaceLeague.uasset`
  - `ProjetUE5/MovementParadoxe/Content/Game/Core/GI_SpaceLeague.uasset`

### 4. Build et toolchain

- Build C++ passe en local avec `-NoXGE` (phase health-check).
- Echec XGE d'infrastructure (licence/coordinator), pas un echec code.

### 5. Integrite Blueprint/Data (blocant prioritaire)

Dans `ProjetUE5/MovementParadoxe/Saved/Logs/MovementParadoxe.log`:

- Erreurs structure `ST_SkinList` et types invalides dans `DA_LegendRegistry`: `:1262`, `:1263`, `:1265`, `:1266`
- Struct manquante `ST_AbilityPresentationOverride`: `:1339`, `:1340`, `:1341`
- Pins casses + variable manquante dans `BP_ParadoxeRaijin`/`BP_ParadoxeKeplar`: `:1268` a `:1277`

## Architecture risk assessment

- **High risk**: migrer massivement C++/BP avant de corriger les erreurs Data/Blueprint.
- **Medium risk**: renommer le projet/module global en Phase 0.
- **Low risk**: ajouter un nouveau module runtime `SpaceLeague` en parallele, puis migrer progressivement.

## Recommended plan (5 steps max)

1. **Stabiliser la couche Data/Blueprint**
   - Corriger `ST_AbilityPresentationOverride`, `ST_SkinList`, pins casses Raijin/Keplar.
2. **Nettoyer les incoherences assets/config**
   - Choisir un seul `GI_SpaceLeague`, traiter LEAG-001/002, clarifier `Content/Game/Old`.
3. **Creer le module runtime `SpaceLeague` (sans renommer le projet)**
   - Garder `MovementParadoxe` comme module legacy/contenant.
4. **Migrer les classes coeur en priorite**
   - `GameMode`, `GameState`, `CharacterBase`, puis `Ball` avec checkpoints build/tests.
5. **Raccorder config runtime**
   - Basculer progressivement map/mode vers la cible validee apres smoke tests.

## GO / NO-GO

- GO pour une migration incremental module `SpaceLeague`.
- NO-GO pour un renommage global immediate du projet avant stabilisation Data/Blueprint.

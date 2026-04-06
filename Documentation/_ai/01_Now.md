---
type: now
status: active
last_review: 2026-03-12
owner: dylan
audience: humans-agents
source_of_truth: code-and-config
---

# 01 Now

Etat courant du projet au 2026-03-12.

## Resume executif

- Le projet Unreal actif s'appelle `MovementParadoxe`.
- Le module C++ principal reste `MovementParadoxe`, avec un sous-domaine `SpaceLeague` actif dans `Source/MovementParadoxe/SpaceLeague`.
- Le socle gameplay de `BP_Paradoxe` migre progressivement vers `ASpaceLeagueCharacterBase`.
- Les briques actuellement stabilisees cote C++ couvrent:
  - ability/cooldown
  - dash/stamina
  - jump/wall jump
  - stop ball/orbit
  - aim spline orbit
  - look input
- Le comportement runtime final depend encore du wiring Blueprint restant:
  - events ability par slot
  - VFX/SFX/camera du dash
  - timers HUD
  - inputs `IA_OrbitAimCurve`

## Point de reprise (session)

- `BP_Paradoxe` doit maintenant etre traite comme une couche de presentation au-dessus de `ASpaceLeagueCharacterBase`.
- La spline `OrbitAim` est geree en C++:
  - depart capsule joueur
  - fin line trace de vue
  - point milieu d'arrondi
  - clip sur obstacle
- Le flux `IA_OrbitAimCurve` cible est:
  - `Triggered -> AccumulateOrbitAimCurveInput`
  - pas de reset sur `Completed` si on veut conserver l'arrondi atteint
- Pour repartir proprement:
  - `Ctrl+Alt+F11` dans UE si Live Coding est actif
  - sinon fermer UE pour un build complet
  - reouvrir UE, `Refresh All Nodes`, compiler `BP_Paradoxe`, `BPPC_Paradoxe`, `UW_HUD`

## Faits verifies (code/config)

### Identite projet

- `ProjetUE5/MovementParadoxe/Config/DefaultEngine.ini`
  - `GameName=MovementParadoxe`
  - `GlobalDefaultGameMode=/Game/Levels/HUB/BP_GM_Hub.BP_GM_Hub_C`
  - `GameInstanceClass=/Game/Instance/GI_SpaceLeague.GI_SpaceLeague_C`

### C++

- Sources presentes:
  - `ProjetUE5/MovementParadoxe/Source/MovementParadoxe.Target.cs`
  - `ProjetUE5/MovementParadoxe/Source/MovementParadoxeEditor.Target.cs`
  - `ProjetUE5/MovementParadoxe/Source/MovementParadoxe/MovementParadoxe.Build.cs`
  - `ProjetUE5/MovementParadoxe/Source/MovementParadoxe/MovementParadoxe.cpp`
  - `ProjetUE5/MovementParadoxe/Source/MovementParadoxe/MovementParadoxe.h`
  - `ProjetUE5/MovementParadoxe/Source/MovementParadoxe/SpaceLeague/` (domaine gameplay actif)

### Faits verifies recents (ability + HUD)

- `ASpaceLeagueCharacterBase`:
  - `TryUseAbility` valide cooldown + data et lance le cooldown seulement en cas de succes.
  - Routeur par slot disponible: `ExecutePassiveAbility`, `ExecuteSkillAbility`, `ExecuteUltimateAbility`.
  - Raison d'echec exposee: `ESpaceLeagueAbilityUseFailReason` + `GetLastAbilityUseFailReason`.
  - Dash expose via `TryStartDash`, `StopDash`, `CanDash`, `OnDashStarted`, `OnDashEnded`, `OnDashStaminaChanged`.
  - Jump expose via `TryHandleJumpInput`, `SetCanWallJump`, `ExecuteWallJump`.
  - Stop Ball expose via `TryStopBall`, `ReleaseOrbitBall`, `OnStopBallCaptured`, `OnStopBallReleased`.
  - `OrbitAim` expose via:
    - `SetOrbitAimCurveInput`
    - `AddOrbitAimCurveInput`
    - `AccumulateOrbitAimCurveInput`
    - `ResetOrbitAimCurveInput`
    - `SetOrbitAimVisible`
  - Fichiers:  
    - `ProjetUE5/MovementParadoxe/Source/MovementParadoxe/SpaceLeague/Characters/SpaceLeagueCharacterBase.h`  
    - `ProjetUE5/MovementParadoxe/Source/MovementParadoxe/SpaceLeague/Characters/SpaceLeagueCharacterBase.cpp`

- `ASpaceLeaguePlayerController`:
  - `OnPossess` appelle `EnsureInGameWidget()` sur `ASpaceLeagueCharacterBase`.
  - Creation/reutilisation propre du widget in-game local.
  - Fichier: `ProjetUE5/MovementParadoxe/Source/MovementParadoxe/SpaceLeague/Core/SpaceLeaguePlayerController.cpp`

- Build:
  - Le build CLI est bloque si Live Coding est actif (`Unable to build while Live Coding is active`).
  - Necessite fermeture editor pour validation compile fiable apres changement UCLASS/UFUNCTION/UPROPERTY.

### Assets importants detectes

- `ProjetUE5/MovementParadoxe/Content/Instance/GI_SpaceLeague.uasset`
- `ProjetUE5/MovementParadoxe/Content/Legends/BP_Paradoxe.uasset`
- `ProjetUE5/MovementParadoxe/Content/Terrain/Blueprints/BP_Ball.uasset`
- `ProjetUE5/MovementParadoxe/Content/Game/Gameplay/Gamemodes/BP_GameState_SpaceLeague.uasset`
- `ProjetUE5/MovementParadoxe/Content/Game/Data/Legends/DA_LegendBase_Raijin.uasset`
- `ProjetUE5/MovementParadoxe/Content/Game/Data/Legends/DA_LegendBase_Keplar.uasset`
- `ProjetUE5/MovementParadoxe/Content/Game/Core/GI_SpaceLeague.uasset` absent (LEAG-004 structurellement resolu)

## Ecarts docs vs reel

1. Les docs haut niveau parlent encore de migration "a venir", alors que le dossier `Source/MovementParadoxe/SpaceLeague` est deja actif.
2. Le flux Blueprint cooldown HUD n'est pas stabilise (timer/event wiring a finaliser pour skill + ultimate).
3. `BP_Paradoxe` peut encore contenir des anciens graphes spline/stop ball/dash qui doublonnent le C++.
4. Plusieurs checklists roadmap/migration non materialisees comme terminees.
5. `RESUME_JEU.md` contient un marqueur de template non rendu sur la date de generation.

## Priorites recommandees (prochaines sessions)

1. Stabiliser le wiring Blueprint ability:
   - `ExecuteSkillAbility` et `ExecuteUltimateAbility` doivent retourner `true` en succes.
   - `OnAbilityCooldownStarted` doit router vers `UW_HUD` (par slot).
2. Nettoyer `BP_Paradoxe`:
   - retirer les anciens graphes spline manuels
   - retirer les timers anti spam stop ball en BP si encore presents
   - ne garder cote BP que FX/SFX/camera/wiring
3. Stabiliser timers HUD:
   - timer looping avec `Time=0.05`
   - event `Update...Cooldown` (pas `On...Finished`)
   - handle stocke et nettoye.
4. Revalider `OrbitAim` en runtime:
   - courbe basse
   - obstacle clipping
   - largeur/epaisseur/material override
5. Fermer UE pour build complet apres modifs C++ reflectees, puis `Refresh All Nodes` dans les BPs.
6. Finaliser LEAG-003 en migrant les referents encore lies a `Content/Game/Old`.
7. Passer `ROADMAP.md` en suivi d'execution reel (done/in-progress/blocked date).

## Liens de travail

- [[_ai/00_Home]]
- [[_ai/AGENT_CONTEXT]]
- [[WORKFLOW_PHASE0_BMAD]]
- [[ROADMAP]]
- [[ARCHITECTURE]]
- [[SPECS_TECHNIQUES]]
- [[IMPACT_MIGRATION]]

## Journal de mise a jour

- 2026-02-19: Creation du snapshot initial "Now" avec verification code/config.
- 2026-02-19: Health check execute. Rapport: `Documentation/reports/health-check-2026-02-19.md`.
- 2026-02-19: Build C++ OK en local avec `-NoXGE`; erreurs Blueprint/Data detectees dans `MovementParadoxe.log`.
- 2026-02-19: Revue architecte executee. Decision: migration incrementale via module `SpaceLeague` sans renommage global immediat.
- 2026-02-19: Risk profile Phase 0 execute. Rapport: `Documentation/reports/risk-profile-phase0-2026-02-19.md`.
- 2026-02-19: Test design Phase 0 execute. Matrice: `docs/qa/assessments/0.0-test-design-20260219.md`.
- 2026-02-21: Cleanup LEAG-001/002 execute: `BP_ParadoxeJhin` et `BP_ParadoxeSenna` archives dans `.old/phase0-cleanup-20260221-015522`.
- 2026-02-21: LEAG-003 execute partiellement: `ST_CharacterUI` archive; autres assets `Game/Old` encore references.
- 2026-02-21: LEAG-004 tente en headless (consolidation + rollback), statut final `blocked` avec snapshots de securite dans `.old/`.
- 2026-02-21: Blockers LEAG-004 observes apres tentatives: pins casses dans `BP_Paradoxe`, `BPPC_Paradoxe`, `UW_HUD`, `GI_Proxy`.
- 2026-02-21: Rapport de tri ajoute: `Documentation/reports/phase0-cleanup-2026-02-21.md`.
- 2026-02-22: Reparation utilisateur en UE5: `Core GI` supprime et references nettoyees (`GI_Proxy`, `BPPC_Paradoxe`, `UW_HUD`, `BP_Paradoxe`, `GM_Sandbox`). LEAG-004 structurellement resolu.
- 2026-02-22: LEAG-004 resolu structurellement en UE Editor (cleanup references vers `Core GI`).
- 2026-02-24: `GetSelectedLegendData` indique comme reimplante par l'utilisateur; validation runtime restante uniquement.
- 2026-03-08: Integration C++ ability/hud en cours sur `SpaceLeagueCharacterBase` + `SpaceLeaguePlayerController` (routeur par slot, fail reason, cache widget local).
- 2026-03-08: Retour utilisateur: images HUD revenues, mais cooldown/ultimate encore instables cote Blueprint (timer/event wiring a finaliser).
- 2026-03-08: Rapport de session ajoute: `Documentation/reports/ability-hud-session-2026-03-08.md`.
- 2026-03-12: Refonte `BP_Paradoxe` poursuivie autour de `ASpaceLeagueCharacterBase` (dash, jump, stop ball, orbit aim spline, look input).
- 2026-03-12: `OrbitAim` consolide en C++:
  - point milieu d'arrondi
  - courbure verticale descendante reelle
  - clip de la courbe sur obstacle
  - pool de `SplineMesh` runtime
- 2026-03-12: Rapport de synthese ajoute: `Documentation/reports/bp-paradoxe-refactor-session-2026-03-12.md`.


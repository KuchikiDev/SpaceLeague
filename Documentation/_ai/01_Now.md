---
type: now
status: active
last_review: 2026-02-24
owner: dylan
audience: humans-agents
source_of_truth: code-and-config
---

# 01 Now

Etat courant du projet au 2026-02-22.

## Resume executif

- Le projet Unreal actif s'appelle `MovementParadoxe`.
- Le module C++ present est `MovementParadoxe` (pas encore `SpaceLeague`).
- Les specs techniques documentent une cible `SpaceLeague` a migrer.
- La doc est riche mais en retard de synchronisation sur plusieurs points critiques.

## Point de reprise (session)

- `LEAG-004` reste resolu structurellement.
- `GetSelectedLegendData` est considere reimplemente (retour utilisateur).
- Il reste uniquement une validation fonctionnelle runtime (pas de correction structurelle en attente sur LEAG-004).
- Checks cibles a faire quand UE est ouvert:
  - compiler `GI_SpaceLeague` + `GM_Sandbox`
  - tester le flow menu/selection -> sandbox (appel `GetSelectedLegendData`)

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

### Assets importants detectes

- `ProjetUE5/MovementParadoxe/Content/Instance/GI_SpaceLeague.uasset`
- `ProjetUE5/MovementParadoxe/Content/Legends/BP_Paradoxe.uasset`
- `ProjetUE5/MovementParadoxe/Content/Terrain/Blueprints/BP_Ball.uasset`
- `ProjetUE5/MovementParadoxe/Content/Game/Gameplay/Gamemodes/BP_GameState_SpaceLeague.uasset`
- `ProjetUE5/MovementParadoxe/Content/Game/Data/Legends/DA_LegendBase_Raijin.uasset`
- `ProjetUE5/MovementParadoxe/Content/Game/Data/Legends/DA_LegendBase_Keplar.uasset`
- `ProjetUE5/MovementParadoxe/Content/Game/Core/GI_SpaceLeague.uasset` absent (LEAG-004 structurellement resolu)

## Ecarts docs vs reel

1. Module C++ cible `SpaceLeague` non cree localement.
2. Config projet des specs (`GlobalDefaultGameMode=/Script/SpaceLeague...`) non alignes avec la config reelle.
3. Plusieurs checklists roadmap/migration non materialisees comme terminees.
4. `RESUME_JEU.md` contient un marqueur de template non rendu sur la date de generation.

## Priorites recommandees (prochaines sessions)

1. Clarifier la strategie: migrer vers module `SpaceLeague` ou consolider `MovementParadoxe`.
2. Finaliser LEAG-003 en migrant les referents encore lies a `Content/Game/Old`.
3. Valider le comportement runtime de `GetSelectedLegendData` (reimplementation utilisateur confirmee; checks cibles `GI_SpaceLeague` + `GM_Sandbox`, puis flow menu/selection -> sandbox).
4. Passer `ROADMAP.md` en suivi d'execution reel (done/in-progress/blocked date).
5. Mettre a jour `SPECS_TECHNIQUES.md` selon la decision architecture.

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


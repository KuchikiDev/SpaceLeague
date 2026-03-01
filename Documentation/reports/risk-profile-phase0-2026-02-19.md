---
type: risk-profile
status: active
last_review: 2026-02-19
owner: codex
audience: humans-agents
---

# Risk Profile - Phase 0 (MovementParadoxe)

Date: 2026-02-19  
Reviewer: Codex (risk-profile)

## Executive Summary

- Risks identified: 9
- Critical risks (score 9): 3
- High risks (score 6): 4
- Medium risks (score 4): 1
- Low risks (score 2-3): 1
- Overall risk score: 13/100 (elevated, pre-migration hardening required)

Scope cible:

- `BP_Paradoxe`
- `BP_Ball`
- `BP_GameMode_SpaceLeague` / runtime `BP_GM_Hub`
- `BP_GameState_SpaceLeague`
- `GI_SpaceLeague`

## Gate YAML Block

```yaml
risk_summary:
  totals:
    critical: 3
    high: 4
    medium: 1
    low: 1
  highest:
    id: DATA-001
    score: 9
    title: "Schema DataAsset casse (struct manquante + map invalide)"
  recommendations:
    must_fix:
      - "Restaurer/creer ST_AbilityPresentationOverride et corriger ST_SkinList"
      - "Corriger BP_ParadoxeRaijin/Keplar (pins casses, variables manquantes)"
      - "Resoudre doublon GI_SpaceLeague et figer un seul chemin runtime"
    monitor:
      - "Stabiliser build local en -NoXGE tant que XGE n'est pas operationnel"
      - "Suivre les warnings include-order/toolchain avant upgrade engine policy"
```

## Risk Matrix

| Risk ID | Description | Probability | Impact | Score | Priority |
|---|---|---:|---:|---:|---|
| DATA-001 | Struct manquante `ST_AbilityPresentationOverride` casse `DA_LegendSkin` | High (3) | High (3) | 9 | Critical |
| DATA-002 | `ST_SkinList` invalide dans `DA_LegendRegistry` | High (3) | High (3) | 9 | Critical |
| TECH-001 | `BP_ParadoxeRaijin/Keplar` en erreur compile (pins + variable) | High (3) | High (3) | 9 | Critical |
| OPS-001 | Doublon `GI_SpaceLeague` (2 emplacements) | High (3) | Medium (2) | 6 | High |
| TECH-002 | Drift architecture runtime (`BP_GM_Hub` actif, `BP_GameMode_SpaceLeague` absent) | Medium (2) | High (3) | 6 | High |
| TECH-003 | Migration `BP_GameState_SpaceLeague` vers C++ risque réplication | Medium (2) | High (3) | 6 | High |
| OPS-002 | Build XGE instable (license/infrastructure), pipeline non fiable | High (3) | Medium (2) | 6 | High |
| TECH-004 | Migration `BP_Ball` sans tests automatisés dédiés | Medium (2) | Medium (2) | 4 | Medium |
| OPS-003 | Absence de tests source (`test_files=0`) | Low (1) | Medium (2) | 2 | Low |

## Evidence

- Struct manquante et DataAsset casses:
  - `ProjetUE5/MovementParadoxe/Saved/Logs/MovementParadoxe.log:1258`
  - `ProjetUE5/MovementParadoxe/Saved/Logs/MovementParadoxe.log:1339`
  - `ProjetUE5/MovementParadoxe/Saved/Logs/MovementParadoxe.log:1340`
- `DA_LegendRegistry` invalide:
  - `ProjetUE5/MovementParadoxe/Saved/Logs/MovementParadoxe.log:1262`
  - `ProjetUE5/MovementParadoxe/Saved/Logs/MovementParadoxe.log:1263`
- Blueprints personnages casses:
  - `ProjetUE5/MovementParadoxe/Saved/Logs/MovementParadoxe.log:1268`
  - `ProjetUE5/MovementParadoxe/Saved/Logs/MovementParadoxe.log:1273`
- Drift config/runtime:
  - `ProjetUE5/MovementParadoxe/Config/DefaultEngine.ini:13`
  - `ProjetUE5/MovementParadoxe/Content/Levels/HUB/BP_GM_Hub.uasset`
- Doublon GI:
  - `ProjetUE5/MovementParadoxe/Content/Instance/GI_SpaceLeague.uasset`
  - `ProjetUE5/MovementParadoxe/Content/Game/Core/GI_SpaceLeague.uasset`
- Build infra:
  - `Documentation/reports/health-check-2026-02-19.md:47`
  - `Documentation/reports/health-check-2026-02-19.md:51`

## Detailed Mitigations

### DATA-001 / DATA-002

- Strategy: Preventive + Corrective
- Actions:
  - Restaurer ou recreer `ST_AbilityPresentationOverride`.
  - Verifier schema `ST_SkinList` et recompiler `DA_LegendRegistry`, `DA_LegendSkin`.
  - Ouvrir/Save+Compile des DataAssets dependants apres correction.
- Testing focus:
  - Chargement startup sans `LoadErrors`.
  - Compile Blueprint sans erreurs sur DataAssets.

### TECH-001

- Strategy: Corrective
- Actions:
  - Corriger nodes `Break ST Spell` et variable `S_SecondSpell` sur:
    - `BP_ParadoxeRaijin`
    - `BP_ParadoxeKeplar`
  - Recompiler les deux BPs + `BP_Paradoxe`.
- Testing focus:
  - Smoke test personnages en PIE.
  - Validation capacites de base (no runtime warnings).

### OPS-001

- Strategy: Preventive
- Actions:
  - Choisir un seul `GI_SpaceLeague` runtime.
  - Mettre a jour references/maps et supprimer l'autre apres verification.
- Testing focus:
  - Validation `GameInstanceClass` en boot.
  - Re-scan references avant suppression.

### TECH-002 / TECH-003

- Strategy: Preventive
- Actions:
  - Decider explicitement la cible runtime (Hub actuel vs GameMode SpaceLeague).
  - Migrer `GameState` en dernier de la phase fondation apres stabilisation Data.
  - Checkpoint compile + smoke test apres chaque reparentage majeur.
- Testing focus:
  - Score/match state replication baseline (meme en local).

### OPS-002

- Strategy: Detective + Corrective
- Actions:
  - Standardiser build local avec `-NoXGE` dans le workflow court terme.
  - Traiter XGE seulement apres stabilisation produit (non bloquant feature).
- Testing focus:
  - Reproductibilite build sur 3 runs consecutifs.

### TECH-004 / OPS-003

- Strategy: Preventive
- Actions:
  - Ajouter au minimum une suite smoke automation pour Ball + Character.
  - Ajouter checklist manuelle stricte tant que suite auto absente.
- Testing focus:
  - Scenarios de non-regression autour de `BP_Ball` et `BP_Paradoxe`.

## Risk-Based Testing Priority

1. Priority 1 (Critical):
   - Data/Blueprint compile integrity (`DA_LegendRegistry`, `DA_LegendSkin`, Raijin, Keplar)
2. Priority 2 (High):
   - GI uniqueness + runtime mode consistency + build stability
3. Priority 3 (Medium/Low):
   - Ball migration tests + minimal automation baseline

## Risk Acceptance

- Must fix before continuing migration:
  - DATA-001, DATA-002, TECH-001
- Can proceed with controls:
  - OPS-002 (if `-NoXGE` policy enforced)
- Not accepted as-is:
  - OPS-001 (GI duplicate), TECH-002 (mode drift)

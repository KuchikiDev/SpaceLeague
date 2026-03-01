---
type: incident
status: active
date: 2026-02-21
ticket: LEAG-004
owner: codex
---

# LEAG-004 Consolidation Attempt (Headless)

## Scope

Resolve duplicate `GI_SpaceLeague` assets:

- `/Game/Instance/GI_SpaceLeague`
- `/Game/Game/Core/GI_SpaceLeague`

## Attempts

1. Consolidate `Core -> Instance`
- Result: partial success in filesystem, but Blueprint regression in `GM_Sandbox` (`GetSelectedLegendData` missing).

2. Consolidate `Instance -> Core`
- Result: `Critical Failure to Consolidate Assets` with multiple affected packages.

3. Recovery / rollback snapshots
- Multiple backups were created and reused to recover a runnable state.

## Final Status

- `LEAG-004`: `DONE` (manual resolution in UE on 2026-02-22)
- Headless mode was insufficient; final cleanup was completed interactively.
- Current probes:
  - `Documentation/reports/asset-referencers-phase0.json`
  - `Documentation/reports/leag004-probe-2026-02-22-after-user-fix.json`

## Residual Blueprint blockers observed

- Structural duplicate issue resolved (no referencers left on `/Game/Game/Core/GI_SpaceLeague`)
- Remaining functional risk (user note): `GetSelectedLegendData` later reimplemented; runtime validation still required

## Snapshots / backups

- `ProjetUE5/MovementParadoxe/.old/leag-004-preconsolidation-20260221-110714/`
- `ProjetUE5/MovementParadoxe/.old/leag-004-recovery-before-reverse-20260221-111418/`
- `ProjetUE5/MovementParadoxe/.old/leag-004-rollback-safety-20260221-112020/`
- `ProjetUE5/MovementParadoxe/.old/leag-004-state-before-rollbacksafety-restore-20260221-112734/`

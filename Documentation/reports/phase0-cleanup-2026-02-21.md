---
type: report
status: active
date: 2026-02-21
owner: codex
scope: phase0-cleanup-leag-001-004
---

# Phase 0 Cleanup Report (LEAG-001 to LEAG-004)

## Execution Summary

- Cleanup date: 2026-02-21
- Archive root: `ProjetUE5/MovementParadoxe/.old/phase0-cleanup-20260221-015522/`
- Strategy: move only assets verified as unreferenced

## Ticket Status

| Ticket | Status | Notes |
|---|---|---|
| LEAG-001 | DONE | `BP_ParadoxeJhin` archived |
| LEAG-002 | DONE | `BP_ParadoxeSenna` archived |
| LEAG-003 | PARTIAL | Only unreferenced asset moved. `Content/Game/Old` still contains referenced assets |
| LEAG-004 | DONE | Resolved manually in UE on 2026-02-22 (Core GI removed, references cleaned) |

## Moved Assets

- `Content/Legends/Blueprints/BP_ParadoxeJhin.uasset`
- `Content/Legends/Blueprints/BP_ParadoxeSenna.uasset`
- `Content/Game/Old/DataAsset/ST_CharacterUI.uasset`

## Remaining Referenced Assets in `Content/Game/Old`

- `/Game/Game/Old/E_Legend` (11 referencers)
- `/Game/Game/Old/S_Legends` (4 referencers)
- `/Game/Game/Old/DT_LegendDB` (1 referencer)
- `/Game/Game/Old/DataAsset/DA_Legend` (7 referencers)
- `/Game/Game/Old/DataAsset/Legend/DA_Jhin` (1 referencer)
- `/Game/Game/Old/DataAsset/Legend/DA_Senna` (1 referencer)

## GI Duplicate Findings (`LEAG-004`)

Current state:

- `/Game/Instance/GI_SpaceLeague`  
  - Referencers: 6 (`GI_Proxy`, `BP_Paradoxe`, `UW_MainMenu`, `GM_Sandbox`, `BPPC_Paradoxe`, `UW_HUD`)
  - Dependencies: `/Game/Game/Old/DataAsset/DA_Legend`

- `/Game/Game/Core/GI_SpaceLeague`  
  - Asset file: absent (`load_asset=false`, no valid AssetRegistry entry)
  - Referencers: none (after user fix on 2026-02-22)
  - Dependencies: none (asset absent)

Attempts executed:

- Attempt A: consolidate `Core -> Instance` (headless UE)
  - Source asset was removed, but `GM_Sandbox` then failed on missing `GetSelectedLegendData`.
- Attempt B: reverse consolidate `Instance -> Core`
  - Triggered critical partial consolidation and touched multiple assets.
- Recovery:
  - Multiple rollback snapshots were created and reapplied to recover a runnable state.

Conclusion: duplicate resolved manually in UE Editor on 2026-02-22 after headless attempts failed.

## Validation

- C++ build: `MovementParadoxeEditor Win64 Development` -> up to date, success.
- UE startup: commandline startup reached `Cmd: QUIT`; process required manual stop afterwards (EOS background loop), but no missing-reference errors were found for moved assets.
- Known pre-existing Blueprint/Data errors remain (unrelated to this cleanup):
  - `DA_LegendRegistry` / `DA_LegendSkin` struct issues
  - `BP_ParadoxeKeplar` / `BP_ParadoxeRaijin` pin issues
- LEAG-004 validation (2026-02-22):
  - `BP_Paradoxe`, `BPPC_Paradoxe`, `UW_HUD`, `GI_Proxy`, `GM_Sandbox`: dependencies now point to `/Game/Instance/GI_SpaceLeague`
  - `/Game/Game/Core/GI_SpaceLeague`: no longer exists and no referencers detected in probe
  - Caution: `GetSelectedLegendData` was later reported as reimplemented by the user; runtime behavior validation is still pending

## Artifacts

- Referencer scan: `Documentation/reports/asset-referencers-phase0.json`
- LEAG-004 incident report: `Documentation/reports/leag004-consolidation-attempt-2026-02-21.md`
- Archive manifest: `ProjetUE5/MovementParadoxe/.old/phase0-cleanup-20260221-015522/manifest.json`
- LEAG-004 backup manifests:
  - `ProjetUE5/MovementParadoxe/.old/leag-004-preconsolidation-20260221-110714/manifest.json`
  - `ProjetUE5/MovementParadoxe/.old/leag-004-recovery-before-reverse-20260221-111418/manifest.json`
  - `ProjetUE5/MovementParadoxe/.old/leag-004-rollback-safety-20260221-112020/manifest.json`

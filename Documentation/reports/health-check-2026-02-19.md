---
type: report
status: active
last_review: 2026-02-19
owner: codex
audience: humans-agents
---

# UE5 Health Check Report - MovementParadoxe

Date: 2026-02-19  
Mode: phase0 baseline

## Scope

- Project validation: `ProjetUE5/MovementParadoxe/MovementParadoxe.uproject`
- Engine/toolchain detection
- C++ build check
- Headless launch smoke check
- Asset/content sanity checks for Phase 0 items

## Summary

Overall status: `YELLOW` (build works, but content/data issues block a clean Phase 0)

- Build (local, no XGE): `PASS`
- Build (XGE default): `FAIL` (infrastructure/license)
- Headless launch: `INCONCLUSIVE` (process did not exit within timeout)
- Blueprint/data integrity: `FAIL` (multiple Blueprint compile errors)
- Cleanup goals (LEAG-001..004): `NOT DONE`

## Findings

### 1. Project and engine

- `MovementParadoxe.uproject` exists and is readable.
- `EngineAssociation`: `5.5`.
- UE installation detected from registry:
  - `D:\Unreal Engine\UE_5.5\Engine\Build\BatchFiles\Build.bat`

### 2. Build

Attempt A (default executor):

- Command: `Build.bat MovementParadoxeEditor ... -waitmutex`
- Result: `FAIL`
- Cause: XGE/IncrediBuild infrastructure error (`Task queue management failed`, license not activated)

Attempt B (local executor):

- Command: `Build.bat MovementParadoxeEditor ... -waitmutex -NoXGE`
- Result: `PASS`
- Total execution time: `62.97s`
- Warnings:
  - compiler version not preferred (VS 14.50 vs UE preferred 14.38)
  - include order still `Unreal5_3` (upgrade notice to `Unreal5_5`/`Latest`)

### 3. Headless launch smoke check

- Command: `UnrealEditor-Cmd ... -ExecCmds="QUIT" -nullrhi -unattended`
- Result: `TIMEOUT` at 10 minutes.
- `QUIT` command was logged, but process stayed alive and required manual kill.
- No fatal/assert detected in latest log.

### 4. Content and assets

- Content size: `16.57 GB`
- Content files: `9729`
- Duplicate critical asset detected:
  - `Content/Instance/GI_SpaceLeague.uasset`
  - `Content/Game/Core/GI_SpaceLeague.uasset`
- Legacy/cleanup state:
  - `Content/Game/Old` exists
  - `.old` exists
  - `BP_ParadoxeJhin` still present
  - `BP_ParadoxeSenna` still present

### 5. Blueprint/data integrity (critical)

From latest runtime log `ProjetUE5/MovementParadoxe/Saved/Logs/MovementParadoxe.log`:

- Blueprint errors: `13`
- Blueprint warnings: `20`
- LoadErrors: `1`
- Fatal/assert: `0`

Notable blocking errors:

- `DA_LegendRegistry` invalid map struct type (`ST_SkinList` empty/invalid)
- `DA_LegendSkin` unknown/missing structure (`ST_AbilityPresentationOverride`)
- `BP_ParadoxeRaijin` and `BP_ParadoxeKeplar` broken pins (`Break ST Spell`, missing `S_SecondSpell`)

### 6. Tests

- No test files detected in `Source/` (`test_files=0`).
- Automated test coverage for Phase 0 cannot be validated yet with current repository state.

## Phase 0 criteria snapshot

- [x] Project compiles (C++) with local executor (`-NoXGE`)
- [ ] Game launches cleanly and exits cleanly (headless run timed out)
- [ ] Base movement validated (not executed)
- [ ] Ball behavior validated (not executed)
- [ ] Score display validated (not executed)
- [ ] Blueprint/data layer clean (currently failing)

## Priority actions

1. `P0` Fix data struct breakage:
   - restore/create `ST_AbilityPresentationOverride`
   - fix `ST_SkinList` usage in `DA_LegendRegistry` and `DA_LegendSkin`
2. `P0` Repair broken Blueprint pins and missing vars in:
   - `BP_ParadoxeRaijin`
   - `BP_ParadoxeKeplar`
3. `P1` Resolve duplicate `GI_SpaceLeague` and choose single source path.
4. `P1` Execute LEAG-001/002 cleanup (`BP_ParadoxeJhin`, `BP_ParadoxeSenna`) after reference validation.
5. `P2` Stabilize build defaults:
   - use `-NoXGE` in local workflow, or fix XGE licensing
   - optionally update include order target to `Unreal5_5` after compatibility pass.

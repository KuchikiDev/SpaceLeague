---
type: changelog
status: active
last_review: 2026-03-12
owner: dylan
audience: humans-agents
---

# Changelog Docs

Historique des changements du vault documentaire.

## 2026-02-19

- Ajout du portail `[[_ai/00_Home]]`.
- Ajout du snapshot d'etat `[[_ai/01_Now]]`.
- Ajout du protocole agent `[[_ai/AGENT_CONTEXT]]`.
- Ajout des conventions `[[_ai/VAULT_CONVENTIONS]]`.
- Ajout des MOCs dans `Documentation/_maps/`.
- Ajout des templates dans `Documentation/_templates/`.
- Ajout de metadonnees + navigation sur les docs existantes.
- Mise a jour `Documentation/.obsidian/workspace.json` pour ouvrir `00_Home.md`.
- Ajout `Documentation/.obsidian/templates.json` (dossier templates = `_templates`).
- Ajout frontmatter/navigation pour `README_SETUP_BMAD_UE5.md` et `GUIDE_SKILLS_UE5.md`.
- Ajout du routeur `PHASE0_COMMAND_ROUTER.md` (selection commande par situation + prompts prets a coller).
- Ajustement workflow: remplacement de `/ue5-check-references` par `/ue5-health-check --report`.
- Nettoyage references workflow: `WORKFLOW_UE5_SKILLS.md` -> `GUIDE_SKILLS_UE5.md`, suppression de `/ue5-backup-project` dans commandes rapides.
- Nettoyage references skills non presentes: remplacement des usages `/ue5-check-references` et `/ue5-restore-asset` par procedures supportees.
- Alignement des specs skills UE5 (`.claude/commands/UE5`): retrait des references a commandes non implementees (restore/list/cleanup/backups projet).
- Ajout du rapport health check Phase 0:
  - `Documentation/reports/health-check-2026-02-19.md`
  - `Documentation/reports/health-check-2026-02-19.json`
- Mise a jour `01_Now.md` avec l'etat issu du health check.
- Ajout du rapport architecture:
  - `Documentation/reports/architecture-review-2026-02-19.md`
- Ajout du rapport risk profile Phase 0:
  - `Documentation/reports/risk-profile-phase0-2026-02-19.md`
  - `Documentation/reports/risk-profile-phase0-2026-02-19.json`
- Ajout du test design Phase 0:
  - `docs/qa/assessments/0.0-test-design-20260219.md`
  - `docs/qa/phase0-test-plan.md`

## 2026-02-24

- Mise a jour `[[_ai/01_Now]]` avec point de reprise post `LEAG-004` (structure OK, validation runtime restante).
- Mise a jour `[[_ai/PHASE0_COMMAND_ROUTER]]` avec section "Reprise actuelle (post LEAG-004)".
- Correction des mentions obsoletes "placeholder vide" pour `GetSelectedLegendData` dans les rapports Phase 0 / LEAG-004.

## 2026-03-08

- Mise a jour de `[[_ai/01_Now]]` avec l'etat reel du socle `SpaceLeague` cote C++:
  - routeur ability par slot
  - fail reason exposee
  - cache/reuse widget in-game local
- Ajout du rapport de session:
  - `Documentation/reports/ability-hud-session-2026-03-08.md`
- Formalisation du point de reprise Blueprint:
  - events `ExecuteSkillAbility` / `ExecuteUltimateAbility`
  - wiring `OnAbilityCooldownStarted` vers `UW_HUD`
  - timer HUD looping (`Time=0.05`) + handle stocke.

## 2026-03-12

- Mise a jour de `[[_ai/01_Now]]` avec l'etat reel de `BP_Paradoxe`:
  - gameplay core consolide dans `ASpaceLeagueCharacterBase`
  - `OrbitAim` migre et stabilise cote C++
  - priorites Blueprint recadrees sur le wiring / presentation
- Ajout du rapport de synthese:
  - `Documentation/reports/bp-paradoxe-refactor-session-2026-03-12.md`
- Documentation du systeme `Stop Ball` / `Orbit Aim`:
  - courbure verticale
  - clipping obstacle
  - pool runtime de `SplineMesh`
  - inputs `IA_OrbitAimCurve`


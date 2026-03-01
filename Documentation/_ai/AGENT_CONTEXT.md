---
type: agent-context
status: active
last_review: 2026-02-19
owner: dylan
audience: agents-skills
---

# Agent Context

Contexte minimal a charger pour toute intervention agent/skill.

## Objectif

Maintenir une source de verite documentaire exploitable par humain + agent, alignee sur le projet Unreal reel.

## Ordre de lecture obligatoire

1. [[_ai/01_Now]]
2. [[_ai/00_Home]]
3. [[WORKFLOW_PHASE0_BMAD]]
4. [[ROADMAP]]
5. Docs detail selon la tache:
   - Gameplay: [[SPECS_FONCTIONNELLES]], [[RESUME_JEU]]
   - Architecture: [[ARCHITECTURE]], [[SPECS_TECHNIQUES]]
   - Migration: [[IMPACT_MIGRATION]]
6. Skills/workflows operationnels:
   - [[README_SETUP_BMAD_UE5]]
   - [[GUIDE_SKILLS_UE5]]
   - [[_ai/PHASE0_COMMAND_ROUTER]]

## Regles de decision

1. `Code/config > docs`.
2. Si un fait n'est pas verifiable, le marquer explicitement en hypothese.
3. Mettre a jour `[[_ai/01_Now]]` apres toute session impactant architecture, config ou roadmap.
4. Ne jamais supprimer l'historique: archiver dans `Documentation/_archive/`.

## Format minimal des notes actives

Frontmatter requis:

- `type`
- `status` (`active`, `draft`, `planned`, `archived`, `blocked`)
- `last_review` (YYYY-MM-DD)
- `owner`

## Protocoles de mise a jour agents

### Apres modification code

1. Verifier si `01_Now` doit changer.
2. Verifier si roadmap/statut doit changer.
3. Ajouter une entree dans `CHANGELOG_DOCS.md`.

### Apres modification doc seulement

1. Garder les liens wiki coherents.
2. Mettre `last_review`.
3. Ajouter une entree dans `CHANGELOG_DOCS.md`.

## Zones sensibles

- Mismatch historique `SpaceLeague` vs `MovementParadoxe`.
- Multiples emplacements potentiels de `GI_SpaceLeague`.
- Documents legacy dans `_archive` references depuis des docs actives.

## Liens utilitaires

- [[_ai/VAULT_CONVENTIONS]]
- [[_ai/CHANGELOG_DOCS]]
- [[_templates/TEMPLATE_NOTE]]
- [[_templates/TEMPLATE_TASK]]
- [[_templates/TEMPLATE_DECISION]]


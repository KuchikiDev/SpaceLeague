---
type: hub
status: active
last_review: 2026-02-19
owner: dylan
audience: humans-agents
---

# 00 Home

Point d'entree unique du vault Obsidian pour Space League / MovementParadoxe.

## Demarrage rapide

1. Lire [[_ai/01_Now]] pour l'etat reel du projet (snapshot concret).
2. Lire [[_ai/AGENT_CONTEXT]] pour les regles de travail agents/skills.
3. Executer le plan via [[WORKFLOW_PHASE0_BMAD]] puis [[ROADMAP]].
4. Pour les details techniques, ouvrir [[ARCHITECTURE]] puis [[SPECS_TECHNIQUES]].

## Carte des domaines

| Domaine | Doc pivot | Docs associees |
|---|---|---|
| Vision gameplay | [[SPECS_FONCTIONNELLES]] | [[RESUME_JEU]], [[_maps/MOC_Gameplay]] |
| Architecture cible | [[ARCHITECTURE]] | [[SPECS_TECHNIQUES]], [[_maps/MOC_Architecture]] |
| Migration C++/BP | [[IMPACT_MIGRATION]] | [[WORKFLOW_PHASE0_BMAD]], [[_maps/MOC_Migration]] |
| Execution | [[_ai/01_Now]] | [[ROADMAP]], [[WORKFLOW_PHASE0_BMAD]], [[_maps/MOC_Execution]] |
| Gouvernance docs | [[_ai/VAULT_CONVENTIONS]] | [[_ai/CHANGELOG_DOCS]], [[_templates/TEMPLATE_NOTE]] |

## Regles de verite (important)

1. Le code/config gagne sur la documentation en cas de conflit.
2. `01_Now.md` est le snapshot de reference a mettre a jour apres chaque session importante.
3. Les documents obsoletes vont dans `Documentation/_archive/`.
4. Chaque note active doit avoir un frontmatter minimal (`status`, `last_review`, `owner`).

## Ecarts connus a traiter

- [ ] Nom de module: specs ecrites pour `SpaceLeague`, code actuel en `MovementParadoxe`.
- [ ] GameMode cible dans specs vs GameMode reel dans `DefaultEngine.ini`.
- [ ] Doubles assets `GI_SpaceLeague` detectes dans Content.
- [ ] Tickets roadmap en grande partie non traces comme "done".

## Navigation complementaire

- [[_INDEX_DOCUMENTATION]]
- [[_maps/MOC_Gameplay]]
- [[_maps/MOC_Architecture]]
- [[_maps/MOC_Migration]]
- [[_maps/MOC_Execution]]
- [[_archive/CORRECTIONS_PRIORITAIRES]]

## Skills et workflows operationnels

- [[README_SETUP_BMAD_UE5]]
- [[GUIDE_SKILLS_UE5]]
- [[_ai/PHASE0_COMMAND_ROUTER]]


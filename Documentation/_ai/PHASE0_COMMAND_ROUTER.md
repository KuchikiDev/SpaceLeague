---
type: playbook
status: active
last_review: 2026-02-24
owner: dylan
audience: humans-agents
---

# Phase 0 Command Router

Selection fiable des agents/tasks/skills a utiliser pour la Phase 0, basee sur les commandes reellement presentes dans `.claude/commands`.

## Navigation

- [[_ai/00_Home]]
- [[_ai/01_Now]]
- [[_ai/AGENT_CONTEXT]]
- [[WORKFLOW_PHASE0_BMAD]]
- [[GUIDE_SKILLS_UE5]]
- [[architecture-review-2026-02-19]]
- [[risk-profile-phase0-2026-02-19]]
- Test design matrix: `docs/qa/assessments/0.0-test-design-20260219.md`

## Reprise actuelle (post LEAG-004)

- `LEAG-001` / `LEAG-002`: done
- `LEAG-003`: partial (assets `Game/Old` encore references)
- `LEAG-004`: resolu structurellement (GI duplique consolide cote references)
- `GetSelectedLegendData`: considere reimplante (retour utilisateur)
- Prochaine validation a executer en UE (runtime):
  1. Compiler `GI_SpaceLeague` + `GM_Sandbox`
  2. Tester menu/selection -> sandbox (appel `GetSelectedLegendData`)

## Inventaire verifie (local)

### Agents BMad disponibles

- `/architect`
- `/dev`
- `/qa`
- `/pm`
- `/po`
- `/analyst`
- `/sm`
- `/ux-expert`
- `/bmad-master`
- `/bmad-orchestrator`

### Tasks BMad utiles Phase 0

- `/risk-profile`
- `/test-design`
- `/qa-gate`
- `/trace-requirements`
- `/nfr-assess`
- `/apply-qa-fixes`

### Skills UE5 disponibles

- `/ue5-health-check`
- `/ue5-compile`
- `/ue5-test`
- `/ue5-backup-asset`
- `/migration-check`
- `/ue5-create-module`

## Ecart detecte dans le workflow

- `WORKFLOW_PHASE0_BMAD.md` reference `/ue5-check-references` (non present dans `.claude/commands/UE5`).
- Remplacement recommande:
  - verification references via `/ue5-health-check --report`
  - puis verification ciblee avec `/migration-check <Blueprint> --parent <ClasseCpp>`

## Decision matrix: quoi lancer et quand

| Situation | Commande principale | Validation immediate |
|---|---|---|
| Etat global avant de commencer | `/ue5-health-check --report` | verifier rapport + blockers |
| Decision architecture | `/architect` | conclusions ecrites + ecarts |
| Analyse risque migration | `/risk-profile` | risques classes par priorite |
| Plan de tests phase 0 | `/test-design` | checklist tests prete |
| Impl C++ / refacto | `/dev` | `/ue5-compile` |
| Verification build | `/ue5-compile` | 0 erreur bloquante |
| Verification feature | `/ue5-test --filter <feature>` | tests passes |
| Avant migration BP->C++ | `/ue5-backup-asset <asset>` | backup cree |
| Apres migration BP->C++ | `/migration-check <BP> --parent <CppClass>` | migration validee |
| Gate qualite fin de phase | `/qa-gate` + `/qa` | GO / NO-GO explicite |

## Pas-a-pas recommande (phase 0)

### Jour 1 - Preparation

1. `/ue5-health-check --report`
2. `/architect`
3. `/risk-profile`
4. `/test-design`

### Jour 2 - Nettoyage LEAG-001 a LEAG-004

1. `/dev` pour appliquer suppressions/nettoyages.
2. `/ue5-compile`
3. `/ue5-health-check`
4. `/qa` pour verifier absence de regression.

### Jours 3-5 - Migration C++ phase 1

1. `/ue5-create-module SpaceLeague` (si la decision est de migrer vers module `SpaceLeague`).
2. `/dev` pour classes C++ prioritaires.
3. `/ue5-compile`
4. `/ue5-test --filter Phase0`

### Jours 6-8 - Migration Blueprint vers C++

Pour chaque BP:

1. `/ue5-backup-asset Content/.../<Blueprint>.uasset`
2. migration dans UE5 Editor (reparentage)
3. `/migration-check <Blueprint> --parent <ClasseCpp>`
4. `/ue5-compile`
5. `/ue5-test --filter <feature>`

### Jours 9-10 - Corrections

1. `/dev`
2. `/ue5-compile`
3. `/ue5-health-check --report`

### Jours 11-12 - Validation finale phase 0

1. `/qa-gate`
2. `/qa`
3. `/ue5-health-check --phase0 --report`

## Prompts prets a coller

### Prompt `/architect`

```text
Analyse Documentation/ARCHITECTURE.md et Documentation/SPECS_TECHNIQUES.md.
Objectif: lister les ecarts entre cible SpaceLeague et etat reel MovementParadoxe (code + DefaultEngine.ini), puis proposer une decision: migrer vers SpaceLeague ou consolider MovementParadoxe.
Sortie attendue: decision, impacts, risques, plan en 5 etapes max.
```

### Prompt `/dev` (nettoyage)

```text
Traite LEAG-001 a LEAG-004 en priorite.
Contraintes: ne rien casser, verifier references avant suppression, journaliser les changements, et valider avec compilation a la fin.
```

### Prompt `/qa`

```text
Execute une verification Phase 0: compilation, lancement sans crash, mouvements de base, balle, score.
Retourne un statut GO/NO-GO avec liste de blocants.
```

### Prompt `/risk-profile`

```text
Evalue les risques de migration BP_Paradoxe, BP_Ball, BP_GameState_SpaceLeague, GI_SpaceLeague.
Classe par probabilite x impact, avec mitigation concrete et ordre de migration.
```

## Regles d'usage

1. Toujours lancer `/ue5-backup-asset` avant un reparentage Blueprint.
2. Toujours enchainer migration -> `/migration-check` -> `/ue5-compile` -> `/ue5-test`.
3. Si conflit docs/code: suivre `[[_ai/01_Now]]` et mettre a jour les docs ensuite.


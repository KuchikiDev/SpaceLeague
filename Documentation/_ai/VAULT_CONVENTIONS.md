---
type: conventions
status: active
last_review: 2026-02-19
owner: dylan
audience: humans-agents
---

# Vault Conventions

Conventions de structure et maintenance du vault Obsidian.

## Arborescence

- `00_Home.md`: portail principal.
- `01_Now.md`: etat reel verifie.
- `AGENT_CONTEXT.md`: protocole de travail pour agents/skills.
- `_maps/`: MOCs par domaine.
- `_templates/`: modeles de notes.
- `_archive/`: documentation obsolete ou remplacee.

## Convention de nommage

1. Fichiers pivots numerotes (`00_`, `01_`) pour ordre de lecture.
2. Noms explicites et stables pour etre faciles a parser par agents.
3. Eviter les renommages frequents des fichiers pivots.

## Frontmatter minimal (notes actives)

```yaml
---
type: <hub|now|spec|architecture|roadmap|workflow|decision|note>
status: <active|draft|planned|blocked|archived>
last_review: YYYY-MM-DD
owner: <person-or-team>
---
```

## Lien et navigation

1. Preferer liens wiki `[[NomNote]]`.
2. Chaque note active doit lier au minimum:
   - `[[_ai/00_Home]]`
   - `[[_ai/01_Now]]`
   - un MOC pertinent.
3. Les docs legacy restent accessibles mais marquees comme archivees.

## Synchronisation doc/code

Cadence recommandee:

- A chaque session code significative: mise a jour `01_Now`.
- Hebdomadaire: revue coherence `Architecture`, `Specs techniques`, `Roadmap`.
- Mensuelle: nettoyage archive et dette documentaire.

## Regles d'archivage

1. Ne pas supprimer une note active sans remplacement.
2. Deplacer vers `_archive/` quand obsolete.
3. Ajouter une mention de remplacement en tete de la note archivee.


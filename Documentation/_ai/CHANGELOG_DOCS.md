---
type: changelog
status: active
last_review: 2026-02-24
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

## 2026-05-10

### Passe 1 — Renommage du nom du jeu dans la documentation
- Renommage global du jeu "Space League" → "ORA" dans toute la documentation (tous les `.md` du vault).

### Passe 2 — Renommage complet dans le projet (C++, assets, config)
- **C++ Source :** Dossier `Source/MovementParadoxe/SpaceLeague/` renommé en `Source/MovementParadoxe/ORA/`.
- **24 fichiers C++** renommés (`SpaceLeague*.h/.cpp` → `ORA*.h/.cpp`) avec classes internes mises à jour (`AORAGameMode`, `AORAGameState`, `AORACharacterBase`, `UORALegendData`, `UORAGameInstance`, `UORAHUD`, etc.).
- **Blender :** `Ball_SpaceLeague.blend/.blend1` → `Ball_ORA.blend/.blend1`.
- **Documentation** : deuxième passe complète — noms de classes C++, chemins de fichiers, noms d'assets cibles (`GI_ORA`, `BP_GameState_ORA`, `BP_GameMode_ORA`), module `ORA`, `ORATypes.h` — mis à jour dans tous les fichiers actifs.
- **Rapports historiques** (`Documentation/reports/`) : conservés tels quels (archives).
- **DefaultEngine.ini** : NON modifié — référence `GI_SpaceLeague` encore active. Mettre à jour vers `GI_ORA.GI_ORA_C` après renommage Blueprint dans UE5 Editor.

### Actions manuelles restantes (dans UE5 Editor)
1. Renommer `Content/Instance/GI_SpaceLeague` → `GI_ORA` (Right-click → Rename dans le Content Browser)
2. Renommer `Content/Game/Gameplay/Gamemodes/BP_GameState_SpaceLeague` → `BP_GameState_ORA`
3. Après ces deux renames, mettre à jour `DefaultEngine.ini` ligne `GameInstanceClass` → `/Game/Instance/GI_ORA.GI_ORA_C`
## 2026-09-26

- `[[_ai/01_Now]]` reecrit sur l'etat reel : projet `ProjetUE5/MovementORA 5.8` (UE 5.8, Perforce), gameplay en place,
  statut de la revue P0/P1/P2 du 2026-09-25, dette restante.
- Nouvelle copie Git du code ORA dans `ProjetUE5/MovementORA/` + script `Tools/Sync-ShowcaseFromPerforce.ps1`.
- `ROADMAP.md` : encart d'etat reel en tete (la roadmap d'origine reste en dessous).
- README : ORA, UE 5.8, formats 2v2/3v3, organisation Git/Perforce a jour.

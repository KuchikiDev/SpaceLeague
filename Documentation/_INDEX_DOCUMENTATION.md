---
type: index
status: active
last_review: 2026-02-19
owner: dylan
audience: humans-agents
---

# INDEX - Documentation Space League

**Date:** 2026-01-25
**Auteur:** Claude (BMAD)

**Navigation Vault:** [[_ai/00_Home]] | [[_ai/01_Now]] | [[_ai/AGENT_CONTEXT]] | [[_ai/VAULT_CONVENTIONS]]
**Maps:** [[_maps/MOC_Gameplay]] | [[_maps/MOC_Architecture]] | [[_maps/MOC_Migration]] | [[_maps/MOC_Execution]]

---

## Fichiers Créés par Claude (Session BMAD)

Ces fichiers forment un ensemble cohérent pour la migration vers une architecture C++ optimale.

## Point d'entree Obsidian (nouveau)

- Hub principal: [[_ai/00_Home]]
- Snapshot execution: [[_ai/01_Now]]
- Contexte agents: [[_ai/AGENT_CONTEXT]]
- Conventions vault: [[_ai/VAULT_CONVENTIONS]]
- Changelog doc: [[_ai/CHANGELOG_DOCS]]

### Ordre de Lecture Recommandé

| # | Fichier | Description | Quand le lire |
|---|---------|-------------|---------------|
| 1 | [[SPECS_FONCTIONNELLES]] | Règles du jeu, personnages, mécaniques, UI | Pour comprendre CE QU'ON CONSTRUIT |
| 2 | [[ARCHITECTURE]] | Architecture C++/Blueprint, structure dossiers, classes | Pour comprendre COMMENT on le construit |
| 3 | [[SPECS_TECHNIQUES]] | Code C++ détaillé (headers, variables, fonctions) | Pour IMPLEMENTER le code C++ |
| 4 | [[IMPACT_MIGRATION]] | Risques par asset, procédures de migration | AVANT de migrer chaque Blueprint |
| 5 | [[ROADMAP]] | Phases, tickets Jira, ordre d'exécution | Pour PLANIFIER le travail |

---

## Détail de Chaque Fichier

### 1. [[SPECS_FONCTIONNELLES]] (12 Ko)
**But:** Document de référence pour le gameplay

**Contenu:**
- Vue d'ensemble du concept
- Règles de base (terrain, scoring, prison)
- Mouvements des personnages
- Événements terrain (rotation, phases)
- Système de personnages (Legends)
- Interface utilisateur
- Multijoueur (local + online)

**Utilité:** Ce document définit CE QUE le jeu doit faire. C'est la référence pour toute implémentation.

---

### 2. [[ARCHITECTURE]] (12 Ko)
**But:** Définir l'architecture technique hybride

**Contenu:**
- Schéma C++ vs Blueprint (qui fait quoi)
- Structure des dossiers UE5 (cible post-migration)
- Structure Source/ C++
- Responsabilités de chaque classe
- Flux de données (sélection perso → spawn)
- Réplication réseau (priorités, variables)
- Conventions de nommage
- Dépendances modules UE5

**Utilité:** Ce document montre COMMENT organiser le code et les assets.

---

### 3. [[SPECS_TECHNIQUES]] (16 Ko)
**But:** Spécifications C++ détaillées prêtes à implémenter

**Contenu:**
- Enums C++ complets (EMatchState, EPlayerGameState, etc.)
- Structs C++ (FTeamData, FLegendData, etc.)
- Headers de toutes les classes:
  - ASpaceLeagueGameMode
  - ASpaceLeagueGameState
  - ASpaceLeagueCharacterBase
  - ABall
  - AGoal
  - APrison
  - ATerrainEventManager
- DataAssets (ULegendDataAsset, UAbilityDataAsset)
- SpaceLeague.Build.cs complet

**Utilité:** Ce document contient LE CODE à écrire. Tu peux copier-coller les headers directement.

---

### 4. [[IMPACT_MIGRATION]] (7 Ko)
**But:** Analyser les risques et sécuriser la migration

**Contenu:**
- Principe de migration (reparentage vs réécriture)
- Risques par asset (BP_Paradoxe, BP_Ball, etc.)
- Procédures sécurisées avec backup
- Ordre de migration recommandé
- Points de non-retour
- Plan de rollback

**Utilité:** Consulte ce document AVANT de migrer chaque Blueprint pour éviter les erreurs.

---

### 5. [[ROADMAP]] (7 Ko)
**But:** Planifier le travail en phases

**Contenu:**
- Vue d'ensemble des 4 phases
- Phase 0: Fondations (nettoyage + migration C++)
- Phase 1: Core Game (2 persos + événements)
- Phase 2: Multijoueur (local + online)
- Phase 3: Extension (4 persos + capacités)
- Tickets Jira référencés
- Critères de validation par phase

**Utilité:** Ce document organise le QUAND et dans quel ordre faire les choses.

---

## Fichiers Pré-existants

| Fichier | Date | Statut | Notes |
|---------|------|--------|-------|
| [[RESUME_JEU]] | 21 janv | **À GARDER** | Bon résumé du concept, complémentaire aux specs |
| [[_archive/CORRECTIONS_PRIORITAIRES]] | 17 janv | **OBSOLÈTE** | Contient instructions Blueprint, remplacé par approche C++ |
| [[README_SETUP_BMAD_UE5]] | 19 fev | **ACTIF** | Setup BMad + skills UE5 |
| [[GUIDE_SKILLS_UE5]] | 19 fev | **ACTIF** | Guide de reference des skills UE5 |

---

## Ressources Confluence

| Page | URL |
|------|-----|
| ROADMAP - Space League v1.0 | https://byakuyacorps.atlassian.net/wiki/spaces/SL/pages/212795394 |
| ARCHITECTURE TECHNIQUE | https://byakuyacorps.atlassian.net/wiki/spaces/SL/pages/212828161 |

---

## Tickets Jira Créés [Claude]

**Total: 16 tickets Phase 0**

Voir [[ROADMAP]] pour la liste complète et l'ordre d'exécution.

---

## Notes Importantes

### Migration Optimale (pas juste non-destructive)

Suite à ta demande, l'approche est désormais **OPTIMALE** et non pas juste "non-destructive":
- Si un Blueprint doit être migré entièrement en C++ pour être optimal → on le fait
- Si une logique Blueprint doit être supprimée et réécrite → on le fait
- L'objectif est la meilleure architecture possible, pas la préservation de l'existant

### Fichiers à Supprimer

Le fichier [[_archive/CORRECTIONS_PRIORITAIRES]] contient des instructions Blueprint qui sont remplacées par l'approche C++. Il peut être:
1. Supprimé (car obsolète)
2. Déplacé vers `.old/` (si tu veux garder une trace)


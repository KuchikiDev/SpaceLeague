---
type: roadmap
status: active
last_review: 2026-02-19
owner: dylan
audience: humans-agents
---

# Roadmap - Space League

**Version:** 1.0
**Date:** 2026-01-25
**Auteur:** Claude (BMAD)

**Navigation Vault:** [[_ai/00_Home]] | [[_ai/01_Now]] | [[_ai/AGENT_CONTEXT]]
**Liens relies:** [[WORKFLOW_PHASE0_BMAD]] | [[IMPACT_MIGRATION]] | [[_maps/MOC_Execution]]
**Note statut:** Plusieurs checklists sont encore ouvertes, suivre l'execution dans [[_ai/01_Now]].

---

## Vue d'Ensemble

```
┌─────────────────────────────────────────────────────────────────────────┐
│  PHASE 0           PHASE 1            PHASE 2           PHASE 3        │
│  Fondations        Core Game          Multijoueur       Extension      │
│  ───────────       ─────────          ───────────       ─────────      │
│  • Nettoyage       • 2 personnages    • Local 2-4P      • 4 personnages│
│  • Migration C++   • Événements       • Online          • Capacités    │
│  • Architecture    • Tests gameplay   • Matchmaking     • Polish       │
│                                                                         │
│  [████░░░░░░]      [░░░░░░░░░░]       [░░░░░░░░░░]      [░░░░░░░░░░]  │
│     En cours          À venir            À venir           À venir     │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## PHASE 0 : Fondations

**Objectif:** Base technique propre et architecture solide
**Durée estimée:** 2-3 semaines

### 0.1 Nettoyage Projet

| Tâche | Priorité | Ticket |
|-------|----------|--------|
| Supprimer BP_ParadoxeJhin | Haute | LEAG-001 |
| Supprimer BP_ParadoxeSenna | Haute | LEAG-002 |
| Supprimer dossier /Old/ | Moyenne | LEAG-003 |
| Résoudre doublon GI_SpaceLeague | Haute | LEAG-004 |

### 0.2 Migration C++ - Phase 1

| Tâche | Priorité | Ticket |
|-------|----------|--------|
| Créer module C++ SpaceLeague | Critique | LEAG-005 |
| Créer ASpaceLeagueGameMode | Critique | LEAG-006 |
| Créer ASpaceLeagueGameState | Critique | LEAG-007 |
| Créer ASpaceLeaguePlayerController | Haute | LEAG-008 |
| Créer ASpaceLeagueCharacterBase | Critique | LEAG-009 |

### 0.3 Migration C++ - Phase 2

| Tâche | Priorité | Ticket |
|-------|----------|--------|
| Reparenter BP_Paradoxe | Critique | LEAG-010 |
| Créer ABall C++ | Haute | LEAG-011 |
| Reparenter BP_Ball | Haute | LEAG-012 |
| Créer AGoal C++ | Moyenne | LEAG-013 |
| Créer APrison C++ | Moyenne | LEAG-014 |

### 0.4 Corrections Techniques

| Tâche | Priorité | Ticket |
|-------|----------|--------|
| Implémenter GI_SpaceLeague fonctions | Haute | LEAG-015 |
| Configurer Map principale | Haute | LEAG-016 |
| Vérifier DA_LegendRegistry | Moyenne | LEAG-017 |

### Critères de Validation Phase 0
- [ ] Projet compile sans erreur (C++ et Blueprint)
- [ ] Jeu lance sans crash
- [ ] Mouvements de base fonctionnent
- [ ] Balle physique fonctionne
- [ ] Score s'affiche

---

## PHASE 1 : Core Game Testable

**Objectif:** 2 personnages jouables avec événements terrain
**Durée estimée:** 3-4 semaines
**Prérequis:** Phase 0 complète

### 1.1 Personnages de Test

| Personnage | Type | État | Ticket |
|------------|------|------|--------|
| Raijin | Foudre | DA + BP existants | LEAG-018 |
| Keplar | Galaxie | DA + BP existants | LEAG-019 |

**Tâches:**
| Tâche | Ticket |
|-------|--------|
| Valider DA_LegendBase_Raijin | LEAG-020 |
| Valider DA_LegendBase_Keplar | LEAG-021 |
| Différencier stats Raijin/Keplar | LEAG-022 |

### 1.2 Événements Terrain

| Événement | Priorité | Ticket |
|-----------|----------|--------|
| Rotation terrain (90/180/360) | Haute | LEAG-023 |
| Créer ATerrainEventManager | Haute | LEAG-024 |
| Déplacement des buts | Moyenne | LEAG-025 |
| Phase Attaque+ | Basse | LEAG-026 |
| Phase Défense+ | Basse | LEAG-027 |

### 1.3 Gameplay Core

| Tâche | Ticket |
|-------|--------|
| Valider système prison complet | LEAG-028 |
| Valider scoring (prison + buts) | LEAG-029 |
| Timer balle inactive | LEAG-030 |
| UI Score en jeu | LEAG-031 |

### Critères de Validation Phase 1
- [ ] Match 1v1 local jouable
- [ ] 2 personnages avec stats différentes
- [ ] Rotation terrain fonctionne
- [ ] Score correct (2pts prison, 1pt but)
- [ ] Prison fonctionnelle

---

## PHASE 2 : Multijoueur

**Objectif:** Local + Online fonctionnels
**Durée estimée:** 4-6 semaines
**Prérequis:** Phase 1 complète

### 2.1 Multijoueur Local

| Tâche | Ticket |
|-------|--------|
| Split-screen 2 joueurs | LEAG-032 |
| Split-screen 4 joueurs | LEAG-033 |
| Gestion multi-contrôleurs | LEAG-034 |
| Attribution équipes local | LEAG-035 |

### 2.2 Réplication Réseau

| Tâche | Priorité | Ticket |
|-------|----------|--------|
| Répliquer ABall | Critique | LEAG-036 |
| Répliquer ASpaceLeagueCharacterBase | Critique | LEAG-037 |
| Répliquer ASpaceLeagueGameState | Critique | LEAG-038 |
| Répliquer événements terrain | Haute | LEAG-039 |

### 2.3 Infrastructure Online

| Tâche | Ticket |
|-------|--------|
| Sessions UE5 | LEAG-040 |
| Lobby création/rejoindre | LEAG-041 |
| UI Sélection personnage | LEAG-042 |
| Matchmaking basique | LEAG-043 |

### Critères de Validation Phase 2
- [ ] 2v2 local fonctionne
- [ ] 2v2 online fonctionne (LAN)
- [ ] Pas de desync notable
- [ ] Lobby fonctionnel

---

## PHASE 3 : Extension

**Objectif:** 4 personnages + capacités
**Durée estimée:** 6-8 semaines
**Prérequis:** Phase 2 complète

### 3.1 Personnages Supplémentaires

| Personnage | Type | Rôle | Ticket |
|------------|------|------|--------|
| Chronis | Temps | Sentinelle | LEAG-044 |
| Aurion | Lumière | Pilier | LEAG-045 |

### 3.2 Système de Capacités

| Tâche | Ticket |
|-------|--------|
| Créer UAbilityComponent | LEAG-046 |
| Système Passif | LEAG-047 |
| Système Sort (avec CD) | LEAG-048 |
| Système Ultime (avec CD) | LEAG-049 |
| UI Capacités | LEAG-050 |

### 3.3 Capacités Raijin

| Capacité | Type | Ticket |
|----------|------|--------|
| Battement du Tonnerre | Passif | LEAG-051 |
| Décharge Fulgurante | Sort | LEAG-052 |
| Colère de Raijin | Ultime | LEAG-053 |

### 3.4 Capacités Keplar

| Capacité | Type | Ticket |
|----------|------|--------|
| (À définir) | Passif | LEAG-054 |
| (À définir) | Sort | LEAG-055 |
| (À définir) | Ultime | LEAG-056 |

### Critères de Validation Phase 3
- [ ] 4 personnages jouables
- [ ] Capacités fonctionnelles
- [ ] Cooldowns corrects
- [ ] UI capacités claire

---

## Backlog (Post-v1.0)

| Feature | Priorité |
|---------|----------|
| Personnages 5-9 | Haute |
| Gravity Flip | Moyenne |
| Phase Chaos Léger | Basse |
| Crossplay | Moyenne |
| Ranking compétitif | Moyenne |
| Système de déblocage | Basse |
| Skins supplémentaires | Basse |
| Maps thématiques | Moyenne |

---

## Changelog

| Date | Version | Changements |
|------|---------|-------------|
| 2026-01-25 | 1.0 | Création initiale |


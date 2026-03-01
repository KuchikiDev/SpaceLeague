---
type: guide
status: active
last_review: 2026-02-19
owner: dylan
audience: humans-agents
---

# Guide des Skills UE5 Custom

**Version:** 1.0
**Date:** 2026-02-19
**Projet:** Space League - MovementParadoxe

**Navigation Vault:** [[_ai/00_Home]] | [[_ai/01_Now]] | [[_ai/AGENT_CONTEXT]]
**Liens relies:** [[README_SETUP_BMAD_UE5]] | [[WORKFLOW_PHASE0_BMAD]] | [[ROADMAP]]

---

## Vue d'Ensemble

Ce guide documente tous les **skills custom UE5** créés pour faciliter le développement du projet Space League.

### Skills Disponibles

| Skill | Description | Usage Typique |
|-------|-------------|---------------|
| `/ue5-compile` | Compile le projet C++/BP | Après changement code C++ |
| `/ue5-test` | Lance tests automatisés | Validation après dev |
| `/ue5-backup-asset` | Backup asset avant modif | **AVANT migration BP** |
| `/migration-check` | Vérifie migration BP→C++ | Après reparentage BP |
| `/ue5-health-check` | Diagnostic santé projet | Daily check / Avant commit |
| `/ue5-create-module` | Crée module C++ | Début Phase 0 (LEAG-005) |

---

## Quick Start Guide

### Workflow Phase 0 - Setup Initial

```bash
# 1. Créer le module SpaceLeague
/ue5-create-module SpaceLeague --type Runtime

# 2. Compiler pour vérifier
/ue5-compile

# 3. Check santé initiale
/ue5-health-check --report
```

### Workflow Phase 0 - Migration d'un Blueprint

```bash
# ÉTAPE 1: Backup OBLIGATOIRE
/ue5-backup-asset Content/Legends/BP_Paradoxe.uasset

# ÉTAPE 2: Migration manuelle dans UE5
# - Ouvrir BP_Paradoxe
# - File > Reparent Blueprint
# - Choisir ASpaceLeagueCharacterBase

# ÉTAPE 3: Vérifier la migration
/migration-check BP_Paradoxe --parent ASpaceLeagueCharacterBase

# ÉTAPE 4: Compiler
/ue5-compile

# ÉTAPE 5: Tester
/ue5-test --filter Character

# ÉTAPE 6: Health check
/ue5-health-check

# ÉTAPE 7: Commit si OK
git add .
git commit -m "Migré BP_Paradoxe vers ASpaceLeagueCharacterBase"
```

### Workflow Quotidien

```bash
# Matin: Check santé
/ue5-health-check

# Développement...
# (éditer code C++)

# Compilation
/ue5-compile

# Tests
/ue5-test

# Soir: Check final avant commit
/ue5-health-check
git commit -m "..."
```

---

## Détails des Skills

### 1. `/ue5-compile`

**Compile le projet UE5 (C++ et Blueprints)**

```bash
# Compilation standard
/ue5-compile

# Compilation rapide (skip hot reload)
/ue5-compile --fast

# Clean + rebuild
/ue5-compile --clean
```

**Quand utiliser:**
- Après modification de code C++
- Avant de lancer UE5 Editor
- Dans le workflow CI/CD

**Output:**
```
✅ Compilation terminée avec succès!
⏱️  Durée: 2m 34s
⚠️  3 warnings (non-bloquants)
```

---

### 2. `/ue5-test`

**Lance les tests automatisés UE5**

```bash
# Tous les tests
/ue5-test

# Tests spécifiques (filter)
/ue5-test --filter Movement
/ue5-test --filter Ball
/ue5-test --filter Score

# Par type
/ue5-test --type unit
/ue5-test --type functional
/ue5-test --type gauntlet

# Verbose (logs détaillés)
/ue5-test --verbose
```

**Types de tests:**
- **Unit Tests** : Classes C++ isolées (rapide ~10-30s)
- **Functional Tests** : Gameplay en conditions réelles (~1-3 min)
- **Gauntlet Tests** : Automation complète (~5-15 min)

**Quand utiliser:**
- Après implémentation d'une feature
- Avant commit
- Dans le workflow CI/CD
- Validation Phase (ex: Phase 0 completion)

**Output:**
```
📊 Résultats:
   ✅ Passed: 12
   ❌ Failed: 1
   ⚠️  Skipped: 2
   ⏱️  Durée: 12.4s
```

---

### 3. `/ue5-backup-asset`

**Crée un backup d'un asset UE5 avant modification**

```bash
# Backup simple
/ue5-backup-asset Content/Legends/BP_Paradoxe.uasset

# Avec description
/ue5-backup-asset Content/Legends/BP_Paradoxe.uasset --description "Avant reparentage vers C++"

# Exemples Phase 0
/ue5-backup-asset Content/Legends/BP_Paradoxe.uasset
/ue5-backup-asset Content/Terrain/Blueprints/BP_Ball.uasset
/ue5-backup-asset Content/Game/Gameplay/Gamemodes/BP_GameMode_SpaceLeague.uasset
```

**Quand utiliser:**
- **TOUJOURS avant reparenter un Blueprint**
- Avant supprimer un asset
- Avant refactoring majeur
- Avant modifier une classe de base

**⚠️ CRITIQUE:**
- Ne JAMAIS migrer un BP sans backup
- Les backups sont dans `.backups/assets/`
- Conservés 30 jours par défaut

**Output:**
```
💾 Backup créé: .backups/assets/BP_Paradoxe_20260219_143522.uasset
📄 Métadonnées: .backups/assets/BP_Paradoxe_20260219_143522.json
🔒 Hash MD5: a3f5c8d9e2b1f4a6c7d8e9f0a1b2c3d4
```

**Restaurer un backup (manuel):**
```bash
# 1) Fermer UE5 Editor
# 2) Copier le .uasset de .backups/assets/ vers le chemin d'origine dans Content/
# 3) Relancer UE5
# 4) Recompiler
/ue5-compile
```

---

### 4. `/migration-check`

**Vérifie qu'une migration Blueprint → C++ est complète**

```bash
# Check basique
/migration-check BP_Paradoxe

# Avec parent spécifique
/migration-check BP_Paradoxe --parent ASpaceLeagueCharacterBase

# Mode strict (zéro warning)
/migration-check BP_Paradoxe --strict
```

**Vérifications effectuées:**
1. ✅ Blueprint hérite de la bonne classe C++
2. ✅ Pas de logique dupliquée (C++ vs BP)
3. ✅ Variables migrées vers C++
4. ✅ Fonctions migrées vers C++
5. ✅ Réplication configurée
6. ✅ Références externes fonctionnent

**Quand utiliser:**
- Immédiatement après reparentage BP
- Avant commit migration
- Dans QA validation

**Output:**
```
📊 Résumé
✅ Checks réussis: 18
⚠️  Warnings: 3
❌ Erreurs: 0

Statut: ✅ MIGRATION VALIDE (avec warnings mineurs)
Recommandation: ✅ OK pour commit
```

**Migrations Phase 0:**
```bash
# Vérifier toutes les migrations
/migration-check BP_Paradoxe --parent ASpaceLeagueCharacterBase
/migration-check BP_Ball --parent ABall
/migration-check BP_Goal --parent AGoal
/migration-check BP_Prison --parent APrison
```

---

### 5. `/ue5-health-check`

**Diagnostic complet de la santé du projet**

```bash
# Check standard
/ue5-health-check

# Avec auto-fix
/ue5-health-check --fix

# Générer rapport détaillé
/ue5-health-check --report

# Check spécifique Phase 0
/ue5-health-check --phase0
```

**Métriques vérifiées:**
1. **Compilation & Build** (erreurs, warnings, temps)
2. **Assets & Contenu** (cassés, doublons, non utilisés)
3. **Code Quality** (deprecated, code smell, complexité)
4. **Performance** (FPS, mémoire, chargement)
5. **Réplication** (variables, RPCs, bandwidth) - Phase 2
6. **Tests** (couverture, succès/échec)

**Quand utiliser:**
- **Daily check** (matin avant dev)
- Avant commit
- Avant merge/PR
- Validation fin de phase

**Output:**
```
Score Santé: 82/100 (🟡 GOOD)
   - Compilation: 95/100 🟢
   - Assets: 75/100 🟡
   - Code Quality: 80/100 🟡
   - Performance: 90/100 🟢
   - Tests: 70/100 🟡

🔧 ACTIONS RECOMMANDÉES
1. ❌ LEAG-004: Résoudre doublon GI_SpaceLeague
2. ⚠️  Nettoyer warnings compilation (12 warnings)
3. ⚠️  Supprimer assets non utilisés (23 MB)
```

**Seuils de Santé:**
| Métrique | 🟢 Good | 🟡 Moderate | 🔴 Critical |
|----------|---------|-------------|-------------|
| FPS | >60 fps | 30-60 fps | <30 fps |
| Compilation | 0 erreurs | 1-5 warnings | >5 warnings |
| Tests | >80% pass | 60-80% | <60% |

---

### 6. `/ue5-create-module`

**Crée un nouveau module C++ pour UE5**

```bash
# Module Runtime (gameplay)
/ue5-create-module SpaceLeague --type Runtime

# Module Editor (outils custom)
/ue5-create-module SpaceLeagueEditor --type Editor

# Avec dépendances custom
/ue5-create-module SpaceLeague --type Runtime --deps "GameplayTags,UMG,Slate"
```

**Quand utiliser:**
- Début Phase 0 (LEAG-005)
- Création module UI/Editor
- Organisation multi-modules

**Fichiers générés:**
```
Source/SpaceLeague/
├── SpaceLeague.Build.cs
├── Public/
│   └── SpaceLeague.h
└── Private/
    └── SpaceLeague.cpp
```

**Après création:**
1. Compiler: `/ue5-compile`
2. Créer classes selon SPECS_TECHNIQUES.md
3. Tester: `/ue5-test`

---

## Workflows Complets

### Workflow 1: Nettoyage Projet (LEAG-001 à 004)

```bash
# Avant de supprimer des assets
/ue5-health-check

# Supprimer BP_ParadoxeJhin (LEAG-001)
# - Vérifier références d'abord
/ue5-health-check --report

# Supprimer dans UE5 Editor
# Content Browser > Delete

# Vérifier santé après
/ue5-health-check

# Compiler
/ue5-compile

# Commit
git commit -m "LEAG-001: Supprimé BP_ParadoxeJhin"
```

### Workflow 2: Créer Module C++ (LEAG-005)

```bash
# Créer module
/ue5-create-module SpaceLeague --type Runtime

# Vérifier structure
ls Source/SpaceLeague/

# Compiler
/ue5-compile

# Health check
/ue5-health-check

# Commit
git commit -m "LEAG-005: Créé module C++ SpaceLeague"
```

### Workflow 3: Migration BP_Paradoxe (LEAG-010)

```bash
# 1. Backup OBLIGATOIRE
/ue5-backup-asset Content/Legends/BP_Paradoxe.uasset

# 2. Migration dans UE5
# File > Reparent Blueprint > ASpaceLeagueCharacterBase

# 3. Vérifier migration
/migration-check BP_Paradoxe --parent ASpaceLeagueCharacterBase

# 4. Compiler
/ue5-compile

# 5. Tester
/ue5-test --filter Character

# 6. Health check
/ue5-health-check

# 7. Commit si tout OK
git commit -m "LEAG-010: Migré BP_Paradoxe vers ASpaceLeagueCharacterBase"
```

### Workflow 4: Validation Phase 0 Complete

```bash
# Health check Phase 0
/ue5-health-check --phase0

# Tests Phase 0
/ue5-test --filter Phase0

# QA Gate
/qa-gate

# Générer rapport
/ue5-health-check --report

# Si tout est ✅
git commit -m "Phase 0 Complete"
git tag v0.1.0-phase0-complete
```

---

## Intégration avec BMad Agents

### Avec `/architect`

```bash
# Valider architecture avant migration
/architect

# Demander:
# - Analyser Documentation/ARCHITECTURE.md
# - Valider structure C++ proposée
# - Confirmer stratégie réplication

# Compiler après validation
/ue5-compile
```

### Avec `/dev`

```bash
# Développement avec tests continus
/dev

# Développer feature...

# Tests automatiques
/ue5-test --filter <FeatureName>

# Health check
/ue5-health-check
```

### Avec `/qa`

```bash
# QA validation
/qa

# Tests complets
/ue5-test

# Migration check
/migration-check <Blueprint>

# Health check
/ue5-health-check

# QA Gate
/qa-gate
```

---

## Troubleshooting

### Problème: "Module SpaceLeague not found"

**Solution:**
```bash
# Vérifier existence
ls Source/SpaceLeague/SpaceLeague.Build.cs

# Régénérer .sln
/ue5-create-module SpaceLeague --regenerate

# Recompiler
/ue5-compile --clean
```

### Problème: "Tests failed"

**Solution:**
```bash
# Tests verbeux pour voir détails
/ue5-test --verbose

# Health check pour identifier problème
/ue5-health-check

# Compiler à nouveau
/ue5-compile
```

### Problème: "Migration failed"

**Solution:**
```bash
# Restaurer backup (manuel)
# 1) Fermer UE5
# 2) Copier le .uasset depuis .backups/assets/ vers Content/
# 3) Relancer UE5

# Analyser le problème
/migration-check <Blueprint> --strict

# Réessayer migration
```

### Problème: "Health score < 70"

**Solution:**
```bash
# Auto-fix
/ue5-health-check --fix

# Rapport détaillé
/ue5-health-check --report

# Corriger manuellement les erreurs critiques
```

---

## Ressources Associées

### Documentation
- [WORKFLOW_PHASE0_BMAD.md](./WORKFLOW_PHASE0_BMAD.md) - Workflow détaillé Phase 0
- [ARCHITECTURE.md](./ARCHITECTURE.md) - Architecture C++/Blueprint
- [SPECS_TECHNIQUES.md](./SPECS_TECHNIQUES.md) - Spécifications C++
- [ROADMAP.md](./ROADMAP.md) - Roadmap complète

### BMad Agents
- `/architect` - Validation architecture
- `/dev` - Développement
- `/qa` - Tests et validation
- `/pm` - Gestion projet

### BMad Tasks
- `/test-design` - Créer plans de test
- `/risk-profile` - Analyser risques
- `/qa-gate` - Gates de qualité
- `/document-project` - Documentation

---

## Checklist Phase 0 (avec Skills)

### 0.1 Nettoyage Projet

- [ ] `/ue5-health-check` - État initial
- [ ] Supprimer BP_ParadoxeJhin (LEAG-001)
- [ ] Supprimer BP_ParadoxeSenna (LEAG-002)
- [ ] Supprimer dossier /Old/ (LEAG-003)
- [ ] Résoudre doublon GI_SpaceLeague (LEAG-004)
- [ ] `/ue5-compile` - Vérifier compilation
- [ ] `/ue5-health-check` - Vérifier amélioration

### 0.2 Migration C++ - Phase 1

- [ ] `/ue5-create-module SpaceLeague` (LEAG-005)
- [ ] `/ue5-compile` - Vérifier module
- [ ] Créer ASpaceLeagueGameMode (LEAG-006)
- [ ] Créer ASpaceLeagueGameState (LEAG-007)
- [ ] Créer ASpaceLeaguePlayerController (LEAG-008)
- [ ] Créer ASpaceLeagueCharacterBase (LEAG-009)
- [ ] `/ue5-compile` - Compiler toutes les classes
- [ ] `/ue5-test --filter Core` - Tester classes
- [ ] `/ue5-health-check` - Vérifier qualité

### 0.3 Migration C++ - Phase 2

- [ ] `/ue5-backup-asset BP_Paradoxe` (LEAG-010)
- [ ] Reparenter BP_Paradoxe
- [ ] `/migration-check BP_Paradoxe` (LEAG-010)
- [ ] Créer ABall (LEAG-011)
- [ ] `/ue5-backup-asset BP_Ball` (LEAG-012)
- [ ] Reparenter BP_Ball
- [ ] `/migration-check BP_Ball` (LEAG-012)
- [ ] Créer AGoal (LEAG-013)
- [ ] Créer APrison (LEAG-014)
- [ ] `/ue5-compile` - Compiler tout
- [ ] `/ue5-test --filter Gameplay` - Tester gameplay
- [ ] `/ue5-health-check` - Vérifier qualité

### 0.4 Validation Finale

- [ ] `/ue5-compile` - Compilation finale
- [ ] `/ue5-test` - Tous les tests
- [ ] `/ue5-health-check --phase0` - Validation Phase 0
- [ ] `/qa-gate` - Gate QA
- [ ] `/ue5-health-check --report` - Rapport final
- [ ] Commit final Phase 0

---

**Dernière mise à jour:** 2026-02-19
**Auteur:** Claude (BMad)
**Version:** 1.0


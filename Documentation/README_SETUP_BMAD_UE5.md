---
type: setup
status: active
last_review: 2026-02-19
owner: dylan
audience: humans-agents
---

# 🚀 Setup BMad + UE5 Skills - Space League

**Date:** 2026-02-19
**Projet:** Space League - MovementParadoxe
**Phase:** Phase 0 - Fondations

**Navigation Vault:** [[_ai/00_Home]] | [[_ai/01_Now]] | [[_ai/AGENT_CONTEXT]]
**Liens relies:** [[GUIDE_SKILLS_UE5]] | [[WORKFLOW_PHASE0_BMAD]] | [[ROADMAP]]

---

## 📦 Ce qui a été créé

### 1. Workflow Phase 0 Détaillé

**Fichier:** [[WORKFLOW_PHASE0_BMAD]]

Un workflow complet étape par étape pour votre Phase 0 avec:
- 7 étapes détaillées (Préparation → Validation → Commit)
- Intégration des agents BMad (/architect, /dev, /qa, etc.)
- Timeline de 13 jours
- Points de contrôle critiques
- Critères de validation

### 2. Skills Custom UE5

**Dossier:** `.claude/commands/UE5/`

6 skills créés spécifiquement pour votre projet UE5:

| Skill | Fichier | Description |
|-------|---------|-------------|
| `/ue5-compile` | ue5-compile.md | Compile le projet C++/BP |
| `/ue5-test` | ue5-test.md | Lance tests automatisés |
| `/ue5-backup-asset` | ue5-backup-asset.md | Backup asset avant migration |
| `/migration-check` | migration-check.md | Vérifie migration BP→C++ |
| `/ue5-health-check` | ue5-health-check.md | Diagnostic santé projet |
| `/ue5-create-module` | ue5-create-module.md | Crée module C++ |

### 3. Guide de Référence

**Fichier:** [[GUIDE_SKILLS_UE5]]

Guide complet avec:
- Quick Start pour chaque skill
- Workflows complets (migration, tests, validation)
- Intégration avec agents BMad
- Troubleshooting
- Checklist Phase 0 interactive

---

## 🎯 Par où commencer?

### Étape 1: Lire la Documentation

```bash
# Ordre de lecture recommandé:
1. GUIDE_SKILLS_UE5.md           # Guide des skills (ce fichier)
2. WORKFLOW_PHASE0_BMAD.md       # Workflow détaillé Phase 0
3. ROADMAP.md                    # Vue d'ensemble des phases
4. ARCHITECTURE.md               # Architecture C++/Blueprint
5. SPECS_TECHNIQUES.md           # Code C++ à implémenter
```

### Étape 2: Tester les Skills

```bash
# Test 1: Health Check initial
/ue5-health-check

# Test 2: Compilation (si module existe déjà)
/ue5-compile

# Test 3: Tests (si tests existent)
/ue5-test
```

### Étape 3: Lancer le Workflow Phase 0

Suivre exactement [[WORKFLOW_PHASE0_BMAD]] etape par etape.

---

## 📚 Structure de la Documentation

```
Documentation/
├── README_SETUP_BMAD_UE5.md        # ← Ce fichier (point d'entrée)
├── GUIDE_SKILLS_UE5.md             # Guide des skills UE5
├── WORKFLOW_PHASE0_BMAD.md         # Workflow Phase 0 détaillé
├── ROADMAP.md                      # Roadmap complète 4 phases
├── ARCHITECTURE.md                 # Architecture C++/Blueprint
├── SPECS_TECHNIQUES.md             # Spécifications C++
├── SPECS_FONCTIONNELLES.md         # Specs gameplay
├── IMPACT_MIGRATION.md             # Analyse risques migration
├── RESUME_JEU.md                   # Résumé du concept
└── _INDEX_DOCUMENTATION.md         # Index de la doc

.claude/commands/UE5/
├── ue5-compile.md                  # Skill compilation
├── ue5-test.md                     # Skill tests
├── ue5-backup-asset.md             # Skill backup
├── migration-check.md              # Skill check migration
├── ue5-health-check.md             # Skill health check
└── ue5-create-module.md            # Skill création module
```

---

## 🔄 Workflow Rapide Phase 0

### Semaine 1: Préparation + Nettoyage

**Jour 1: Préparation**
```bash
# Valider architecture
/architect
# Lui demander d'analyser Documentation/ARCHITECTURE.md

# Analyser risques
/risk-profile
# Analyser IMPACT_MIGRATION.md

# Créer plan de test
/test-design

# Health check initial
/ue5-health-check --report
```

**Jour 2: Nettoyage (LEAG-001 à 004)**
```bash
# Pour chaque asset à supprimer:
# 1. Vérifier références
# 2. Supprimer dans UE5
# 3. Compiler
/ue5-compile

# 4. Health check
/ue5-health-check
```

### Semaine 2: Migration C++ Phase 1

**Jours 3-5: Classes Framework (LEAG-005 à 009)**
```bash
# Créer module
/ue5-create-module SpaceLeague --type Runtime

# Pour chaque classe:
/dev
# Implémenter selon SPECS_TECHNIQUES.md

/ue5-compile
/ue5-test --filter <ClassName>
/ue5-health-check
```

**Jours 6-8: Migration C++ Phase 2 (LEAG-010 à 014)**
```bash
# Pour chaque Blueprint:
/ue5-backup-asset Content/Path/To/Blueprint.uasset
# Migration manuelle dans UE5
/migration-check <BlueprintName> --parent <CPPClass>
/ue5-compile
/ue5-test
/ue5-health-check
```

### Semaine 3: Corrections + Validation

**Jours 9-10: Corrections (LEAG-015 à 017)**
```bash
/dev
# Implémenter corrections

/ue5-compile
/ue5-test
/ue5-health-check
```

**Jours 11-12: Validation Phase 0**
```bash
# QA Gate
/qa-gate

# Health check complet
/ue5-health-check --phase0 --report

# Tous les tests
/ue5-test

# Si ✅ → Commit
git commit -m "Phase 0 Complete"
```

---

## 🎮 Skills UE5 - Quick Reference

### Compilation

```bash
/ue5-compile                # Standard
/ue5-compile --fast         # Rapide
/ue5-compile --clean        # Clean build
```

### Tests

```bash
/ue5-test                           # Tous
/ue5-test --filter Movement         # Filtrés
/ue5-test --type unit               # Par type
/ue5-test --type functional
/ue5-test --type gauntlet
```

### Migration Blueprint

```bash
# 1. BACKUP
/ue5-backup-asset Content/Legends/BP_Paradoxe.uasset

# 2. Migration manuelle (UE5 Editor)

# 3. CHECK
/migration-check BP_Paradoxe --parent ASpaceLeagueCharacterBase

# 4. COMPILE
/ue5-compile

# 5. TEST
/ue5-test --filter Character

# 6. COMMIT
git commit -m "Migré BP_Paradoxe"
```

### Health Check

```bash
/ue5-health-check           # Standard
/ue5-health-check --fix     # Auto-fix
/ue5-health-check --report  # Rapport détaillé
/ue5-health-check --phase0  # Check Phase 0
```

### Création Module

```bash
/ue5-create-module SpaceLeague --type Runtime
```

---

## 🔧 Intégration avec Agents BMad

### Agents Disponibles

| Agent | Commande | Usage Phase 0 |
|-------|----------|---------------|
| **Architect** | `/architect` | Valider architecture C++ |
| **Dev** | `/dev` | Implémenter classes/migrations |
| **QA** | `/qa` | Tester migrations/fonctionnalités |
| **PM** | `/pm` | Gérer tickets LEAG-xxx |
| **PO** | `/po` | Prioriser features |

### Tasks Disponibles

| Task | Commande | Usage Phase 0 |
|------|----------|---------------|
| **Test Design** | `/test-design` | Créer plans de test |
| **Risk Profile** | `/risk-profile` | Analyser risques migration |
| **QA Gate** | `/qa-gate` | Validation fin de phase |
| **NFR Assess** | `/nfr-assess` | Exigences non-fonctionnelles |
| **Document Project** | `/document-project` | MAJ documentation |

### Workflow Typique avec BMad

```bash
# 1. PRÉPARATION (Agents)
/architect          # Valider architecture
/risk-profile       # Identifier risques
/test-design        # Créer plans de test

# 2. DÉVELOPPEMENT (Dev + Skills UE5)
/dev                # Implémenter
/ue5-compile        # Compiler
/ue5-test           # Tester

# 3. VALIDATION (QA + Skills UE5)
/qa                 # Tests manuels
/migration-check    # Check migrations
/ue5-health-check   # Diagnostic
/qa-gate            # Gate final

# 4. DOCUMENTATION
/document-project   # MAJ docs
```

---

## 📋 Checklist Avant de Commencer

### Prérequis Techniques

- [ ] Unreal Engine 5.3+ installé
- [ ] Visual Studio 2022 installé
- [ ] Git configuré
- [ ] Projet MovementParadoxe clone localement
- [ ] BMad framework installé (.bmad-core/)
- [ ] Skills UE5 installés (.claude/commands/UE5/)

### Prérequis Documentation

- [ ] Lu ROADMAP.md (vue d'ensemble)
- [ ] Lu ARCHITECTURE.md (structure C++)
- [ ] Lu SPECS_TECHNIQUES.md (code à écrire)
- [ ] Lu WORKFLOW_PHASE0_BMAD.md (workflow détaillé)
- [ ] Lu GUIDE_SKILLS_UE5.md (skills UE5)

### Prérequis Connaissance

- [ ] Comprend architecture C++/Blueprint hybride
- [ ] Connait les bases UE5 (GameMode, GameState, etc.)
- [ ] Sait reparenter un Blueprint
- [ ] Comprend la réplication réseau (Phase 2)

---

## 🎯 Premiers Pas (Setup Initial)

### 1. Vérifier l'Installation

```bash
# Vérifier que les skills sont disponibles
ls .claude/commands/UE5/

# Devrait afficher:
# - ue5-compile.md
# - ue5-test.md
# - ue5-backup-asset.md
# - migration-check.md
# - ue5-health-check.md
# - ue5-create-module.md
```

### 2. Health Check Initial

```bash
/ue5-health-check --report

# Analyser le rapport:
# - Score actuel?
# - Erreurs critiques?
# - Warnings?
# - Actions recommandées?
```

### 3. Lire le Workflow

```bash
# Ouvrir dans l'éditeur
code Documentation/WORKFLOW_PHASE0_BMAD.md

# Lire entièrement avant de commencer
# Noter les points critiques (🔴 STOP Points)
```

### 4. Démarrer Phase 0 - Étape 1

```bash
# Suivre exactement WORKFLOW_PHASE0_BMAD.md
# Étape 1.1: Validation Architecture

/architect

# Lui demander:
# - Analyser Documentation/ARCHITECTURE.md
# - Valider SPECS_TECHNIQUES.md
# - Confirmer stratégie réplication
```

---

## 🆘 Aide et Support

### Questions Fréquentes

**Q: Par où commencer exactement?**
A: Lire [GUIDE_SKILLS_UE5.md](./GUIDE_SKILLS_UE5.md) puis suivre [WORKFLOW_PHASE0_BMAD.md](./WORKFLOW_PHASE0_BMAD.md)

**Q: Les skills UE5 fonctionnent comment?**
A: Ce sont des templates pour l'agent Claude. Quand vous tapez `/ue5-compile`, l'agent reçoit les instructions du fichier `ue5-compile.md` et exécute les actions.

**Q: Dois-je implémenter les skills moi-même?**
A: Non, les skills sont déjà créés. Claude les utilise automatiquement quand vous tapez `/ue5-xxx`.

**Q: Que faire si une migration échoue?**
A:
1. Restaurer manuellement le `.uasset` depuis `.backups/assets/` vers `Content/`
2. Analyser avec `/migration-check --strict`
3. Corriger les erreurs
4. Réessayer

**Q: Comment savoir si Phase 0 est terminée?**
A: Lancer `/ue5-health-check --phase0` et `/qa-gate`. Si tous les critères sont ✅, c'est bon.

### Commandes d'Urgence

```bash
# Projet ne compile plus
/ue5-compile --clean

# Tests échouent
/ue5-test --verbose

# Projet instable
/ue5-health-check --fix

# Migration a cassé quelque chose
# Restaurer manuellement depuis .backups/assets/ puis recompiler
/ue5-compile
```

---

## 📊 Métriques de Succès Phase 0

### Critères Techniques

- [ ] ✅ Projet compile sans erreur (C++ et Blueprint)
- [ ] ✅ Jeu lance sans crash
- [ ] ✅ Mouvements de base fonctionnent
- [ ] ✅ Balle physique fonctionne
- [ ] ✅ Score s'affiche
- [ ] ✅ Health Score ≥ 75/100

### Critères Migration

- [ ] ✅ Module SpaceLeague créé
- [ ] ✅ Classes C++ framework créées (5 classes)
- [ ] ✅ BP_Paradoxe migré vers C++
- [ ] ✅ BP_Ball migré vers C++
- [ ] ✅ BP_Goal et BP_Prison créés
- [ ] ✅ Tous les migration-check ✅

### Critères Documentation

- [ ] ✅ ARCHITECTURE.md à jour
- [ ] ✅ ROADMAP.md Phase 0 marquée complète
- [ ] ✅ PHASE0_RAPPORT.md créé
- [ ] ✅ Commits Git propres

---

## 🚀 Prochaines Étapes (Après Phase 0)

### Phase 1: Core Game

Une fois Phase 0 terminée:

1. **Valider transition**
   ```bash
   /ue5-health-check --report
   git tag v0.1.0-phase0-complete
   ```

2. **Préparer Phase 1**
   ```bash
   /pm
   # Créer tickets LEAG-018 à LEAG-031
   # Planifier personnages (Raijin/Keplar)
   ```

3. **Lire workflow Phase 1** (à créer)
   - Implémenter personnages
   - Événements terrain
   - Tests gameplay

---

## 📞 Ressources

### Documentation Projet

- [ROADMAP.md](./ROADMAP.md) - Roadmap complète
- [ARCHITECTURE.md](./ARCHITECTURE.md) - Architecture technique
- [SPECS_TECHNIQUES.md](./SPECS_TECHNIQUES.md) - Code C++
- [SPECS_FONCTIONNELLES.md](./SPECS_FONCTIONNELLES.md) - Gameplay
- [WORKFLOW_PHASE0_BMAD.md](./WORKFLOW_PHASE0_BMAD.md) - Workflow Phase 0
- [GUIDE_SKILLS_UE5.md](./GUIDE_SKILLS_UE5.md) - Guide skills

### BMad Framework

- [User Guide](../.bmad-core/user-guide.md) - Guide BMad
- [Enhanced IDE Workflow](../.bmad-core/enhanced-ide-development-workflow.md)
- [Working in Brownfield](../.bmad-core/working-in-the-brownfield.md)

### Confluence

- [ROADMAP Space League](https://byakuyacorps.atlassian.net/wiki/spaces/SL/pages/212795394)
- [ARCHITECTURE TECHNIQUE](https://byakuyacorps.atlassian.net/wiki/spaces/SL/pages/212828161)

---

## ✅ Ready to Start!

Vous avez maintenant tout pour démarrer:

1. ✅ **Workflow détaillé** Phase 0 (13 jours)
2. ✅ **6 Skills UE5** custom pour votre projet
3. ✅ **Guide complet** d'utilisation
4. ✅ **Intégration BMad** (agents + tasks)
5. ✅ **Documentation** complète

**Prochaine action:** Ouvrir [WORKFLOW_PHASE0_BMAD.md](./WORKFLOW_PHASE0_BMAD.md) et commencer Étape 1!

---

**Créé le:** 2026-02-19
**Par:** Claude (BMad Orchestrator)
**Version:** 1.0
**Statut:** ✅ Ready to Use

**Bonne chance pour votre Phase 0!** 🚀


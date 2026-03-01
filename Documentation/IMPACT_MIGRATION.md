---
type: migration
status: planned
last_review: 2026-02-19
owner: dylan
audience: humans-agents
---

# Analyse d'Impact - Migration C++ Hybride

**Version:** 1.0
**Date:** 2026-01-25
**Auteur:** Claude (BMAD)

**Navigation Vault:** [[_ai/00_Home]] | [[_ai/01_Now]] | [[_ai/AGENT_CONTEXT]]
**Liens relies:** [[ARCHITECTURE]] | [[SPECS_TECHNIQUES]] | [[WORKFLOW_PHASE0_BMAD]] | [[_maps/MOC_Migration]]

---

## Objectif

Migrer vers une architecture hybride C++ / Blueprint **SANS CASSER** le gameplay existant.

---

## Principe de Migration Non-Destructive

```
AVANT                           APRÈS
──────                          ─────
BP_Paradoxe                     ASpaceLeagueCharacterBase (C++)
  └── (Character)                   │
                                    ▼
                                BP_Paradoxe (hérite C++)
                                  └── Conserve toute sa logique

BP_Ball                         ABall (C++)
  └── (Actor)                       │
                                    ▼
                                BP_Ball (hérite C++)
                                  └── Conserve toute sa logique
```

**Clé:** Les Blueprints existants HÉRITENT des nouvelles classes C++. Leur logique interne reste intacte.

---

## Analyse des Risques par Asset

### 1. BP_Paradoxe (Risque: MOYEN)

**Situation actuelle:**
- Parent: `Character` ou `ACharacter`
- Contient: Mouvements, dash, grappin, wall-run, interactions balle

**Action requise:**
```
1. Créer ASpaceLeagueCharacterBase en C++ (hérite ACharacter)
2. Reparenter BP_Paradoxe → ASpaceLeagueCharacterBase
```

**Risques:**
| Risque | Probabilité | Impact | Mitigation |
|--------|-------------|--------|------------|
| Variables perdues | Faible | Haut | Backup avant migration |
| Fonctions cassées | Faible | Moyen | Tester après reparentage |
| Références invalides | Moyen | Moyen | Vérifier redirectors |

**Procédure sécurisée:**
```
1. BACKUP: Dupliquer BP_Paradoxe → BP_Paradoxe_Backup
2. Créer ASpaceLeagueCharacterBase (C++) - VIDE au début
3. Compiler C++
4. Ouvrir BP_Paradoxe → Class Settings → Parent Class
5. Changer vers ASpaceLeagueCharacterBase
6. Compiler Blueprint
7. TESTER: Lancer le jeu, vérifier mouvements
8. Si OK → Supprimer backup
9. Si KO → Restaurer depuis backup
```

---

### 2. BP_ParadoxeRaijin / BP_ParadoxeKeplar (Risque: FAIBLE)

**Situation actuelle:**
- Parent: `BP_Paradoxe`

**Action requise:**
- AUCUNE modification directe
- Ces BP héritent automatiquement via BP_Paradoxe

**Risques:**
| Risque | Probabilité | Impact |
|--------|-------------|--------|
| Cascade si BP_Paradoxe cassé | Dépendant | Haut |

**Mitigation:**
- Tester BP_Paradoxe avant de toucher aux enfants

---

### 3. BP_Ball (Risque: MOYEN)

**Situation actuelle:**
- Parent: `Actor` ou `StaticMeshActor`
- Contient: Physique, collisions, possession

**Action requise:**
```
1. Créer ABall en C++ (hérite AActor)
2. Reparenter BP_Ball → ABall
```

**Variables à préserver:**
- Composant StaticMesh
- Composant SphereCollision
- Physique settings
- Toute variable custom

**Procédure sécurisée:**
```
1. BACKUP: Dupliquer BP_Ball → BP_Ball_Backup
2. DOCUMENTER: Screenshot des variables importantes
3. Créer ABall (C++) avec composants de base
4. Reparenter BP_Ball → ABall
5. Vérifier que les composants sont préservés
6. TESTER: Physique de la balle
```

---

### 4. BPPC_Paradoxe (PlayerController) (Risque: FAIBLE)

**Situation actuelle:**
- Parent: `PlayerController`

**Action requise:**
```
1. Créer ASpaceLeaguePlayerController en C++
2. Reparenter BPPC_Paradoxe → ASpaceLeaguePlayerController
```

**Risques minimaux** car les PlayerControllers ont généralement peu de logique custom en Blueprint.

---

### 5. GI_SpaceLeague (GameInstance) (Risque: FAIBLE)

**Recommandation:** Garder en Blueprint

**Raison:**
- GameInstance est rarement critique pour les performances
- Facilité de configuration des références
- Pas besoin de réplication

---

### 6. BP_GameState_SpaceLeague (Risque: MOYEN-HAUT)

**Situation actuelle:**
- Parent: `GameStateBase`
- Contient: Score, état match

**Action requise:**
```
1. Créer ASpaceLeagueGameState en C++ avec réplication
2. Reparenter BP_GameState → ASpaceLeagueGameState
```

**Risques:**
| Risque | Probabilité | Impact |
|--------|-------------|--------|
| Réplication cassée | Moyen | Haut |
| Variables non transférées | Moyen | Moyen |

**Procédure sécurisée:**
```
1. BACKUP complet
2. Créer ASpaceLeagueGameState avec variables répliquées
3. Exposer variables avec UPROPERTY(BlueprintReadWrite)
4. Reparenter
5. TESTER en multijoueur local
```

---

## Ordre de Migration Recommandé

```
Phase 1 - Fondations (Faible risque)
├── 1.1 Créer module C++ SpaceLeague
├── 1.2 Créer ASpaceLeagueGameMode (vide)
├── 1.3 Créer ASpaceLeaguePlayerController (vide)
└── 1.4 Tester compilation

Phase 2 - Core Game (Risque moyen)
├── 2.1 Créer ASpaceLeagueCharacterBase
├── 2.2 Reparenter BP_Paradoxe
├── 2.3 TESTER mouvements
├── 2.4 Créer ABall
├── 2.5 Reparenter BP_Ball
└── 2.6 TESTER physique balle

Phase 3 - Réseau (Risque moyen-haut)
├── 3.1 Créer ASpaceLeagueGameState avec réplication
├── 3.2 Créer ASpaceLeaguePlayerState
├── 3.3 Migrer variables score vers C++
└── 3.4 TESTER multijoueur local

Phase 4 - Gameplay Avancé
├── 4.1 Créer APrison
├── 4.2 Créer AGoal
├── 4.3 Créer ATerrainEventManager
└── 4.4 Intégrer rotation terrain
```

---

## Points de Non-Retour

### Changements Réversibles
- Reparentage de Blueprint (peut revenir à l'ancien parent)
- Ajout de nouvelles classes C++
- Ajout de nouvelles fonctions

### Changements Difficiles à Annuler
- Suppression de variables (données perdues)
- Renommage de fonctions utilisées par d'autres BP
- Modification de la hiérarchie de plusieurs BP en cascade

---

## Checklist Pré-Migration

```
[ ] Backup complet du projet
[ ] Git commit "Pre-migration backup"
[ ] Tester que le jeu fonctionne AVANT migration
[ ] Documenter les variables importantes par screenshot
[ ] S'assurer que Visual Studio 2022 est installé
[ ] S'assurer que UE5 C++ templates sont installés
```

---

## Checklist Post-Migration (Par Asset)

```
[ ] Blueprint compile sans erreur
[ ] Pas de warning "Unresolved reference"
[ ] Variables préservées (vérifier valeurs)
[ ] Fonctions appellables depuis Blueprint
[ ] Test en jeu: fonctionnalité OK
[ ] Test multijoueur (si applicable)
```

---

## Plan de Rollback

Si la migration échoue :

```
1. NE PAS sauvegarder le projet
2. Fermer Unreal Editor
3. Git reset --hard HEAD
4. Rouvrir le projet
5. Vérifier que tout fonctionne
6. Analyser ce qui a échoué
7. Corriger l'approche
8. Réessayer
```

---

## Support

En cas de problème durant la migration :
1. Screenshot de l'erreur
2. Logs (Output Log complet)
3. État du Blueprint (Class Settings)
4. Me contacter avec ces informations


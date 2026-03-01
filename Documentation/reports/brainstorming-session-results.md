---
type: brainstorming-session
status: in-progress
date: 2026-02-21
session-topic: Space League - Capacités, Système de Balle, Mécaniques de Gameplay
participants: Dylan, Claude
duration: En cours
---

# Session de Brainstorming - Space League

**Navigation Vault:** [[Documentation/_ai/00_Home|00_Home]] | [[Documentation/_ai/01_Now|01_Now]] | [[Documentation/RESUME_JEU|RESUME_JEU]]
**Liens reliés:** [[Documentation/SPECS_FONCTIONNELLES|SPECS_FONCTIONNELLES]] | [[Documentation/ROADMAP|ROADMAP]] | [[Documentation/_maps/MOC_Gameplay|MOC_Gameplay]]

---

## 📋 Executive Summary

### Session Topic
Exploration et design de trois aspects fondamentaux du gameplay de Space League :
1. **Capacités des personnages** (focus Keplar + futurs personnages)
2. **Système de balle** (vitesse, contrôle, équilibrage)
3. **Mécaniques de gameplay** (mouvement, événements terrain, innovation)

### Session Goals
- **Philosophie de design :** Créer un jeu accessible avec un plafond technique élevé (modèle Rocket League)
- **Focus :** Équilibrage et innovation
- **Approche :** Recommandation guidée avec techniques adaptées à chaque sujet

### Techniques Used
- ⏳ En cours de définition...

### Total Ideas Generated
- 🔢 Compteur en cours...

### Key Themes
- 🎯 À compléter au fur et à mesure...

---

## 🌌 PARTIE 1 : Capacités des Personnages

**Durée :** En cours
**Techniques utilisées :** Remixage Thématique, Matrice d'Équilibrage, Playtest Mental

### Contexte
- **Personnages existants :**
  - Raijin (Foudre, Assaillant) - capacités complètes
  - Keplar (Galaxie, Assaillant) - capacités À DÉFINIR
- **Personnages futurs :** Chronis (Temps), Aurion (Lumière), etc.
- **Classes :** Assaillant, Sentinelle, Pilier
- **Structure des capacités :** Passif, Sort (~20s CD), Ultimate (~50s CD)

---

### 🎨 Technique 1 : Remixage Thématique

**Focus :** Keplar (Galaxie/Observation)
**Durée :** 25 minutes
**Résultat :** 3 capacités complètes définies

#### Concepts Thématiques Identifiés

**Thème principal :** Télescope spatial Kepler (observation de trajectoires célestes)

**Concepts remixés :**
- 🔭 **Observation** → Vision partielle des rebonds (Passif)
- 🌀 **Gravité** → Zone gravitationnelle contrôlée (Sort)
- 🌌 **Vision cosmique** → Révélation totale des trajectoires (Ultimate)
- 📡 **Prédiction** → Anticipation des angles et mouvements

**Référence visuelle :** Law (One Piece) pour l'effet gravitationnel

---

#### 🔭 Capacités Finales de Keplar [H]

**Rôle :** Assaillant (Galaxie/Observation)
**Philosophie de gameplay :** Information supérieure + contrôle spatial tactique

---

##### **Passif : Œil de Kepler**

**Effet :** Keplar voit une courte portion de trajectoire (1-2 mètres) avant et après chaque rebond de balle.

**Spécifications :**
- Vision partielle automatique à chaque rebond
- **Seulement Keplar** voit l'information (pas les alliés)
- Longueur : 1-2 mètres (Option A - peut devenir variable selon vitesse)
- Permet anticipation tactique sans révéler tout le jeu

**Effet permanent, sans condition.**

**Avantages :**
- ✅ Info constante pour mieux lire le jeu
- ✅ Subtil mais utile pour un joueur expérimenté
- ✅ Skill expression (savoir utiliser l'info)

**Design rationale :** Récompense l'observation et l'anticipation (thème télescope)

---

##### **Sort : Puits Gravitationnel**

**Activation :** Keplar cible un joueur (ami ou ennemi) et applique une accélération gravitationnelle

**Effets :**
- **Ciblage direct** d'un joueur spécifique (pas une zone AoE)
- Le joueur ciblé subit une forte accélération vers le sol
- **Plaqué au sol** pendant 1.5-2 secondes
- Impossible de sauter/dash pendant la durée
- Pas de portée limitée (si visible, peut être ciblé)

**Visuel :**
- Faisceau/rayon gravitationnel du joueur vers le sol
- Effet de distorsion autour du joueur ciblé
- Animation de plaquage au sol
- **Pas de sphère type Law** - effet ciblé uniquement

**Utilité tactique :**
- **Offensif :** Clouer un ennemi pour tir facile
- **Défensif :** Faire chuter un allié pour éviter un tir adverse
- **Disruption :** Interrompre un dash/saut ennemi

**Temps de recharge :** 25 secondes

**Avantages :**
- ✅ Polyvalent (off/def)
- ✅ Skill-based (bon ciblage requis)
- ✅ Peut sauver un allié ou punir un ennemi

**Design rationale :** Manipulation gravitationnelle (thème galaxie/espace)

---

##### **Ultimate : Vision Cosmique**

**Activation :** Keplar entre en état d'observation totale

**Effets :**
1. **Vision totale :** Voit TOUTES les trajectoires de **la balle** en temps réel
   - Prévisualisation complète de tous les tirs possibles
   - Tous les rebonds de la balle visibles à l'avance
   - Trajectoires de la balle révélées (pas les mouvements des joueurs)

2. **Transformation visuelle (Skin Change) :**
   - Apparence modifiée (aura cosmique, effet spatial)
   - **Visible par TOUS** les joueurs

3. **Audio cue :**
   - Référence **Reaper (Overwatch)**
   - Voix audible par tous : *"L'univers se révèle"* (ou équivalent)
   - Signale aux ennemis que l'ultimate est actif

**Limitations :**
- Keplar peut **SEULEMENT voir** les trajectoires
- Capacités normales inchangées
- Pas de boost de stats

**Durée :** 12-15 secondes
**Temps de recharge :** 50 secondes

**Avantages :**
- ✅ Game-changer pour lecture du jeu
- ✅ Permet clutch plays avec info parfaite
- ✅ Contrepartie : ennemis sont avertis (audio/visuel)

**Design rationale :** Observation cosmique maximale (thème télescope spatial)

---

##### **Counter (Type)**
- **Faible contre :** Temps
- **Fort contre :** Polarité

---

#### 💡 Insights de Design

**Points forts :**
- ✅ **Cohérence thématique parfaite** : Observation (télescope) → Vision/Prédiction
- ✅ **Identité unique** : Seul personnage avec vision prédictive
- ✅ **Skill ceiling élevé** : Information ≠ victoire automatique
- ✅ **Rôle Assaillant respecté** : Polyvalent, peut carry avec bonne lecture
- ✅ **Contreparties claires** : Ultimate visible/audible, Sort affecte alliés

**Équilibre :**
- ⚖️ **Passif subtil** mais constant (pas OP, récompense observation)
- ⚖️ **Sort tactique** avec risque (zone affecte tout le monde)
- ⚖️ **Ultimate puissant** mais signalé (ennemis peuvent jouer autour)

**Différenciation vs Raijin :**
- Raijin = **Vitesse/Agressivité** (dash, accélération balle, état foudre)
- Keplar = **Information/Contrôle spatial** (vision, gravité, prédiction)
- Deux approches du rôle Assaillant très différentes ✅

**Potentiel Rocket League-style :**
- 🎮 **Accessible** : Passif facile à comprendre
- 🏆 **Hardcore** : Exploiter la vision des rebonds demande skill
- 📈 **Skill ceiling** : Maîtriser le timing du Puits Gravitationnel
- 🧠 **Lecture du jeu** : Ultimate récompense l'intelligence tactique

---

### ⚖️ Technique 2 : Matrice d'Équilibrage
*À venir...*

---

### 🎮 Technique 3 : Playtest Mental
*À venir...*

---

## ⚽ PARTIE 2 : Système de Balle

**Durée :** À venir
**Techniques utilisées :** À définir

*Section à compléter...*

---

## 🎯 PARTIE 3 : Mécaniques de Gameplay

**Durée :** À venir
**Techniques utilisées :** À définir

*Section à compléter...*

---

## 📊 Synthèse Globale

### Idées par Catégorie

#### 🔥 Immediate Opportunities
*Idées prêtes à implémenter immédiatement...*

#### 🚀 Future Innovations
*Concepts nécessitant du développement/recherche...*

#### 🌙 Moonshots
*Idées ambitieuses et transformatives...*

#### 💡 Insights & Learnings
*Réalisations clés de la session...*

---

## 🎯 Action Planning

### Top 3 Priority Ideas
*À définir en fin de session...*

---

## 🔮 Reflection & Follow-up

### What Worked Well
*À compléter en fin de session...*

### Areas for Further Exploration
*À compléter en fin de session...*

### Recommended Follow-up Techniques
*À compléter en fin de session...*

---

*Document généré le : 2026-02-21*
*Dernière mise à jour : En cours*
*🤖 Généré avec Claude Code + BMAD™ Brainstorming Framework*

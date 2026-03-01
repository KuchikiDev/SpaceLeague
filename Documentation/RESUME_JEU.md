---
type: summary
status: draft
last_review: 2026-02-19
owner: dylan
audience: humans-agents
---

# 🎮 RÉSUMÉ COMPLET - SPACE LEAGUE

**Navigation Vault:** [[_ai/00_Home]] | [[_ai/01_Now]] | [[_ai/AGENT_CONTEXT]]
**Liens relies:** [[SPECS_FONCTIONNELLES]] | [[ROADMAP]] | [[_maps/MOC_Gameplay]]
**Note statut:** Document de synthese; certaines sections mentionnent des actions anciennes a reverifier.

## 📋 Vue d'ensemble

**Space League** est un jeu de sport compétitif multijoueur développé sur **Unreal Engine 5**, opposant deux équipes dans un terrain divisé en deux camps séparés. Le jeu combine des mécaniques de tir de précision, de mobilité avancée et de stratégie d'équipe pour créer une expérience unique de sport futuriste.

---

## 🎯 Concept Principal

### Mécanique de Base

Le jeu oppose **deux équipes** placées chacune dans leur propre camp, **sans possibilité de franchir la ligne centrale**. Toutes les actions (passes, tirs, défenses) se font à distance depuis son propre camp, créant une séparation claire et une lecture de jeu immédiate.

### Système de Score Dual

Le jeu propose **deux méthodes de scoring complémentaires** :

| Méthode | Points | Description |
|--------|--------|-------------|
| **Prison** | **2 points** | Envoyer deux joueurs adverses en prison simultanément |
| **But flottant** | **1 point** | Faire traverser la balle à travers un but flottant adverse |

Cette dualité crée un équilibre entre :
- **Stratégie agressive** (élimination par prison - 2 points)
- **Précision de tir** (buts - 1 point)

---

## 🏃 Mécaniques de Déplacement

### Actions de Base

- **Sprint** : Accélération classique pour la prise de vitesse
- **Saut** : Franchissement vertical court, utile pour esquiver ou préparer un grappin
- **Dash multidirectionnel** : Esquive rapide dans n'importe quelle direction
- **Dash mural** : Rebond contre un mur ou obstacle solide pour changer de direction
- **Grappin** : S'accrocher à certains obstacles pour changer rapidement de trajectoire

### Gestion de la Gravité

La gravité peut être modifiée :
- Par le terrain (événements dynamiques)
- Par le joueur via une capacité spéciale (long temps de récupération)

Permet des trajectoires inhabituelles pour surprendre, esquiver ou atteindre des zones spécifiques.

---

## ⚽ Système de Balle

### Contrôle de la Balle

Le **contrôle** est la base du système. Avec un timing précis, le joueur peut :
- Arrêter la balle
- La faire tourner autour du joueur (orbite)
- Préparer le tir, l'enroulé, la vitesse et le momentum offensif

**Fenêtre de tir** :
- Balle **lente** → fenêtre **longue**
- Balle **rapide** → fenêtre **courte**

### Enroulés et Tirs

Après le contrôle, le joueur peut préparer un **enroulé** (effet courbé) :
- **Trait visuel** en temps réel montrant la trajectoire
- La balle suit automatiquement la direction visée
- Prend le **premier obstacle** sur sa route (mur, obstacle, but)
- Les rebonds sont prévisualisés

**Plus la courbe est forte** :
- Plus la distance est longue
- Plus la balle met de temps à arriver
- Plus la trajectoire est prévisible... ou trompeuse

### Ultra-Contrôle

Si le joueur effectue un contrôle **parfait** (synchronisé au millimètre) :
- Balle tourne avec effet visuel et sonique
- Tir **deux fois plus rapide** que la normale
- Enroulé **plus net** avec trait visuel marqué
- Fenêtre **très courte** (récompense la précision)

**Skill expression** : Les joueurs expérimentés peuvent renverser un duel grâce à un contrôle parfait.

---

## 👥 Système de Classes

### Rôles

Le jeu propose différents rôles avec des identités de gameplay distinctes :

#### **Pilier (Tank)**
- Fenêtre de contrôle légèrement plus longue
- Tir plus stable mais moins flexible en enroulé
- Parfait pour jouer simple et sûr

#### **Assaillant**
- Contrôles très rapides
- Fenêtre plus courte mais enroulés beaucoup plus précis
- Classe reine pour les tirs techniques
- Rôle le plus polyvalent (attaque/soutien/carry)

#### **Sentinelle**
- À chaque contrôle, la balle perd légèrement de sa vitesse d'entrée
- Meilleur contrôle et tirs plus lisibles
- Idéal en défense ou pour casser les attaques adverses

### Personnages Identifiés

#### **Raijin [H]**
- **Rôle** : Assaillant
- **Type** : Foudre
- **Counter** : Faible contre Réflexion, fort contre Lumière

**Capacités** :
- **Passif (Battement du Tonnerre)** : Naturellement plus rapide (vitesse augmentée, dash plus long/vif)
- **Sort (Décharge Fulgurante)** : Décharge électrique avant tir/passe → accélération immédiate, trajectoire très tendue (cooldown 20s)
- **Ultime (Colère de Raijin)** : État de foudre vivante pendant 15s → contrôles ultra-réactifs, dashs instantanés, tirs extrêmement rapides (cooldown 50s)

#### **Keplar [H]**
- **Rôle** : Assaillant
- **Type** : Observation
- **Counter** : Malédiction

**Capacités** : (À compléter)

---

## 🎮 Système de Capacités

Chaque personnage possède **trois types de capacités** :

### **Passif**
- Effet **permanent**, toujours actif
- Renforce subtilement un aspect majeur (mobilité, défense, précision, etc.)
- S'applique au sol et en prison

### **Sort (Compétence Unique)**
- **Un seul sort actif** par personnage
- Impactant mais **cooldown court** (~5 secondes)
- Aucun système de mana → limite = temps de recharge
- Offensif, défensif ou utilitaire selon le personnage
- Accessible dès le début de la partie
- Fonctionne au sol et en prison

### **Ultimate**
- Capacité la plus puissante
- **Long temps de récupération**
- **Impact majeur** (peut renverser une situation)
- Fonctionne également en prison
- Peut être combinée avec enroulé, dash, grappin, contrôle parfait

**Contre d'Ultimate** :
- **Ultra-Contrôle** : Un joueur peut stopper un ultimate avec un contrôle parfait
- **Ultimate adverse capable de contrer**

Un ultimate **ne peut pas** être annulé par :
- Un tir normal
- Un sort classique

---

## 🏟️ Événements Terrain

### Rotation Globale

Le terrain peut pivoter de **90°, 180° ou 360°**, créant une perturbation contrôlée :
- La caméra ajuste automatiquement la vue
- La prison reste fixe
- Les joueurs doivent recalculer leurs angles de tir instantanément
- Ouvre des opportunités tactiques

### Phases Dynamiques

#### **Phase "Attaque+"** (Offensive)
- Rythme du jeu s'accélère
- Balle gagne en vitesse
- Dash plus réactifs
- Rebonds plus puissants, tirs plus incisifs
- Renforce le momentum offensif

#### **Phase "Défense+"** (Stabilisation)
- Instant de stabilité stratégique
- Buts légèrement plus fiables (déplacement plus lent, zone plus nette)
- Obstacles défensifs renvoient la balle de manière plus robuste
- Permet aux équipes en difficulté de reprendre leur souffle

#### **Phase "Chaos Léger"**
- Variation contrôlée
- Certains obstacles deviennent inactifs
- But peut basculer entre deux positions fixes
- Rebonds légèrement plus imprévisibles
- Crée des opportunités rapides pour les joueurs réactifs

### Gravity Flip

Modifie brièvement le sens de la gravité appliqué aux joueurs (sans affecter la balle) :
- Personnages peuvent être attirés vers le haut ou latéralement
- Crée des déplacements atypiques
- Offre des options d'esquive inhabituelles

### Rotation des Obstacles et Déplacement des Buts

- Les obstacles peuvent changer d'emplacement après un délai
- Les buts peuvent se repositionner parmi plusieurs zones prédéfinies
- Empêche les stratégies trop statiques
- Le terrain évolue au fil du match

---

## 🎯 Points Forts

### ✅ Design de Gameplay

1. **Mécanique de Score Dual**
   - Équilibre entre stratégie agressive (prison) et précision (buts)
   - Deux voies de victoire créent de la profondeur stratégique

2. **Système de Contrôle Technique**
   - Ultra-Contrôle récompense la maîtrise
   - Fenêtre de timing variable selon la vitesse de la balle
   - Skill expression élevée

3. **Mobilité Avancée**
   - Grappin, dash multidirectionnel, dash mural
   - Modification de gravité
   - Trajectoires complexes et créatives

4. **Séparation Claire des Camps**
   - Lecture de jeu immédiate
   - Pas de confusion sur la position des équipes
   - Focus sur la précision et la stratégie plutôt que le combat rapproché

5. **Système de Classes Différencié**
   - Chaque classe manipule la balle différemment
   - Identités de gameplay distinctes
   - Équilibre entre accessibilité et profondeur

6. **Événements Terrain Dynamiques**
   - Rotation, phases, gravity flip
   - Empêche la stagnation
   - Crée des opportunités tactiques

7. **Système de Capacités Bien Pensé**
   - Passif/Sort/Ultimate avec rôles clairs
   - Cooldowns équilibrés
   - Utilisation au sol et en prison

### ✅ Architecture Technique

1. **Unreal Engine 5**
   - Moteur moderne et performant
   - Outils de développement avancés
   - Support multijoueur intégré

2. **Structure de Projet Organisée**
   - Séparation claire des systèmes (Game/Gameplay/Data)
   - Système de Data Assets pour les personnages
   - Architecture modulaire

3. **Documentation Confluence**
   - Règles détaillées
   - Mécaniques documentées
   - Personnages décrits

---

## ⚠️ Points Faibles / Défis

### 🔴 Problèmes Techniques Identifiés

1. **Système de Base Incomplet**
   - D'après `CORRECTIONS_PRIORITAIRES.md`, plusieurs éléments manquants :
     - Structs non créés (ST_KeyFloat, ST_CooldownOverrideEntry, ST_ParamOverrideEntry)
     - GI_SpaceLeague incomplet (fonctions manquantes)
     - BP_Paradoxe utilise encore DataTable au lieu de DA_LegendBase
     - BP_GameMode_SpaceLeague non créé
     - Map non configurée avec PlayerStarts

2. **Architecture en Transition**
   - Migration en cours de DataTable vers Data Assets
   - Système de réplication à vérifier
   - GameMode/GameState à finaliser

### 🟡 Défis de Game Design

1. **Complexité de la Courbe d'Apprentissage**
   - Ultra-Contrôle très exigeant (timing au millimètre)
   - Risque de frustrer les joueurs débutants
   - Nécessite un tutoriel solide

2. **Équilibrage des Classes**
   - Différences subtiles entre classes (fenêtre de contrôle, vitesse)
   - Risque de déséquilibre si mal calibré
   - Système de types (Foudre/Réflexion/Lumière) à équilibrer

3. **Lisibilité du Jeu**
   - Beaucoup d'effets visuels simultanés (enroulés, dash, capacités, événements terrain)
   - Risque de surcharge visuelle
   - Nécessite des effets visuels clairs et distincts

4. **Système de Prison**
   - Mécanique intéressante mais complexe
   - Nécessite 2 joueurs en prison pour scorer (coordination requise)
   - Risque de frustration si mal communiqué

5. **Événements Terrain**
   - Rotation, phases, gravity flip peuvent désorienter
   - Risque de frustration si trop fréquents
   - Nécessite un équilibre entre dynamisme et lisibilité

### 🟠 Documentation et Contenu

1. **Personnages Incomplets**
   - Keplar : capacités non détaillées
   - Autres personnages mentionnés (Senna, Jhin) non documentés
   - Système de types (Foudre/Réflexion/Lumière/Malédiction) à clarifier

2. **Histoire et Style**
   - Sections vides dans les profils de personnages (Histoire, Style)
   - Univers du jeu à développer
   - Identité visuelle à définir

3. **Mécaniques Avancées**
   - Certaines mécaniques mentionnées comme "à terme" (timing du dash, grappin combiné avec sprint, etc.)
   - Roadmap de développement à clarifier

---

## 📊 État Actuel du Projet

### ✅ Ce qui est Fait

- **Design de Gameplay** : Règles et mécaniques bien définies
- **Documentation Confluence** : Base solide de documentation
- **Architecture de Base** : Structure de projet organisée
- **Système de Classes** : Concept et différenciation établis
- **Personnages** : Au moins 2 personnages partiellement définis (Raijin, Keplar)

### 🔄 En Cours

- **Migration Technique** : DataTable → Data Assets
- **Implémentation Système de Base** : GameMode, GameState, GameInstance
- **Configuration Multi-joueur** : Spawn, réplication, synchronisation

### ❌ À Faire

1. **Corrections Prioritaires** (d'après `CORRECTIONS_PRIORITAIRES.md`)
   - Créer les structs manquantes
   - Compléter GI_SpaceLeague
   - Corriger BP_Paradoxe
   - Créer BP_GameMode_SpaceLeague
   - Configurer la map

2. **Développement Gameplay**
   - Implémenter le système de contrôle de balle
   - Système d'enroulés et trajectoires
   - Système de prison
   - Système de buts flottants
   - Événements terrain

3. **Personnages**
   - Compléter les capacités de Keplar
   - Créer les autres personnages (Senna, Jhin, etc.)
   - Équilibrer les classes

4. **Polissage**
   - Effets visuels et feedback
   - Son et musique
   - UI/UX
   - Tutoriel

---

## 🎯 Recommandations

### Priorité 1 : Finaliser la Base Technique
- Compléter les corrections prioritaires
- Stabiliser l'architecture
- Tester le spawn et la réplication

### Priorité 2 : Prototyper le Core Gameplay
- Système de contrôle de balle
- Système de tir et enroulés
- Système de score (prison + buts)

### Priorité 3 : Équilibrer et Polir
- Équilibrer les classes
- Ajuster la courbe d'apprentissage
- Améliorer la lisibilité visuelle

### Priorité 4 : Contenu
- Compléter les personnages
- Développer l'univers
- Créer plus de variété (maps, événements)

---

## 📝 Conclusion

**Space League** présente un **concept de gameplay original et prometteur** avec :
- Une mécanique de score dual intéressante
- Un système de contrôle technique récompensant la maîtrise
- Une mobilité avancée créative
- Des classes différenciées

Cependant, le projet est encore en **phase de développement précoce** avec :
- Des bases techniques à finaliser
- Des mécaniques à implémenter
- Du contenu à compléter
- Un équilibrage à affiner

Le jeu a le potentiel d'être un **sport compétitif unique** s'il parvient à :
- Maintenir l'équilibre entre accessibilité et profondeur
- Offrir une lisibilité visuelle claire malgré la complexité
- Créer une courbe d'apprentissage progressive
- Équilibrer finement les classes et mécaniques

---

*Document genere le : 2026-02-19 (date de revue documentaire)*
*Basé sur : Confluence Space League + Codebase + Documentation projet*


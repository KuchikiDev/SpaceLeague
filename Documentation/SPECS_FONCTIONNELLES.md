---
type: spec-functional
status: active
last_review: 2026-02-19
owner: dylan
audience: humans-agents
---


# Spécifications Fonctionnelles - Space League

**Version:** 1.0
**Date:** 2026-01-25
**Auteur:** Claude (BMAD)
**Source:** Confluence Space League

**Navigation Vault:** [[_ai/00_Home]] | [[_ai/01_Now]] | [[_ai/AGENT_CONTEXT]]
**Liens relies:** [[RESUME_JEU]] | [[ROADMAP]] | [[_maps/MOC_Gameplay]]

---

## 1. Vue d'Ensemble du Jeu

### 1.1 Concept

Space League est un jeu de sport compétitif opposant deux équipes dans une arène spatiale. Les joueurs doivent marquer des points en envoyant la balle dans le but adverse ou en éliminant les adversaires pour les envoyer en prison.

### 1.2 Format

| Paramètre | Valeur |
|-----------|--------|
| Joueurs par équipe | 2 ou 3 |
| Formats supportés | 2v2, 3v3 |
| Durée match | 5 minutes |

---

## 2. Règles de Base

### 2.1 Structure du Terrain

```
┌─────────────────────────────────────────────────────────────┐
│                        PRISON                                │
│   (Zone commune - au-dessus du terrain principal)           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   ┌─────────────────────┬─────────────────────┐            │
│   │                     │                     │            │
│   │      CAMP A         │      CAMP B         │            │
│   │                     │                     │            │
│   │    [But B]          │          [But A]    │            │
│   │                     │                     │            │
│   │   (Équipe A)        │    (Équipe B)       │            │
│   │                     │                     │            │
│   └─────────────────────┴─────────────────────┘            │
│                    LIGNE MÉDIANE                            │
│              (infranchissable)                              │
└─────────────────────────────────────────────────────────────┘
```

**Règles:**
- Chaque équipe reste dans son camp
- Aucun joueur ne peut franchir la ligne médiane
- Toutes les actions se font à distance

### 2.2 Système de Scoring

| Action | Points | Condition |
|--------|--------|-----------|
| Prison complète | 2 | 2 joueurs adverses en prison simultanément |
| But | 1 | Balle traverse le but flottant adverse |

### 2.3 Mécanisme de Prison

**Entrée en prison:**
- Un joueur touché par la balle adverse va en prison
- Le joueur est téléporté dans la zone prison (au-dessus du terrain)

**Sortie de prison:**
- Toucher un adversaire avec la balle depuis la prison
- Ou libération automatique après X secondes (configurable)

**Activités en prison:**
- Les prisonniers peuvent se déplacer librement
- Ils peuvent attraper une balle envoyée par un allié
- Ils peuvent tirer sur les adversaires
- Ils **ne peuvent PAS** marquer de but

**Scoring prison:**
- Si 2 joueurs d'une équipe sont en prison simultanément → 2 points pour l'adversaire
- Les 2 prisonniers sont alors libérés

### 2.4 Mécanisme de But

- Chaque camp possède un but flottant
- Le but peut apparaître à différents emplacements prédéfinis
- Quand un but est marqué, il se déplace vers un nouvel emplacement
- Un but = 1 point

### 2.5 Timer Inactivité

- Si la balle n'est pas touchée pendant X secondes
- La balle effectue automatiquement un tir puissant vers le but

---

## 3. Mouvements des Personnages

### 3.1 Déplacements de Base

| Mouvement | Touche (par défaut) | Description |
|-----------|---------------------|-------------|
| Marche | Stick gauche | Déplacement standard |
| Sprint | LB maintenu | Vitesse augmentée |
| Saut | A / X | Saut vertical |
| Accroupi | B / O | Position basse |

### 3.2 Mouvements Avancés

| Mouvement | Touche | Description | Cooldown |
|-----------|--------|-------------|----------|
| Dash | RB | Déplacement rapide directionnel | 2s |
| Grappin | LT | Accroche et tire vers un point | 3s |
| Wall-run | Auto | Course sur les murs | - |
| Double saut | A×2 | Second saut en l'air | - |

### 3.3 Interactions Balle

| Action | Touche | Description |
|--------|--------|-------------|
| Attraper | Auto | Attrape la balle à proximité |
| Tir | RT | Lance la balle en ligne droite |
| Passe | RT + direction | Lance vers un allié |
| Enroulé | RT + stick | Ajoute une courbe au tir |
| Lâcher | Y / Triangle | Lâche la balle sans force |

---

## 4. Événements Terrain

### 4.1 Rotation Globale

| Type | Angle | Effet |
|------|-------|-------|
| Rotation 90° | 90° | Terrain pivote d'un quart |
| Rotation 180° | 180° | Terrain pivote d'un demi-tour |
| Rotation 360° | 360° | Rotation complète (perturbation visuelle) |

**Comportement:**
- La prison reste FIXE (ne tourne jamais)
- La caméra s'ajuste automatiquement
- Les joueurs doivent recalculer leurs angles

### 4.2 Phases de Jeu

#### Phase Attaque+
- Balle plus rapide
- Dash plus réactifs
- Rebonds plus puissants
- Durée: 15-30 secondes

#### Phase Défense+
- Buts moins mobiles
- Obstacles défensifs renforcés
- Durée: 15-30 secondes

#### Phase Chaos Léger
- Obstacles aléatoirement actifs/inactifs
- Rebonds légèrement imprévisibles
- Durée: 10-20 secondes

### 4.3 Gravity Flip (Post-v1.0)

- Gravité des joueurs modifiée temporairement
- La balle n'est PAS affectée
- Permet des esquives inhabituelles

### 4.4 Déplacement Dynamique

- Les obstacles peuvent changer de position
- Les buts se déplacent entre positions prédéfinies
- Timing prévisible pour éviter la frustration

---

## 5. Personnages (Legends)

### 5.1 Rôles

| Rôle | Description | Style de jeu |
|------|-------------|--------------|
| Assaillant | Attaque, soutien, carry | Polyvalent, offensif |
| Sentinelle | Défenseur, gardien | Protection, anticipation |
| Pilier | Tank, ligne de front | Robuste, présence |

### 5.2 Système de Types

```
Cycle de faiblesses:

Malédiction → Observation → Origine → Temps
     ↑                                  ↓
     ←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←

Temps → Galaxie → Polarité → Réflexion
   ↑                              ↓
   ←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←

Réflexion → Foudre → Lumière → Malédiction
      ↑                            ↓
      ←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←
```

### 5.3 Structure des Capacités

Chaque personnage possède:

| Slot | Type | Activation | Cooldown typique |
|------|------|------------|------------------|
| Passif | Effet permanent | Automatique | - |
| Sort | Actif | Bouton X | 15-25s |
| Ultime | Actif puissant | Bouton Y | 45-60s |

### 5.4 Personnages v1.0

#### Raijin [H] - Assaillant (Foudre)

| Capacité | Type | Effet | CD |
|----------|------|-------|-----|
| Battement du Tonnerre | Passif | Vitesse augmentée, dash plus long | - |
| Décharge Fulgurante | Sort | Balle accélérée, trajectoire tendue | 20s |
| Colère de Raijin | Ultime | État foudre: contrôles ultra-réactifs | 50s |

**Counter:** Faible contre Réflexion, Fort contre Lumière

#### Keplar [H] - Assaillant (Galaxie)

| Capacité | Type | Effet | CD |
|----------|------|-------|-----|
| (À définir) | Passif | - | - |
| (À définir) | Sort | - | - |
| (À définir) | Ultime | - | - |

**Counter:** Faible contre Temps, Fort contre Polarité

### 5.5 Personnages Futurs (v2.0+)

| Personnage | Type | Rôle | Priorité |
|------------|------|------|----------|
| Chronis | Temps | Sentinelle | 3 |
| Aurion | Lumière | Pilier | 4 |
| Andaris | ? | Assaillant | 5 |
| Magnora | Polarité | Sentinelle | 6 |
| Vaalbara | Observation | Sentinelle | 7 |
| Pandore | Malédiction | Pilier | 8 |
| Obsidia | Origine | Pilier | 9 |

---

## 6. Interface Utilisateur

### 6.1 HUD En Jeu

```
┌─────────────────────────────────────────────────────────┐
│  [Score A: 0]              [Timer: 5:00]    [Score B: 0] │
├─────────────────────────────────────────────────────────┤
│                                                         │
│                    (Zone de jeu)                        │
│                                                         │
├─────────────────────────────────────────────────────────┤
│  [Portrait]  [❤️❤️❤️]                                    │
│  [Passif ✓] [Sort: 5s] [Ult: 30s]                       │
│                                                         │
│  [Prison: 0/2]        [Coéquipiers: ●●]                │
└─────────────────────────────────────────────────────────┘
```

### 6.2 Écrans Requis

| Écran | Priorité | Phase |
|-------|----------|-------|
| Menu Principal | Haute | 1 |
| Sélection Personnage | Haute | 2 |
| HUD Match | Haute | 1 |
| Scoreboard | Moyenne | 1 |
| Écran Fin de Match | Moyenne | 1 |
| Lobby Multijoueur | Haute | 2 |
| Options | Basse | 3 |

---

## 7. Multijoueur

### 7.1 Modes de Jeu

| Mode | Joueurs | Réseau |
|------|---------|--------|
| Local 2P | 2 | Même machine |
| Local 4P | 4 | Même machine (split-screen) |
| Online 2v2 | 4 | Internet |
| Online 3v3 | 6 | Internet |

### 7.2 Flux Multijoueur Online

```
1. Menu Principal
   ↓
2. Créer Partie / Rejoindre
   ↓
3. Lobby (sélection perso, équipe)
   ↓
4. Tous prêts → Chargement Map
   ↓
5. Match
   ↓
6. Fin → Scoreboard → Retour Lobby
```

### 7.3 Matchmaking (v2.0+)

- File d'attente par format (2v2, 3v3)
- Matchmaking basé sur le niveau (futur)
- Ping prioritaire pour la qualité de connexion

---

## 8. Progression (v2.0+)

### 8.1 Déblocage

| Élément | Méthode |
|---------|---------|
| Personnages 1-2 | Débloqués par défaut |
| Personnages 3-4 | Gagnés après X matchs |
| Personnages 5-9 | Objectifs spécifiques |
| Skins | Monnaie in-game |

### 8.2 Statistiques

| Stat | Description |
|------|-------------|
| Matchs joués | Total |
| Victoires | Nombre |
| Buts marqués | Total |
| Éliminations | Total |
| Temps en prison | Cumulé |

---

## 9. Audio (Guidelines)

### 9.1 Effets Sonores Requis

| Catégorie | Sons |
|-----------|------|
| Balle | Tir, passe, rebond, attraper |
| Mouvement | Pas, dash, grappin, saut |
| Score | But, prison, libération |
| UI | Boutons, transitions |
| Ambiance | Foule, arena |

### 9.2 Musique

| Contexte | Style |
|---------|-------|
| Menu | Électronique calme |
| Match | Électronique dynamique |
| Victoire | Épique, triomphant |
| Défaite | Mélancolique |

---

## 10. Glossaire

| Terme | Définition |
|-------|------------|
| Legend | Personnage jouable |
| Camp | Moitié de terrain d'une équipe |
| Prison | Zone au-dessus du terrain pour joueurs éliminés |
| Enroulé | Effet de courbe sur la balle |
| Dash | Déplacement rapide avec invincibilité courte |
| Grappin | Outil d'accroche pour traverser rapidement |
| Type | Élément associé à un personnage (ex: Foudre) |
| Counter | Type avantagé contre un autre |


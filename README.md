<div align="center">

# SPACE LEAGUE

**Jeu de sport compétitif futuriste — Unreal Engine 5**

*Développé par Byakuya Corps*

---

[![Unreal Engine 5](https://img.shields.io/badge/Unreal_Engine-5-black?style=for-the-badge&logo=unrealengine)](https://www.unrealengine.com/)
[![C++](https://img.shields.io/badge/C++-17-00599C?style=for-the-badge&logo=cplusplus)](https://isocpp.org/)
[![Blueprint](https://img.shields.io/badge/Blueprint-UE5-0071C5?style=for-the-badge)](https://docs.unrealengine.com/en-US/blueprints/)
[![Jira](https://img.shields.io/badge/Jira-Project_Management-0052CC?style=for-the-badge&logo=jira)](https://byakuyacorps.atlassian.net/)
[![Confluence](https://img.shields.io/badge/Confluence-Documentation-172B4D?style=for-the-badge&logo=confluence)](https://byakuyacorps.atlassian.net/)
[![Claude AI](https://img.shields.io/badge/Claude_AI-BMAD_Agents-D4A017?style=for-the-badge)](https://claude.ai/)
[![Perforce](https://img.shields.io/badge/Perforce-Assets_&_UE5-404040?style=for-the-badge&logo=perforce)](https://www.perforce.com/)

</div>

---

> ### ⚠️ Dépôt de démonstration — projet incomplet par conception
>
> Ce dépôt a été créé **uniquement à des fins de présentation** (CV, portfolio, école) pour montrer le travail réalisé : architecture technique, code source, documentation, game design et outils.
>
> Le projet Unreal Engine 5 complet (assets, textures, meshes, animations, Content/) est **trop volumineux pour Git** (~27 GB). Il est versionné sur **Perforce**, le standard de l'industrie du jeu vidéo pour les fichiers binaires lourds (Epic Games, EA, Ubisoft).
>
> **Ce dépôt contient :** code source C++, documentation, configurations, maquettes, modèles 3D légers, identité visuelle et outils.
> **Le reste (projet UE5 complet) est sur Perforce**, accessible sur demande.

---

## Table des matières

- [Présentation du jeu](#-présentation-du-jeu)
- [Concept de gameplay](#-concept-de-gameplay)
- [Mécaniques principales](#-mécaniques-principales)
- [Personnages (Legends)](#-personnages-legends)
- [Stack technique](#-stack-technique)
- [Architecture du projet](#-architecture-du-projet)
- [Structure des dossiers](#-structure-des-dossiers)
- [Roadmap de développement](#-roadmap-de-développement)
- [Outils & Workflow](#-outils--workflow)
- [Équipe](#-équipe)
- [Documentation](#-documentation)

---

## 🎮 Présentation du jeu

**Space League** est un **jeu de sport compétitif multijoueur** développé sur Unreal Engine 5, opposant deux équipes dans une arène futuriste divisée en deux camps distincts. Le jeu fusionne des mécaniques de **tir de précision**, de **mobilité avancée**, de **stratégie d'équipe** et un **système de contrôle de balle technique** pour créer une expérience de sport futuriste unique et hautement compétitive.

> Imaginez un mélange entre **Rocket League**, **Valorant** et **Quidditch** — mais avec de la gravité modifiable, des grappins, et des ultimes capables de retourner un match.

Le projet est en développement actif avec une **gestion de projet professionnelle** (Jira + Confluence) et une **intégration d'IA** pour accélérer le design, l'architecture et la documentation.

---

## 🎯 Concept de gameplay

### Règles de base

Deux équipes s'affrontent dans une arène partagée **sans possibilité de traverser la ligne centrale**. Toutes les actions (passes, tirs, défenses) se font à distance depuis son propre camp, créant une lecture de jeu claire et une tension constante.

### Système de score dual

| Méthode | Points | Description |
|---------|--------|-------------|
| **Prison** | **2 points** | Envoyer 2 joueurs adverses en prison simultanément |
| **But flottant** | **1 point** | Faire traverser la balle à travers un but adverse flottant |

Cette dualité stratégique force les équipes à choisir entre :
- **Agression** : cibler les joueurs ennemis pour les emprisonner (2 pts)
- **Précision** : viser les buts pour scorer régulièrement (1 pt)

---

## 🏃 Mécaniques principales

### Mobilité avancée

| Action | Description |
|--------|-------------|
| **Sprint** | Accélération classique |
| **Dash multidirectionnel** | Esquive rapide dans n'importe quelle direction |
| **Dash mural** | Rebond contre les surfaces pour changer de trajectoire |
| **Grappin** | Accroché à des obstacles pour se propulser |
| **Modification de gravité** | Capacité spéciale changeant le sens de la gravité du joueur |

### Système de contrôle de balle

Le **contrôle** est le cœur du gameplay. Avec un timing précis, le joueur peut :
- Stopper la balle et la faire orbiter autour de lui
- Préparer un **enroulé** (tir avec effet courbé) avec prévisualisation de trajectoire en temps réel
- Réaliser un **Ultra-Contrôle** (timing parfait au millimètre) pour doubler la vitesse du tir

```
Balle lente  → Fenêtre de contrôle longue  → Tir standard
Balle rapide → Fenêtre de contrôle courte  → Tir puissant
Ultra-Contrôle → Fenêtre très courte       → Tir × 2 vitesse + enroulé parfait
```

### Événements terrain dynamiques

Le terrain évolue en cours de match pour empêcher la stagnation :

- **Rotation** : Le terrain pivote de 90°, 180° ou 360°, forçant les joueurs à recalculer leurs angles
- **Phase Attaque+** : Rythme accéléré, balles plus rapides, dashs plus réactifs
- **Phase Défense+** : Stabilisation, obstacles défensifs renforcés
- **Phase Chaos Léger** : Obstacles inactifs, buts repositionnables, rebonds imprévisibles
- **Gravity Flip** : Inversion brève de la gravité sur les joueurs

---

## 👥 Personnages (Legends)

### Classes de jeu

| Classe | Style | Caractéristique |
|--------|-------|-----------------|
| **Assaillant** | Offensif / Technique | Contrôles ultra-rapides, enroulés précis, classe la plus polyvalente |
| **Pilier (Tank)** | Stable / Fiable | Fenêtre de contrôle plus longue, tir stable |
| **Sentinelle** | Défensif | Chaque contrôle réduit la vitesse de la balle, idéal pour casser les attaques |

### Système de capacités (3 par personnage)

```
Passif    → Effet permanent, actif en jeu et en prison
Sort      → Capacité active, cooldown court (~5s), impactante
Ultimate  → Capacité puissante, long cooldown, peut renverser un match
```

Un **Ultra-Contrôle parfait** peut contrecarrer un ultimate adverse — récompensant la maîtrise technique.

### Personnages (roster actuel & prévu)

| Personnage | Type | Rôle | État |
|------------|------|------|------|
| **Raijin** | Foudre ⚡ | Assaillant | En développement |
| **Keplar** | Galaxie 🌌 | Assaillant | En développement |
| **Chronis** | Temps ⏳ | Sentinelle | Prévu Phase 3 |
| **Aurion** | Lumière ✨ | Pilier | Prévu Phase 3 |

#### Focus : Raijin ⚡
- **Passif — Battement du Tonnerre** : Vitesse naturellement augmentée, dash plus long et plus vif
- **Sort — Décharge Fulgurante** : Décharge électrique avant un tir → trajectoire ultra-tendue *(CD: 20s)*
- **Ultimate — Colère de Raijin** : 15 secondes en état de foudre vivante → contrôles ultra-réactifs, tirs extrêmement rapides *(CD: 50s)*

---

## 🛠 Stack technique

| Technologie | Usage |
|-------------|-------|
| **Unreal Engine 5** | Moteur de jeu principal |
| **C++ (MSVC)** | Architecture core, réplication réseau, performance |
| **Blueprints UE5** | Logique visuelle, animations, polish, tuning |
| **Enhanced Input** | Système d'input moderne UE5 |
| **GameplayTags** | Classification des capacités et états |
| **UMG / Slate** | Interface utilisateur (HUD, menus) |
| **NetCore (UE5)** | Réplication multijoueur |
| **Jira** | Gestion de projet, tickets, sprints |
| **Confluence** | Documentation technique et design |
| **Claude AI + BMAD** | Agents IA pour design, architecture, documentation |
| **Visual Studio 2022** | IDE pour le développement C++ |
| **Obsidian** | Knowledge base locale (vault de documentation) |

### Architecture hybride C++ / Blueprint

Le projet suit les **best practices Epic Games** avec une séparation claire des responsabilités :

```
C++ (Core)                          Blueprint (Polish)
─────────────────────────────       ─────────────────────────────
ASpaceLeagueGameMode                BP_Raijin (hérite C++)
ASpaceLeagueGameState               BP_Keplar (hérite C++)
ASpaceLeaguePlayerController        WBP_HUD
ASpaceLeaguePlayerState             WBP_Scoreboard
ASpaceLeagueCharacterBase           Animations, VFX, SFX
ABall                               Effets de contrôle de balle
AGoal                               Effets d'événements terrain
APrison
ATerrainEventManager
```

> **Principe** : Le C++ gère tout ce qui touche à la performance, la réplication réseau et la maintenabilité. Le Blueprint gère les variations visuelles, le tuning et le polish.

---

## 📁 Structure des dossiers

```
Leagues/
│
├── 📂 ProjetUE5/               → Projets Unreal Engine 5
│   ├── MovementParadoxe/       → Projet principal actif (C++ + Blueprints)
│   │   ├── Source/             → Code C++ (modules, classes core)
│   │   ├── Content/            → Assets Blueprint, maps, Data Assets
│   │   └── Plugins/            → Plugins UE5 additionnels
│   └── Flame/                  → Prototype expérimental (effets, shaders)
│
├── 📂 Ninja/                   → Premier prototype UE5 (early exploration)
│
├── 📂 Documentation/           → Documentation complète du projet
│   ├── SPECS_FONCTIONNELLES.md → Règles, mécaniques, personnages, UI
│   ├── ARCHITECTURE.md         → Architecture technique C++/Blueprint
│   ├── SPECS_TECHNIQUES.md     → Headers C++ détaillés, prêts à implémenter
│   ├── ROADMAP.md              → Phases, tickets Jira, critères de validation
│   ├── RESUME_JEU.md           → Synthèse complète du concept
│   ├── IMPACT_MIGRATION.md     → Risques et procédures de migration Blueprint→C++
│   └── _ai/                    → Contexte et conventions pour agents IA
│
├── 📂 Assets/                  → Assets graphiques et packs 3D
│   └── FeudalJapan_Meshingun/  → Pack d'assets thématiques (Japon féodal)
│
├── 📂 Model/                   → Modèles 3D créés par Byakuya
│
├── 📂 HUD/                     → Maquettes et brouillons d'interface (UI/UX)
│
├── 📂 Map/                     → Concepts et plans de niveaux
│
├── 📂 Logo/                    → Identité visuelle du projet
│   ├── Logo.ai                 → Fichier source Adobe Illustrator
│   └── L1.png, L2.png...       → Exports du logo
│
└── 📂 Brouillon/               → Esquisses, concepts, idées en cours
```

---

## 🗺 Roadmap de développement

```
PHASE 0          PHASE 1          PHASE 2          PHASE 3
Fondations       Core Game        Multijoueur      Extension
──────────       ─────────        ───────────      ─────────
• Nettoyage      • 2 personnages  • Local 2-4P     • 4 personnages
• Migration C++  • Événements     • Online LAN     • Capacités
• Architecture   • Tests gameplay • Matchmaking    • Polish
• GameMode/State • Score/Prison   • Lobby UI       • Équilibrage

[████░░░░░░]     [░░░░░░░░░░]     [░░░░░░░░░░]     [░░░░░░░░░░]
  En cours          À venir          À venir          À venir
```

### Phase 0 — Fondations (en cours)

| Ticket | Tâche | Priorité |
|--------|-------|----------|
| LEAG-005 | Créer module C++ SpaceLeague | Critique |
| LEAG-006 | Créer ASpaceLeagueGameMode | Critique |
| LEAG-007 | Créer ASpaceLeagueGameState | Critique |
| LEAG-009 | Créer ASpaceLeagueCharacterBase | Critique |
| LEAG-011 | Créer ABall C++ | Haute |
| LEAG-016 | Configurer Map principale | Haute |

### Classes C++ à implémenter

| Classe | Fichier | Phase |
|--------|---------|-------|
| `ASpaceLeagueGameMode` | `Core/SpaceLeagueGameMode.h` | Phase 0 |
| `ASpaceLeagueGameState` | `Core/SpaceLeagueGameState.h` | Phase 0 |
| `ASpaceLeagueCharacterBase` | `Characters/SpaceLeagueCharacterBase.h` | Phase 0 |
| `ABall` | `Gameplay/Ball.h` | Phase 0 |
| `AGoal` | `Gameplay/Goal.h` | Phase 1 |
| `APrison` | `Gameplay/Prison.h` | Phase 1 |
| `ATerrainEventManager` | `Gameplay/TerrainEventManager.h` | Phase 1 |
| `UAbilityComponent` | `Characters/Components/AbilityComponent.h` | Phase 3 |

---

## ⚙️ Outils & Workflow

### Gestion de projet — Jira

Tout le développement est tracé via **Jira** (espace `SL` — Space League) avec des tickets numérotés `LEAG-XXX`, organisés par phase et priorité. Le backlog est géré en sprints pour maintenir un rythme de développement structuré.

> 📋 [Jira Space League](https://byakuyacorps.atlassian.net/) *(accès privé)*

### Documentation — Confluence

La documentation de design et technique est maintenue sur **Confluence** :
- Règles du jeu complètes
- Fiches personnages et capacités
- Décisions d'architecture
- Bilans de sprint

> 📚 [Confluence Space League](https://byakuyacorps.atlassian.net/wiki/spaces/SL/) *(accès privé)*

### Intelligence Artificielle — Claude AI + Méthode BMAD

Space League intègre une approche de **développement assisté par IA** qui va bien au-delà de la simple génération de code. Le projet utilise **Claude (Anthropic)** via le framework **BMAD (Boilerplate Multi-Agent Design)** — une méthode structurée qui orchestre plusieurs agents IA spécialisés pour couvrir toutes les dimensions d'un projet.

#### Qu'est-ce que BMAD ?

BMAD est un framework open-source qui transforme Claude en une **équipe virtuelle de professionnels du jeu vidéo**, chacun avec un rôle, des responsabilités et un contexte précis. Plutôt que d'utiliser l'IA comme un simple outil de recherche, BMAD permet de conduire de vraies sessions de travail structurées.

#### Comment Claude a été utilisé sur ce projet

| Usage | Détail |
|-------|--------|
| **Brainstorming gameplay** | Sessions structurées pour concevoir les mécaniques (système de balle, enroulés, prison, ultra-contrôle), explorer les alternatives, identifier les risques de game design |
| **Game design** | Conception et équilibrage des classes (Pilier/Assaillant/Sentinelle), des personnages (Raijin, Keplar...), du système de capacités Passif/Sort/Ultimate |
| **Architecture technique** | Décisions d'architecture C++/Blueprint, choix des patterns (Data Assets vs DataTable, réplication réseau), migration progressive Blueprint → C++ |
| **Documentation vivante** | Génération et maintien automatique des specs fonctionnelles, specs techniques, roadmap, rapports de phase et fiches personnages |
| **Revue de code et QA** | Analyse des Blueprints existants, identification des dépendances, plans de migration sécurisés, risk profiles |
| **Gestion de projet** | Création et organisation des tickets Jira (LEAG-001 à LEAG-056), estimation des phases, critères de validation |

#### Les agents impliqués

| Agent | Rôle dans Space League |
|-------|------------------------|
| **Analyst** | Analyse des besoins, rédaction des specs fonctionnelles |
| **Architect** | Conception de l'architecture C++/Blueprint, choix techniques |
| **PM** | Backlog Jira, roadmap, priorisation des 4 phases |
| **Dev** | Assistance à l'implémentation C++, headers, patterns |
| **QA** | Revues d'architecture, plans de test, risk profiles |
| **PO** | Validation des user stories, critères d'acceptation |

> L'utilisation de Claude via BMAD a permis de produire en quelques sessions une documentation technique et fonctionnelle équivalente à plusieurs semaines de travail classique, tout en maintenant une cohérence entre toutes les couches du projet (design → technique → gestion).

### Knowledge Base — Obsidian

Un vault Obsidian local relie toute la documentation en un **graphe de connaissances** : specs, architecture, roadmap et contexte IA sont interconnectés et navigables.

---

## 👥 Équipe

### Neyru — Lead Developer

Responsable de l'architecture technique et du développement C++/Blueprint.

- **Développement C++** : Architecture core (GameMode, GameState, CharacterBase, Ball, Goal, Prison)
- **Blueprints** : Systèmes de gameplay, logique de personnages, UI
- **Architecture** : Design patterns, réplication réseau, migration Blueprint→C++
- **Workflow & Tooling** : Mise en place Jira, Confluence, intégration agents IA (BMAD)
- **Documentation** : Specs techniques, roadmap, architecture

### Byakuya — Game Designer & 3D Artist

Responsable du design créatif, de l'univers visuel et du game design.

- **Modélisation 3D** : Création des assets et personnages du jeu (Blender / outils 3D)
- **Animations** : Conception et intégration des animations de personnages
- **Brainstorming & Game Design** : Conception des mécaniques, équilibrage, personnages
- **Identité visuelle** : Logo, HUD, charte graphique
- **Développement** : Montée en compétences sur C++ et Blueprints UE5

> Byakuya apporte à la fois la vision créative du projet et développe progressivement ses compétences en développement Unreal Engine (Blueprint → C++), couvrant ainsi les deux dimensions — art et technique — du développement de jeu vidéo.

---

## 📄 Documentation

La documentation complète se trouve dans le dossier [`Documentation/`](Documentation/) :

| Document | Description |
|----------|-------------|
| [SPECS_FONCTIONNELLES.md](Documentation/SPECS_FONCTIONNELLES.md) | Règles du jeu, mécaniques, personnages, UI |
| [ARCHITECTURE.md](Documentation/ARCHITECTURE.md) | Architecture technique hybride C++/Blueprint |
| [SPECS_TECHNIQUES.md](Documentation/SPECS_TECHNIQUES.md) | Code C++ détaillé, headers, structs, enums |
| [ROADMAP.md](Documentation/ROADMAP.md) | Phases, tickets Jira, critères de validation |
| [RESUME_JEU.md](Documentation/RESUME_JEU.md) | Synthèse du concept et état du projet |
| [IMPACT_MIGRATION.md](Documentation/IMPACT_MIGRATION.md) | Procédures de migration Blueprint → C++ |

---

## 🗄 Versioning — Git + Perforce

Le projet utilise **deux systèmes de versioning complémentaires** :

### Git (ce dépôt) — Code & Documentation

Git est utilisé pour tout ce qui est **texte, code et assets légers** :

| Inclus dans Git | Description |
|-----------------|-------------|
| `Documentation/` | Specs, architecture, roadmap, résumé du jeu |
| `ProjetUE5/*/Source/` | Code C++ source du projet |
| `ProjetUE5/*/Config/` | Configurations Unreal Engine |
| `ProjetUE5/*.uproject` | Fichiers de projet UE5 |
| `Logo/` | Identité visuelle (PNG, AI, FLA) |
| `HUD/` | Maquettes d'interface |
| `Model/` | Fichiers de modèles 3D |
| `Map/` | Concepts de niveaux |
| `Brouillon/` | Esquisses et concepts |
| `web-bundles/` | Assets web |

### Perforce — Assets binaires & projet UE5 complet

Perforce est le standard de l'industrie du jeu vidéo pour la gestion des **fichiers binaires volumineux** qui sont incompatibles avec Git :

| Exclu de Git (sur Perforce) | Taille | Raison |
|-----------------------------|--------|--------|
| `ProjetUE5/*/Content/` | ~17 GB | Assets binaires UE5 (textures, meshes, audio) |
| `Assets/` | ~9 GB | Packs d'assets 3D sources |
| `Ninja/Content/` | ~700 MB | Assets du prototype |
| `ProjetUE5/*/Binaries/` | — | Fichiers compilés (générés) |
| `ProjetUE5/*/Intermediate/` | — | Fichiers intermédiaires (générés) |
| `ProjetUE5/*/Saved/` | — | Sauvegardes auto (générées) |

> Perforce est le système utilisé par la majorité des studios AAA (Epic Games, EA, Ubisoft) précisément pour sa gestion des gros fichiers binaires et son système de locking adapté aux assets 3D.

---

## 🚀 Lancer le projet

### Prérequis

- **Unreal Engine 5.3+** (via Epic Games Launcher)
- **Visual Studio 2022** avec workload *Game development with C++*
- **Windows 10/11**

### Installation

```bash
# Cloner le dépôt
git clone https://github.com/KuchikiDev/SpaceLeague.git

# Naviguer vers le projet UE5 actif
cd ProjetUE5/MovementParadoxe

# Générer les fichiers de projet Visual Studio
# Clic droit sur MovementParadoxe.uproject → "Generate Visual Studio project files"

# Compiler et ouvrir
# Double-cliquer sur MovementParadoxe.uproject
```

> **Note :** Sans les fichiers `Content/` (hébergés sur Perforce), le projet s'ouvrira dans UE5 mais sans assets visuels. Pour accéder au projet complet, contacter l'équipe.

---

## ⚖️ Droits & Propriété intellectuelle

**© 2025-2026 Byakuya Corps — Tous droits réservés.**

Ce dépôt et l'ensemble de son contenu (code source, documentation, designs, modèles 3D, identité visuelle, concepts de gameplay) sont la **propriété exclusive de Byakuya Corps**, représenté par :

- **Neyru** (Dylan Pischedda) — Lead Developer
- **Byakuya** — Game Designer & 3D Artist

### Ce qui est autorisé

- Consulter et lire le contenu à des fins d'évaluation (recrutement, école, portfolio)
- Partager le lien vers ce dépôt public

### Ce qui est interdit sans autorisation écrite

- Copier, reproduire ou réutiliser le code source ou les assets dans un autre projet
- Distribuer, revendre ou publier tout ou partie du contenu
- Utiliser le nom, le logo ou l'identité visuelle de Space League ou Byakuya Corps

### Assets tiers

Ce projet utilise des assets sous licence tierce :
- **Unreal Engine 5** — [Epic Games EULA](https://www.unrealengine.com/en-US/eula/unreal)
- **FeudalJapan Meshingun** — asset pack sous licence Marketplace Epic Games

Pour toute demande de collaboration ou d'accès au projet complet : contacter l'équipe via GitHub Issues.

---

<div align="center">

*Space League — Byakuya Corps © 2025-2026*

*Développé avec Unreal Engine 5, C++, Blueprints, Jira, Confluence et Claude AI*

</div>

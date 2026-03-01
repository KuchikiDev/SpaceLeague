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

## 📖 Wiki

Toute la documentation détaillée est disponible sur le **[Wiki GitHub](https://github.com/KuchikiDev/SpaceLeague/wiki)** :

| Page | Description |
|------|-------------|
| [Présentation du jeu](https://github.com/KuchikiDev/SpaceLeague/wiki/Presentation) | Concept, règles, système de score, état du projet |
| [Mécaniques de gameplay](https://github.com/KuchikiDev/SpaceLeague/wiki/Gameplay) | Contrôle de balle, enroulés, mobilité, événements terrain |
| [Personnages — Legends](https://github.com/KuchikiDev/SpaceLeague/wiki/Personnages) | Classes, roster, capacités (Raijin, Keplar…) |
| [Architecture technique](https://github.com/KuchikiDev/SpaceLeague/wiki/Architecture) | C++/Blueprint, modules, réplication réseau |
| [Méthode BMAD & Claude AI](https://github.com/KuchikiDev/SpaceLeague/wiki/BMAD%E2%80%90Claude) | Comment l'IA est utilisée dans le projet |
| [Roadmap](https://github.com/KuchikiDev/SpaceLeague/wiki/Roadmap) | 4 phases, tickets Jira, critères de validation |
| [Équipe](https://github.com/KuchikiDev/SpaceLeague/wiki/Equipe) | Byakuya & Neyru — rôles et contributions |

---

## 🎮 Le jeu en bref

**Space League** est un sport futuriste **3v3 compétitif** où deux équipes s'affrontent sans jamais franchir la ligne centrale. Tout se joue à distance : précision du tir, timing du contrôle de balle, coordination d'équipe.

| Méthode de score | Points | Comment |
|------------------|--------|---------|
| **Prison** | **2 pts** | Envoyer 2 adversaires simultanément en prison |
| **But flottant** | **1 pt** | Faire traverser la balle dans le but adverse |

> Imaginez un mélange entre **Rocket League**, **Valorant** et **Quidditch** — avec gravité modifiable, grappins et ultimes capables de retourner un match.

→ [Présentation complète sur le Wiki](https://github.com/KuchikiDev/SpaceLeague/wiki/Presentation)

---

## 🛠 Stack technique

| Technologie | Usage |
|-------------|-------|
| **Unreal Engine 5** | Moteur de jeu principal |
| **C++ (MSVC)** | Architecture core, réplication réseau, performance |
| **Blueprints UE5** | Logique visuelle, animations, polish |
| **Enhanced Input / GameplayTags** | Input moderne, classification des capacités |
| **Jira** | Gestion de projet, tickets LEAG-XXX, sprints |
| **Confluence** | Documentation technique et design |
| **Claude AI + BMAD** | Agents IA pour design, architecture, documentation |
| **Perforce** | Versioning des assets binaires (~27 GB) |
| **Git / GitHub** | Versioning du code, docs et assets légers |

→ [Architecture technique détaillée sur le Wiki](https://github.com/KuchikiDev/SpaceLeague/wiki/Architecture)

---

## 🤖 Claude AI + Méthode BMAD

Le projet utilise **Claude (Anthropic)** via le framework **BMAD** — une méthode qui orchestre plusieurs agents IA spécialisés : brainstorming de mécaniques, architecture C++, documentation vivante, gestion de projet Jira, revues de code.

| Agent | Rôle |
|-------|------|
| **Analyst** | Specs fonctionnelles, brainstorming gameplay |
| **Architect** | Décisions C++/Blueprint, migration technique |
| **PM** | Backlog Jira (56 tickets), roadmap 4 phases |
| **Dev** | Assistance implémentation C++ |
| **QA** | Risk profiles, plans de test, revues |

→ [Méthode BMAD complète sur le Wiki](https://github.com/KuchikiDev/SpaceLeague/wiki/BMAD%E2%80%90Claude)

---

## 👥 Équipe

| Membre | Rôle | Domaines |
|--------|------|----------|
| **Byakuya** | Lead Developer | C++, Blueprint, architecture, Jira, Confluence, workflow IA |
| **Neyru** | Game Designer & 3D Artist | Modélisation 3D, animations, game design, brainstorming, Blueprint → C++ |

→ [Profils complets sur le Wiki](https://github.com/KuchikiDev/SpaceLeague/wiki/Equipe)

---

## 📁 Structure des dossiers

```
Leagues/
├── 📂 ProjetUE5/MovementParadoxe/  → Projet UE5 principal (C++ + Blueprints)
│   ├── Source/                     → Code C++ source [Git]
│   ├── Config/                     → Configurations UE5 [Git]
│   └── Content/                    → Assets binaires [Perforce, ~17 GB]
├── 📂 Documentation/               → Specs, architecture, roadmap, reports
├── 📂 Logo/                        → Identité visuelle (AI, PNG)
├── 📂 HUD/                         → Maquettes d'interface
├── 📂 Model/                       → Modèles 3D (fichiers sources)
├── 📂 Map/                         → Concepts de niveaux
├── 📂 Brouillon/                   → Esquisses et concepts
└── 📂 web-bundles/                 → Agents BMAD
```

---

## 🗄 Versioning — Git + Perforce

| Sur Git (ce dépôt) | Sur Perforce |
|--------------------|--------------|
| Code C++ source | Content UE5 (~17 GB) |
| Configurations UE5 | Assets 3D sources (~9 GB) |
| Documentation | Plugins UE5 |
| Logos, HUD, maquettes | Binaires compilés |

> Perforce est le standard utilisé par Epic Games, EA et Ubisoft pour les assets binaires de jeux vidéo.

---

## 🚀 Lancer le projet

**Prérequis :** Unreal Engine 5.3+, Visual Studio 2022 *(Game development with C++)*, Windows 10/11

```bash
git clone https://github.com/KuchikiDev/SpaceLeague.git
cd ProjetUE5/MovementParadoxe
# Clic droit sur MovementParadoxe.uproject → "Generate Visual Studio project files"
# Double-cliquer sur MovementParadoxe.uproject
```

> Sans les fichiers `Content/` (sur Perforce), le projet s'ouvrira dans UE5 mais sans assets visuels. Pour accéder au projet complet, contacter l'équipe.

---

## ⚖️ Droits & Propriété intellectuelle

**© 2025-2026 Byakuya Corps — Tous droits réservés.**

Ce dépôt et son contenu sont la propriété exclusive de **Byakuya Corps** (Byakuya & Neyru).

**Autorisé :** consulter à des fins d'évaluation (recrutement, école, portfolio), partager le lien.

**Interdit sans autorisation écrite :** copier ou réutiliser le code/assets, distribuer, utiliser le nom ou logo Space League / Byakuya Corps.

**Assets tiers :** Unreal Engine 5 ([Epic Games EULA](https://www.unrealengine.com/en-US/eula/unreal)) · FeudalJapan Meshingun (Marketplace Epic Games)

Contact : [GitHub Issues](https://github.com/KuchikiDev/SpaceLeague/issues)

---

<div align="center">

*Space League — Byakuya Corps © 2025-2026*

*Développé avec Unreal Engine 5, C++, Blueprints, Jira, Confluence et Claude AI*

</div>

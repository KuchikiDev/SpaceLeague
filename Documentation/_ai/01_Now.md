---
type: now
status: active
last_review: 2026-09-30
owner: dylan
audience: humans-agents
source_of_truth: code-and-config
---

# 01 Now

Etat courant du projet au 2026-09-26.

## Resume executif

- Le jeu s'appelle **ORA** (ex-"Space League", renomme le 2026-05-10). Formats **2v2** et **3v3**.
- Le projet Unreal actif est `ProjetUE5/MovementORA 5.8/` : **Unreal Engine 5.8**, module C++ `MovementORA`, classes `AORA*`.
- Ce projet est versionne sur **Perforce** (stream `//SpaceLeague/main`, workspace `ORA-Byakuya`), pas dans Git.
  Git en garde une copie du code dans `ProjetUE5/MovementORA/` (Source, Config, plugin GameplayVariables),
  mise a jour par `Tools/Sync-ShowcaseFromPerforce.ps1`.
- `ProjetUE5/MovementParadoxe/` est l'ancien projet (UE 5.3, fevrier-avril 2026), conserve pour l'historique.

## Point de reprise (session)

- P0 et P2 de la revue du 2026-09-25 traites (voir ci-dessous). Prochain chantier : **capacites Raijin/Keplar** (P1-4),
  puis **evenements terrain** (rotation du terrain ; le deplacement automatique des buts existe deja).
- Le score "prison complete" (P1-3) est code et compile ; il reste a le tester en PIE avec 2 joueurs humains
  (les bots ne vont jamais en prison).

## A faire en priorite (mise a jour du 2026-09-30)

Fait et soumis sur Perforce :
- Mur (dash sur le mur, camera a l'arrivee sur un mur, collage apres un saut) : CL 155.
- Coyote time sur le saut (0,12 s, le double saut reste disponible) : CL 156.
- Suppression de 40 reglages GameplayVariables inutilises : CL 157. Le dash reste en ligne droite (pas de controle).
- Deja en place : balle en orbite mise a jour apres le mouvement, aide a la visee du grappin (zone de visee agrandie).
- Essai rejete : double appui rapide sur saut = saut vertical (retire, pas soumis).

- Assets en attente (redirecteurs, reenregistrements du 26/09) : CL 158. Avertissement E_Legend de DT_LegendDB corrige : CL 159.
- Prison complete testee en PIE : le dernier touche reste sur le terrain, les prisonniers reviennent tout de suite
  et la balle repart dans l'autre sens : CL 160.

Ensuite :
1. Rotation du terrain.
2. Capacites Raijin/Keplar.

## Faits verifies (code/config)

### Identite projet

- `MovementORA.uproject` : `EngineAssociation=5.8`, module `MovementORA`.
- `Config/DefaultEngine.ini` :
  - `GameDefaultMap` / `EditorStartupMap` = `/Game/Levels/SandboxMap/TerrainSandbox`
  - `GlobalDefaultGameMode=/Game/Levels/HUB/BP_GM_Hub.BP_GM_Hub_C`
  - `GameInstanceClass=/Game/Instance/GI_ORA.GI_ORA_C`
  - Redirections `SpaceLeague*` / `Paradoxe*` -> `ORA*` (classes et enums).

### Gameplay en place (C++)

- Deroule du match (`AORAGameState`) : attente -> intro -> decompte -> partie -> prolongation (mort subite) -> fin.
- Score : but (1 pt), regle anti-camping de la balle, **prison complete** (2 joueurs d'une equipe en prison en meme temps
  -> 2 pts pour l'adversaire, puis liberation). Reglages dans GameplayVariables, section `Match|Prison`.
- Prison de 10 s, grappin, dash/stamina, wall run / wall slide, stop ball + orbit aim, passes avec focus.
- Bots d'entrainement (`ORA/AI`), logs dans la categorie `LogORABot` (`log LogORABot Verbose` pour le detail).
- Authentification en ligne (`ORAAuthenticationSubsystem`, API `api.oragame.eu`), cible serveur dedie.

### Organisation du code (depuis le 2026-09-26)

- `ORACharacterBase` : `ORACharacterBase.cpp` (coeur) + `_Input`, `_Ball`, `_OrbitAim`, `_Dash`, `_Wall`.
- `ORACharacter` : `ORACharacter.cpp` (coeur, tir, visuels du grappin) + `_Input`, `_Movement`, `_Grapple`, `_Obstacles`.
- `ORAGameState` : `ORAGameState.cpp` (score, contacts balle, prison) + `_MatchFlow`, `_Terrain`, `_Abilities`.
- Les helpers internes (namespace anonyme) restent dans un seul fichier par classe (compatible build unity).

## Revue du 2026-09-25 : statut

| Point | Statut |
|-------|--------|
| P0-1 Identifiants Perforce en clair | Scripts corriges (login par ticket), `Perforce/config.txt` ignore. **A faire : changer le mot de passe P4** |
| P0-2 Slots de capacites Raijin/Keplar | Corrige (EnumRedirect + DA reenregistres) |
| P1-3 Score prison complete | Code, compile ; test PIE 2 joueurs a faire |
| P1-4 Capacites Raijin/Keplar | A faire |
| P1-5 Evenements terrain | Deplacement des buts deja fait ; rotation du terrain a faire |
| P2-6 Git / documentation | Copie du code ORA dans Git + doc a jour |
| P2-7 Script de build | `Build_MovementORA_Safe.bat` pointe vers `MovementORA 5.8` |
| P2-8 Gros fichiers C++ | Decoupes par domaine (build editeur, jeu et unity OK) ; logs bots dans `LogORABot` |
| P2-9 Nettoyage assets | Assets sans version reenregistres, cartes du MatchBoard mobiles, materiau Mannequin corrige, redirecteur `E_Legend` libere |

## Ecarts / dette restante

1. `Content/Game/Old` : `DT_LegendDB` et `S_Legends` restent utilises par `BP_ParadoxeTurnAround` (pion de test du sandbox) ;
   `BP_ParadoxeRaijin` garde une variable inutilisee `LegendData_0` de type `S_Legends`.
2. Redirecteurs sans reference a supprimer : `Game/Old/E_Legend`, `Game/Old/DataAsset/DA_Legend`,
   `Game/Old/DataAsset/Legend/DA_Keplar`, `Game/Old/DataAsset/Legend/DA_Raijin`, `/Game/BP_AffichePersonnage`.
3. PoseAssets du Mannequin (contenu d'exemple Epic) a regenerer depuis leurs animations.
4. `.p4ignore` ignore `**/__ExternalActors__/` : les acteurs externes des maps d'exemple (FeudalJapan, ThirdPerson) ne sont pas versionnes.

## Liens de travail

- [[_ai/00_Home]]
- [[_ai/AGENT_CONTEXT]]
- [[ROADMAP]]
- [[ARCHITECTURE]]
- [[SPECS_FONCTIONNELLES]]
- [[SPECS_TECHNIQUES]]

## Journal de mise a jour

- 2026-02-19: Creation du snapshot initial "Now" avec verification code/config.
- 2026-02-19: Health check, revue architecte, risk profile et test design Phase 0 (rapports dans `Documentation/reports/`).
- 2026-02-21: Cleanup LEAG-001/002 (`BP_ParadoxeJhin`, `BP_ParadoxeSenna` archives). LEAG-003 partiel.
- 2026-02-22: LEAG-004 resolu structurellement en UE Editor (references vers `Core GI` nettoyees).
- 2026-03-08: Integration C++ ability/HUD sur `SpaceLeagueCharacterBase` + `SpaceLeaguePlayerController`.
- 2026-03-12: `OrbitAim` consolide en C++ (courbure, clip obstacle, pool de `SplineMesh`).
- 2026-05-10: Renommage du jeu en ORA dans toute la documentation.
- 2026-06-28: Migration vers Unreal Engine 5.8 (`ProjetUE5/MovementORA 5.8`, module `MovementORA`), versionne sur Perforce.
- 2026-07 a 2026-09: grappin, bots, deroule du match, prison, anti-camping, authentification en ligne, serveur dedie.
- 2026-09-25: Revue de reprise (P0/P1/P2). Corrections P0-1, P0-2, P1-3, P2-7.
- 2026-09-26: P2 termines : decoupage C++, nettoyage assets, copie du code ORA dans Git, doc a jour.
- 2026-09-28: Retours de jeu : priorites dash sur le mur et camera a l'arrivee sur un mur.

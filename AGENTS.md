# Instructions pour les agents IA — ORA

Ce fichier s'applique a tout agent IA qui travaille dans ce depot (Claude Code, Codex, Cursor, Copilot, Gemini...).

## Ou est le vrai projet

- Jeu : **ORA** (ex-"Space League"), sport 2v2 / 3v3, **Unreal Engine 5.8**.
- Projet actif : `ProjetUE5/MovementORA 5.8/` (module C++ `MovementORA`, classes `AORA*`). C'est un lien vers `C:\UnrealProjects\MovementORA 5.8`.
- **Perforce est la source de verite** du projet (stream `//SpaceLeague/main`, workspace `ORA-Byakuya`).
- **Git est la vitrine** : il garde une copie du code dans `ProjetUE5/MovementORA/`, mise a jour par `Tools/Sync-ShowcaseFromPerforce.ps1`.
- Etat courant et priorites : `Documentation/_ai/01_Now.md`.

## Regle : chaque changement est verifie, commite et envoye tout de suite

Apres **chaque feature ajoutee, chaque fix, ou chaque changement important** (refactor, nettoyage d'assets, config), on ne cumule pas : on fait immediatement les trois etapes ci-dessous, dans cet ordre. Un changement = un envoi. On ne regroupe pas des changements sans rapport.

### 1. Verifier

- Code C++ modifie : compiler la cible editeur, Unreal ferme (ou Live Coding desactive).
  ```
  "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" MovementORAEditor Win64 Development "-Project=S:\Programmation\Jeu\UE5\Byakuya\ORA\ProjetUE5\MovementORA 5.8\MovementORA.uproject" -WaitMutex
  ```
- Assets / Blueprints modifies : les recompiler et verifier le log (`Saved/Logs/MovementORA.log`) : pas de nouvelle erreur.
- Gameplay modifie : tester le cas concerne (PIE, ou lancement sans rendu `-game -nullrhi` sur `TerrainSandbox`).
- **Si la verification echoue : on corrige, ou on n'envoie pas et on le signale.** Jamais de submit d'un projet qui ne compile pas.

### Raccourci : tout envoyer en une commande

`Tools/Envoyer.ps1` fait les etapes 2 et 3 d'un coup (changelist + reconcile de Source/Config/GameplayVariables, confirmation, submit, puis fusion de la branche `claude/*` dans `main` et push) :

```
powershell -ExecutionPolicy Bypass -File Tools\Envoyer.ps1
```

Options : `-Message`, `-Paths`, `-Branch`, `-Oui` (sans confirmation), `-SansGit`, `-SansPerforce`.

### 2. Perforce : submit

- Toujours lancer `p4` depuis `S:\Programmation\Jeu\UE5\Byakuya\ORA` (le `.p4config` y definit `P4PORT=127.0.0.1:1667`, un tunnel SSH vers le serveur).
- Creer un changelist qui ne contient **que** les fichiers du changement, puis le soumettre :
  ```
  p4 change            # description : type + quoi + pourquoi + verification faite
  p4 reconcile -c <CL> <fichiers ou dossiers du changement>
  p4 opened -c <CL>    # relire la liste avant d'envoyer
  p4 submit -c <CL>
  ```
- Format de description : `feat|fix|refactor|chore: resume court`, puis le detail et la verification faite (ex. "build editeur OK, test PIE 2 joueurs OK").
- Si `p4` repond "password invalid" ou "session expired" : demander a l'utilisateur de lancer `p4 login`. **Un agent ne tape, ne stocke ni ne change jamais un mot de passe.**
- Si l'agent n'a pas le droit de lancer les commandes `p4` d'ecriture, il donne a l'utilisateur les commandes exactes, avec le numero de changelist et la liste des fichiers.

### 3. Git : commit et push

- Mettre a jour la copie du code : `powershell -File Tools/Sync-ShowcaseFromPerforce.ps1` (depuis la racine du depot).
- Commiter (meme format de message que Perforce) puis pousser la branche courante : `git push origin HEAD`.
- Sur une branche de travail (worktree `claude/...` ou autre) : pousser la branche et ouvrir une pull request vers `main`.

## Ne jamais envoyer

- Identifiants et jetons : `Perforce/config.txt`, mots de passe, tickets p4, `SecurityToken`, cles d'API.
- Fichiers generes : `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/`.
- Fichiers de travail des agents : `.codex_tmp/`, `.codex_video_review/`, captures et scripts temporaires.

## Bon a savoir

- Nouvelle variable de tuning gameplay en C++ : l'exposer aussi dans le plugin `GameplayVariables` (voir `ProjetUE5/MovementORA 5.8/Documentation/GameplayVariablesConvention.md`).
- Le serveur blueprint-mcp (commandlet Unreal) verrouille les assets : l'arreter avant d'enregistrer des assets en ligne de commande.
- Les helpers en namespace anonyme restent dans un seul `.cpp` par classe (compatibilite build unity).

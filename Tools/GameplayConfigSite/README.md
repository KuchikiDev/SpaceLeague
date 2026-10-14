# Site des variables de gameplay

Petit site local pour regler les variables du plugin `GameplayVariables`, rangees par menu, avec un croquis par variable qui suit la valeur en direct.

- Lancer : double-clic sur `Lancer.bat` (ou `node Tools/GameplayConfigSite/server.js`), puis http://localhost:5178
- Les definitions (nom, categorie, bornes, aide) sont lues dans `Plugins/GameplayVariables/.../GameplayVariablesSettings.h` : une nouvelle variable apparait toute seule.
- **Enregistrer** reecrit la section `[/Script/GameplayVariables.GameplayVariablesSettings]` de `Config/DefaultGame.ini` (copie de secours `DefaultGame.ini.bak`). Relancer le PIE pour appliquer, puis envoyer le `.ini` comme d'habitude (`Tools/Envoyer.ps1`).
- Croquis : `sketches.js` (table `MAP` = variable -> croquis). Une variable sans croquis dedie affiche une jauge.
- `ORA_PROJECT` permet de pointer vers une autre copie du projet (tests).

# Convention GameplayVariables

Quand on ajoute ou modifie des variables de configuration gameplay en C++ dans ce projet:

1. la variable doit etre exposee dans le plugin `GameplayVariables`,
2. elle doit etre rangee dans une section existante si cette section est pertinente,
3. sinon il faut creer une nouvelle section claire, stable et facile a retrouver.

Objectif:

- centraliser les reglages de gameplay modifiables,
- eviter que des valeurs importantes restent dispersees uniquement dans des classes C++ ou des Blueprints,
- garder une organisation coherente dans `UGameplayVariablesSettings`.

Exemples:

- mouvement, dash, saut -> section existante correspondante,
- balle, tir, passe, enroule -> section `Balle` existante si elle reste lisible,
- si un ensemble devient trop gros ou trop specifique, creer une section dediee plutot que surcharger une categorie deja confuse.

Regle de travail pour les prochaines modifications:

- si Codex change une valeur de tuning gameplay ou ajoute un nouveau parametre editable, il doit aussi ajouter l'entree correspondante dans `Plugins/GameplayVariables/Source/GameplayVariables/Public/GameplayVariablesSettings.h`,
- et, si necessaire, brancher la lecture de cette variable dans le code runtime qui l'utilise.

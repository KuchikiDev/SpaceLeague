# ORA — POC du site univers

## État courant — édition 11, 14 septembre 2026

Release `20260914-05` publiée. Le site compte 26 adresses et neuf fiches 3D. Les sections d’éditions ci-dessous retracent l’évolution ; l’édition courante les remplace pour les contrôles techniques.

`npm run check` vérifie tous les modules sources et outils JavaScript ; `npm run build` régénère les destinations puis les fichiers gzip. Refaire le build après chaque modification de `dist/` pour éviter de servir un fichier compressé périmé. Nginx sert les fichiers précompressés, conserve la revalidation du cache et désactive la compression pour `/account-api/`.

42 fichiers texte précompressés : 796 695 → 233 240 octets (71 % de réduction sur l’ensemble). Le bundle Three.js limité aux 17 exports réellement utilisés passe de 539 965 à 135 788 octets ; l’ancien couple de modules génériques représentait 2 120 885 octets bruts et 418 036 octets compressés. Tous les fichiers texte servis, y compris ceux de moins de 1 Kio, sont vérifiés contre leur source après le build afin d’empêcher la publication d’un gzip périmé. Ce n’est pas une mesure de temps de chargement ni du volume de chaque page.

`animation-loop.js` suspend les callbacks canvas/WebGL en pause, en mouvement réduit et lorsque la page est masquée ; reprise sans saut temporel et nettoyage à la navigation. Contrôles : `verify-animation-lifecycle.mjs` (visibilité masquée simulée), `verify-model-motion.mjs`, `verify-navigation.mjs`, `verify-v6.mjs`, `verify-diagrams.mjs`, `verify-account.mjs`, `verify-account-public.mjs`, `verify-accessibility.mjs` et `verify-compression.mjs`. Le contrôle d’accessibilité structurelle parcourt les 26 routes et vérifie les identifiants, relations ARIA, noms des commandes et liens internes ; il ne remplace pas un audit humain avec lecteur d’écran. Le dernier contrôle utilise HTTPS et compare les corps normaux/décompressés sur le site public.

`npm run test:local` démarre et arrête automatiquement le serveur de prévisualisation, puis exécute les dix contrôles locaux actuels dans un ordre déterministe. `npm test` reste le contrôle rapide de syntaxe et d’intégrité du build.

Les releases et le manifeste sont inclus dans les sauvegardes ORA/R2. La restauration du manifeste et du schéma PostgreSQL de l’espace joueur a été vérifiée dans KAN-39. Limites inchangées : validation artistique, authentification réelle Steam complète et raccordement des données au jeu.

Site statique immersif en français, créé le 13 septembre 2026 à la demande de Dylan. URL : https://poc.oragame.eu.

## État

POC implémenté ; validation artistique et éditoriale par Dylan à effectuer. Les illustrations, le motif graphique ORA et les formulations éditoriales sont exploratoires. Aucune biographie, origine de l’énergie ou géographie précise n’est rendue canonique.

## Contenu et interactions

- Accueil cinématique avec parallaxe légère et particules.
- Atlas avec trois repères, déplacement de cadrage et navigation au clavier.
- Quatre fiches personnages : identité, histoire encore ouverte, forces, vulnérabilités, aptitudes. Liens directs `/#personnage/raijin`, `keplar`, `andaris`, `pandore` ; boutons précédent/suivant du navigateur conservés.
- Roster complet des neuf noms documentés.
- Trois chapitres sur l’isolement, la reconnexion et l’ORA.
- Mise en page responsive ; dialogues natifs avec focus, Échap et retour au déclencheur ; réduction des mouvements selon la préférence système et le bouton du site.

Le contenu établi vient de Confluence ORA : [Univers et civilisations](https://byakuyacorps.atlassian.net/wiki/spaces/SL/pages/248021008), [Personnages](https://byakuyacorps.atlassian.net/wiki/spaces/SL/pages/248217602), [Raijin](https://byakuyacorps.atlassian.net/wiki/spaces/SL/pages/249528329), [Keplar](https://byakuyacorps.atlassian.net/wiki/spaces/SL/pages/249823241), [Andaris](https://byakuyacorps.atlassian.net/wiki/spaces/SL/pages/249724930), [Pandore](https://byakuyacorps.atlassian.net/wiki/spaces/SL/pages/249888769).

## Développement

Node.js 22+ : `npm ci`, `npm run build`, puis `npm run dev` et ouvrir http://127.0.0.1:4173. `dist/` contient le livrable public ; la source du modèle 3D est dans `src/character-model.mjs`, les autres modules publics restent directement éditables dans `dist/`. Le build extrait le sous-ensemble Three.js, régénère les routes et les fichiers gzip, puis vérifie leur cohérence. `npm run check` vérifie la syntaxe. Les contrôles de rendu utilisent le Playwright du runtime local Codex ; son chemin doit être adapté sur une autre machine. `ORA_TEST_URL` permet de cibler un autre déploiement.

Images générées originales conservées dans `../Artifacts/ora-site-art/` ; les deux anciens WebP, remplacés par les scènes procédurales et non chargés depuis l’édition 03, ont été retirés du livrable. Polices auto-hébergées Barlow Condensed et Manrope sous SIL Open Font License, textes de licence inclus. Aucun appel à un fournisseur tiers depuis le site. Pas d’analytics ; seule la préférence d’animation est enregistrée localement dans le navigateur. L’espace joueur et ses données sont décrits séparément dans `../api/README-WEB-PLAYER.md`.

## Hébergement et restauration

- Propriétaire : Dylan / ORA.
- Nginx existant du VPS, ports HTTP/HTTPS existants, proxy Cloudflare et certificat d’origine existant. Aucun processus applicatif supplémentaire.
- Répertoire `/opt/ora/site/releases/<version>`, lien atomique `/opt/ora/site/current`, manifeste `/opt/ora/site/release-manifest.json`, accès central `/srv/services/ora/site`.
- Seul `dist/` est publié. Scripts, QA et documentation ne sont pas servis.
- `deploy-site.sh` vérifie le SHA-256 du paquet, prépare une nouvelle release, teste Nginx, recharge et teste l’origine. En cas d’échec, il restaure la configuration précédente et le lien précédent lorsqu’ils existent.
- Les releases sont conservées et la configuration remplacée est sauvegardée dans `/opt/ora/site/backups/`. La première release n’a pas de prédécesseur : son rollback retire uniquement le nouveau virtual host.
- Données persistantes : aucune donnée métier. Le site est reproductible depuis les sources et images locales ; les releases sur le VPS constituent une copie supplémentaire. L’intégration à la sauvegarde automatique hors site n’est pas présumée.
- Journaux : `/var/log/nginx/ora-poc.access.log` et `ora-poc.error.log`, rotation Nginx existante. Contrôle : réponse HTTPS 200 et chargement des ressources ; aucune nouvelle automation de surveillance.
- Pour une migration de domaine : modifier uniquement `server_name`, la redirection HTTPS, le certificat si nécessaire, le DNS et le champ host du manifeste/script. Les liens du site sont relatifs à l’origine.
- POC publiquement accessible mais exclu de l’indexation via robots, métadonnée et en-tête HTTP. `noindex` n’est pas un contrôle d’accès.

Suivi : [KAN-38](https://byakuyacorps.atlassian.net/browse/KAN-38).

## Révision du 13 septembre — édition 02

La page à long défilement est remplacée, à la demande de Dylan, par un carrefour et des destinations distinctes : `/univers/`, `/personnages/`, `/personnages/raijin/` (et les trois autres personnages), `/histoire/?chapitre=1`.

Une seule scène est montée à la fois. L’atlas se déplace au pointeur ou au clavier et propose zoom/recentrage. Les personnages disposent d’un écran de sélection, de trois onglets et d’aptitudes dépliables. Les chroniques se parcourent avec précédent/suivant et une ligne de chapitres. Sur téléphone, seul le contenu de la destination courante peut défiler pour rester lisible.

Après modification du shell `dist/index.html`, exécuter `node build-routes.mjs` pour régénérer les sept entrées statiques. Le contrôle courant est `node verify-v2.mjs` ; `verify.mjs` documente uniquement les tests de la première version et ses sélecteurs ne correspondent plus à l’édition 02. Dernière release : `20260913-04`.

## Édition 03 — 13 septembre 2026

Release publique : 20260913-05. Les images générées ne sont plus affichées ni chargées. Le module dist/ambient.js dessine des orbites, trajectoires lumineuses et particules autonomes à cadence limitée, avec nettoyage à chaque changement de scène. Signatures typographiques pour les personnages, pulsations et tracés CSS. La pause et la préférence système de réduction du mouvement restent actives.

Vérification : node verify-v2.mjs (navigation), node verify-v3.mjs (mouvement autonome, pause, canvas unique, absence de requêtes images, ordinateur/mobile). Le second script a également réussi contre https://poc.oragame.eu via ORA_TEST_URL. Documentation Confluence et KAN-38 mis à jour ; validation artistique ouverte.

## Édition 04 — 13 septembre 2026
Release 20260913-06 : noms contrastés et dégagés ; îles flottantes facettées et arène stylisée en canvas avec lévitation autonome. Géométrie exploratoire. Tests verify-v2 locaux et verify-v3 locaux/publics réussis. Confluence et KAN-38 actualisés.

## Édition 05 — 13 septembre 2026
Release 20260913-07 : symboles SVG à la place des initiales ; repères sans numéros attachés aux îlots dans la surface de carte, coordonnées et lévitation partagées. Terrain recentré et réduit, projection ajustée, cercle central isolé du tracé médian. Tests verify-v2 locaux et verify-v3 locaux/publics réussis. Confluence et KAN-38 actualisés.

## Éditions 06 et 07 — 13/14 septembre 2026
Relecture des 24 pages actives ORA et de l’historique : voir CONTENT-AUDIT.md. Ajout de 14 destinations documentaires (jeu, règles, mouvements, arène, rôles, civilisations, ORA, carnet, roster, cinq personnages), puis quatre pages de compte. Release 20260914-01, 26 adresses avec l’accueil.

L’espace joueur repose sur les routes /account-api du backend existant. Connexion Steam, profil, XP/niveau, lecture des données ELO/rang/points/collection et historique privé. Les systèmes sans données sont explicitement vides ; aucune attribution fictive. Voir ../api/README-WEB-PLAYER.md pour les limites (alimentation jeu, catalogue, barème, liaison invité, validation réelle Steam).

Tests actuels : verify-navigation.mjs, verify-v3.mjs, verify-v6.mjs, verify-account.mjs et verify-account-public.mjs. verify-v2.mjs conserve le parcours historique à quatre personnages/dialogue.

## Édition 08 — 14 septembre 2026

Release `20260914-02` publiée sur https://poc.oragame.eu. `dist/diagrams.js` fournit 24 scènes SVG distinctes pour les sujets du jeu et du carnet ; animations autonomes, pause et mouvement réduit. Le fond tactique commun est retiré de ces guides. Géométrie illustrative, règles documentées conservées, aucune capacité inventée pour les rôles.

Validation locale et publique : `node verify-diagrams.mjs` (24 scènes, limites du texte SVG, animation/pause, débordement mobile) et `node verify-v6.mjs` (guides, score, roster et neuf fiches). Captures dans `qa/diagrams`. Confluence POC/historique et KAN-38 mis à jour. L’API compte et ses limites restent celles de l’édition 07.

## Édition 09 — Personnages en 3D et contres

Release `20260914-03` publiée. Neuf études 3D procédurales dans `dist/character-model.js`, à la place du terrain des personnages. Apparences exploratoires autorisées pour le POC, à valider ; aucun modèle définitif du jeu ou skin revendiqué. Rotation glisser/clavier, recentrage, animation de repos, pause, mouvement réduit et message si WebGL est indisponible. Ressources libérées lors de la navigation.

`dist/tactics.js` : rappel et onglet Contres, réponses mécaniques, synergies, zone supérieure et paramètres ouverts. Sources Confluence : Raijin 249528329, Keplar 249823241, Andaris 249724930, Pandore 249888769, roster 249856008. Les cinq kits ouverts ne reçoivent pas de matchups inventés.

Three.js 0.186.0 est fixé dans package-lock.json. `npm run build` utilise esbuild pour extraire uniquement le sous-ensemble réellement employé par les silhouettes, chargé à la demande sur une fiche personnage ; la licence MIT reste copiée dans `dist/vendor`. `node sync-three.mjs` est conservé comme alias de compatibilité. Documentation technique consultée : https://threejs.org/docs/pages/WebGLRenderer.html et https://threejs.org/manual/en/how-to-dispose-of-objects.html.

Tests locaux/publics réussis : `node verify-characters-3d.mjs`, `node verify-model-motion.mjs`, `node verify-v6.mjs`. Captures dans qa/characters-3d. Confluence POC, historique et KAN-38 à jour. L’espace compte conserve ses limites documentées.

## Édition 10 — Sélecteur personnages simplifié

Release `20260914-04` publiée. Les pictogrammes ont été retirés à côté des noms dans le sélecteur des fiches personnages. Les trois noms du rôle sont centrés, avec un espacement adapté aux écrans ordinateur et mobile. Vérification locale et publique : `node verify-character-names.mjs`.

## Édition 12 — Arène & archipel scellé

Refonte visuelle et couche d'énigmes sur la base de l'édition 11 locale. **Publiée le 25 septembre 2026, release `20260925-02`** (la `-01` a été remplacée pour forcer le rafraîchissement du cache). Les 25 destinations existantes, l'espace joueur Steam, les fiches 3D et les contenus éditoriaux sont conservés.

- **HUD d'arène.** Le header devient un tableau de score : deux camps (univers / jeu) séparés par la ligne médiane ; la balle d'ORA se pose sur la ligne au carrefour puis glisse vers la destination courante. Panneaux vitrés à coins de visée, grain, dégradé aura cuivre → cyan, onglets en capsules, aptitudes présentées comme des icônes de sort avec anneau de recharge, balle sur la frise des chroniques.
- **Trajectoires.** Chaque navigation lance une balle qui rebondit sur les bords de l'écran (`dist/trajectory.js`) et la destination s'ouvre depuis le point choisi. Un curseur-balle suit le pointeur et s'ouvre en orbite sur les éléments interactifs. Sur l'accueil et l'atlas, une balle rebondit en continu dans l'arène de l'îlot (`ambient.js`). Tout s'efface avec le bouton de mouvement réduit.
- **L'archipel scellé** (`/enigmes/`, `dist/enigmes.js`, `dist/puzzles.js`). Neuf îlots, un par personnage, chacun avec une énigme inspirée de son Type documenté : Foudre (ligne directe, timing), Galaxie (Distorsion, courbures), Observation (prédire un rebond), Malédiction (propagation des marques), Réflexion (miroirs), Lumière (révéler un mot), Origine (état initial), Temps (cadrans à aligner avec les Types du roster), Polarité (aimants). Chaque sceau brisé relie son îlot au cœur ; les neuf ouvrent le sceau d'ORA, dont la réponse vient de `/univers/ora/`, et débloquent le mode Aura. Les niveaux déterministes ont une solution unique vérifiée par recherche exhaustive. Les indices renvoient aux pages du site. Aucune mécanique, valeur d'équilibrage ou élément de lore n'est inventé.
- **Données.** Progression dans `localStorage` (`ora-archipel`) uniquement, jamais envoyée à l'API compte ; mention ajoutée au dialogue d'information.

Contrôles : `node verify-enigmes.mjs` résout les neuf énigmes comme un joueur, éveille le sceau, vérifie le mode Aura, la persistance, la réinitialisation et le mobile. Il est ajouté à `npm run test:local` (11 contrôles). `verify-accessibility.mjs` couvre 30 adresses, dont l'archipel, deux énigmes et le sceau central. Tablette (761–1024 px) : la navigation passe dans le menu.

**Cache.** Cloudflare impose aux navigateurs un cache de 4 h sur les fichiers statiques (`max-age=14400`), au-delà du `expires 5m` de Nginx. Les fichiers modifiés par une édition portent donc un paramètre de version : `/style.css?v=12` et `/app.js?v=12` dans `index.html`, `./ambient.js?v=12` et `./character-model.js?v=12` dans `app.js`. Il faut l'incrémenter à chaque édition qui les modifie. Les nouveaux modules n'en ont pas besoin. Ne pas versionner un module importé par plusieurs fichiers, pour ne pas le dupliquer. `nginx-poc.conf` est aligné sur la configuration active du VPS (cache 5 min pour JS/CSS, 30 jours pour `/assets/`).

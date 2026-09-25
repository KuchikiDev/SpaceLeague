# Revue documentaire — 13 septembre 2026

Revue de l’accueil ORA et de ses 23 pages descendantes (arbre complet retourné par Confluence, sans pagination restante), complétée par une recherche Rovo. L’historique a été relu jusqu’aux premières décisions. Les archives Space League sont exclues des références actuelles conformément à AGENTS.md.

## Pages consultées

| Page | ID Confluence |
| --- | --- |
| Accueil du projet | 247889921 |
| Vision et piliers | 247857162 |
| Règles du match | 247857170 |
| Balle, mouvements et interactions | 247889936 |
| Arène, buts et obstacles | 247889944 |
| État actuel du prototype | 248283146 |
| Personnages jouables | 248217602 |
| Roster, rôles et principes de draft | 249856008 |
| Raijin | 249528329 |
| Keplar | 249823241 |
| Andaris | 249724930 |
| Pandore | 249888769 |
| Parcours joueur et multijoueur | 248152076 |
| Univers et civilisations | 248021008 |
| Historique des changements | 249528321 |
| Architecture technique et multijoueur | 250413148 |
| Serveur de jeu dédié UE5 | 250544164 |
| Backend, API et base de données | 250609685 |
| Référence des appels API | 250413173 |
| Infrastructure VPS, domaine et sécurité | 250544187 |
| Inventaire central du VPS et des services | 252542993 |
| Exploitation, sécurité et sauvegardes du VPS | 254345225 |
| Audit technique et feuille de route — septembre 2026 | 258211842 |
| Site univers — POC immersif | 260702219 |

## Adaptation au site

| Destination | Sources principales |
| --- | --- |
| /jeu/ | 247857162, 247889921 |
| /jeu/regles/ | 247857170 |
| /jeu/mouvements/ | 247889936 |
| /jeu/arene/ | 247889944 |
| /jeu/roles/ et /personnages/roster/ | 249856008, 248217602 |
| /personnages/obsidia/, aurion/, vaalbara/, chronis/, magnora/ | 249856008 : noms, Types et rôles uniquement ; kits non définis |
| /univers/civilisations/ et /univers/ora/ | 248021008 |
| /developpement/ | 248283146, 248152076, historique 249528321 du 10 au 13 septembre |

Les quatre fiches existantes restent fondées sur leurs pages individuelles. Les nouvelles accroches, symboles et schémas sont une adaptation éditoriale exploratoire. Aucun pouvoir supplémentaire, ville, biographie ou origine canonique n’est ajouté. Les valeurs de score et durées restent des bases de test. Le schéma et l’exemple de score du site ne simulent pas un match complet.

Les pages techniques ont été relues pour distinguer les états récents des sections anciennes. Leur contenu opérationnel, les chemins internes, les détails d’infrastructure et les routes privées ne sont pas publiés sur le site.

## Contradictions repérées

- Personnages jouables affirme qu’aucune capacité ne fonctionne, alors que l’état du prototype et l’historique documentent une validation locale des sorts et cooldowns le 13 septembre. Cela ne valide pas tous les kits ni leur réseau.
- État actuel du prototype évoque encore le risque de copie non versionnée ; le passage dans Perforce est documenté le 9 septembre, restauration sur workspace vierge restant à vérifier.
- Référence API affirme API non implémentée et propose des routes /api ; Backend décrit le contrat /v1 effectivement déployé le 4 septembre. Les premières propositions doivent être identifiées comme historiques.
- Architecture conserve des tableaux de préparation serveur/API devenus obsolètes ; les pages spécialisées décrivent l’implémentation.
- Infrastructure et sauvegardes conservent des états historiques root/R2 non activé : la synthèse actuelle doit renvoyer aux mises à jour ultérieures, sans prétendre à un nouvel audit du serveur dans cette tâche.

Ces clarifications sont documentaires ; aucun statut de ticket gameplay ni aucune règle d’équilibrage n’est validé par la livraison du site.


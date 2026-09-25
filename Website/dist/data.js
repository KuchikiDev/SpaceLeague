export const characters = {
  raijin: {
    name: 'Raijin', type: 'Foudre', role: 'Assaillant', position: '0%', color: '#9fdfe1',
    tagline: 'La pression de l’instant.',
    identity: 'Raijin impose une pression immédiate. Il cherche la ligne directe, accumule de l’énergie grâce à ses contres et réduit les possibilités de déplacement adverses. Face à lui, une ouverture suffit à changer le rythme d’un échange.',
    editorial: 'Contrer. Charger. Ouvrir une ligne. Frapper au bon instant.',
    strength: 'Transformer une ligne ouverte en menace et limiter les repositionnements par grappin.',
    weakness: 'Un obstacle solide ou un contre correctement exécuté peut arrêter son offensive, même pendant son Ultime.',
    state: 'Base validée · équilibrage à tester',
    abilities: [
      ['Passif', 'Surcharge', 'Chaque contre réussi donne une Charge. Une fois le seuil atteint, Raijin peut consommer ses Charges pour accélérer fortement un tir obligatoirement droit. La balle retrouve sa vitesse précédente au premier contact avec un joueur, un contre ou un obstacle solide.'],
      ['Sort', 'Électrisation', 'Électrise temporairement plusieurs obstacles blancs grabables adverses. Ils ne peuvent plus être saisis au grappin, mais la balle continue de les traverser. Le Sort reste utilisable depuis la zone d’élimination.'],
      ['Ultime', 'Surtension', 'Raijin choisit un adversaire. Son prochain tir corrige progressivement sa trajectoire vers cette cible. La balle reste contrable, conserve ses collisions et ne peut pas effectuer de virage impossible.']
    ]
  },
  keplar: {
    name: 'Keplar', type: 'Galaxie', role: 'Assaillant', position: '33.3333%', color: '#bcb0ed',
    tagline: 'L’espace est une possibilité.',
    identity: 'Keplar manipule l’espace pour construire des trajectoires que les autres ne peuvent normalement pas produire. Il prépare ses possessions, accumule de l’Expansion puis transforme la courbure de ses tirs en un avantage inattendu.',
    editorial: 'Une ligne ne raconte qu’une partie du chemin.',
    strength: 'Contourner les placements et les obstacles grâce à plusieurs changements de courbure dans un seul tir.',
    weakness: 'Ses trajectoires demandent de la préparation. Un changement tardif du terrain peut déjouer son plan.',
    state: 'Base validée · équilibrage à tester',
    abilities: [
      ['Passif', 'Expansion', 'Keplar accumule progressivement de l’Expansion au fil de ses possessions. Conservée entre les possessions, cette ressource augmente la courbure possible avec Distorsion. Elle revient à zéro quand Distorsion est consommée.'],
      ['Sort', 'Distorsion', 'Le prochain tir peut suivre une trajectoire avec plusieurs changements successifs de courbure, par exemple droite, gauche, puis droite. Son comportement depuis la zone d’élimination reste à définir.'],
      ['Ultime', 'Gravité instable', 'La gravité du camp adverse change périodiquement entre différentes surfaces, avec un avertissement avant chaque changement. La balle, le but, les obstacles, Keplar et son allié ne sont pas affectés.']
    ]
  },
  andaris: {
    name: 'Andaris', type: 'Observation', role: 'Assaillant', position: '66.6667%', color: '#eab875',
    tagline: 'Un échange d’avance.',
    identity: 'Andaris accumule de l’information. Elle lit la préparation adverse et aide son équipe à anticiper les événements du match. Son avantage tient à son regard : comprendre l’échange avant que les autres n’en voient les conséquences.',
    editorial: 'Voir une possibilité. Laisser à son équipe le temps de la saisir.',
    strength: 'Lire les adversaires qu’elle contre et offrir à son binôme une fenêtre d’anticipation stratégique.',
    weakness: 'L’information ne remplace pas le geste. Éliminer un adversaire analysé réinitialise l’Analyse accumulée sur lui.',
    state: 'Concept validé · paramètres à définir',
    abilities: [
      ['Passif', 'Analyse', 'Chaque contre ajoute de l’Analyse au tireur adverse. Au seuil prévu, Andaris voit quel joueur est le plus proche de sa trajectoire en préparation. L’indication évolue avec la trajectoire ; l’Analyse est réinitialisée si cet adversaire est éliminé.'],
      ['Sort', 'Vision persistante', 'Pendant une durée à définir, Andaris continue de voir la trajectoire préparée par l’adversaire analysé après le départ de la balle. Le partage éventuel de cette information avec son allié reste ouvert.'],
      ['Ultime', 'Prémonition', 'Andaris et son allié voient les temps de recharge et la disponibilité des Ultimes adverses, le prochain emplacement du but et les prochains changements des éléments dynamiques. Les futurs emplacements des obstacles blancs grabables ne sont pas révélés.']
    ]
  },
  pandore: {
    name: 'Pandore', type: 'Malédiction', role: 'Pilier', position: '100%', color: '#d2a4d8',
    tagline: 'Chaque échange laisse une trace.',
    identity: 'Pandore transforme les défenses réussies en punition différée. Elle ne retire pas les commandes de ses adversaires : elle rend leur prochaine élimination plus coûteuse. La pression qu’elle installe se révèle au fil des échanges.',
    editorial: 'Une défense aujourd’hui. Une conséquence demain.',
    strength: 'Punir les offensives répétées et prolonger une supériorité numérique lors d’une élimination maudite.',
    weakness: 'La malédiction ne donne aucun avantage immédiat sans élimination. Le retour collectif adverse efface les malédictions restantes.',
    state: 'Passif conceptuel · kit à compléter',
    abilities: [
      ['Passif', 'Sceau de Pandore', 'Les adversaires accumulent individuellement des marques Maudit lorsque Pandore contre leurs tirs, selon la condition actuellement retenue. Ces marques allongent leur prochaine élimination, puis sont consommées. Le retour collectif d’une équipe réinitialise ses malédictions.'],
      ['Sort', 'À concevoir', 'Le Sort devra correspondre à son identité de Malédiction et à son rôle de Pilier, tout en restant utile depuis la zone d’élimination.'],
      ['Ultime', 'À concevoir', 'L’Ultime devra produire un événement identifiable et modifier temporairement une règle ou une situation du match. Son fonctionnement reste ouvert.']
    ]
  }
};

export const fragments = [
  ['FRAGMENT 01 — FONDATION ÉTABLIE', 'Des îlots, des mondes.', 'Des villes s’élèvent sur des îlots flottants séparés dans le ciel. L’isolement a laissé à chacune le temps de suivre sa propre voie.'],
  ['FRAGMENT 02 — FONDATION ÉTABLIE', 'À chacun son temps.', 'Chaque ville possède sa propre temporalité et une civilisation inspirée de différentes cultures. Architecture, technologie, vêtements et pratique du sport expriment sa singularité.'],
  ['FRAGMENT 03 — PISTE D’UNIVERS', 'Une énergie ancienne.', 'L’ORA est envisagée comme l’énergie qui relie pouvoirs, arènes et transformations du terrain. Son origine et son lien éventuel avec la lévitation des îlots restent à définir.']
];
export const chapters = [
  ['I', 'LE TEMPS DE L’ISOLEMENT', 'Chacun son horizon.', 'Pendant longtemps, les civilisations des îlots ont vécu séparées. Chaque ville a développé sa culture, son rythme et sa manière d’habiter le ciel. Un même monde, sans histoire commune.', 'Fondation établie de l’univers'],
  ['II', 'LE TEMPS DE LA RENCONTRE', 'Les horizons se rejoignent.', 'Un événement récent a relié les civilisations jusqu’alors séparées. Sa nature n’est pas encore fixée. Ce passage de l’isolement à la rencontre constitue le point de départ de l’histoire d’ORA.', 'Reconnexion établie · nature de l’événement à définir'],
  ['III', 'UNE QUESTION ANCIENNE', 'Que porte le nom d’ORA ?', 'Son nom s’inspire de l’aura. Énergie naturelle, héritage d’une civilisation disparue ou découverte nouvelle : plusieurs pistes restent ouvertes. Le sport futuriste et cette énergie ancienne forment ensemble la direction de l’univers.', 'Origine de l’ORA et raison de la compétition en exploration']
];



#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayVariablesSettings.generated.h"

// Convention projet:
// Toute nouvelle variable de tuning gameplay ajoutee en C++ comme config editable
// doit aussi etre exposee ici dans le plugin GameplayVariables.
// Si une section adaptee existe deja, on y ajoute la variable.
// Sinon, on cree une nouvelle section claire et coherente.

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Variables de gameplay"))
class GAMEPLAYVARIABLES_API UGameplayVariablesSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UGameplayVariablesSettings();

	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
#endif

public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Match|Introduction", meta = (DisplayName = "Activer la cinematique d'introduction", ToolTip = "Affiche la vue aerienne, le zoom et le fondu avant le decompte 3-2-1. Desactivez-la pour arriver directement au decompte pendant les tests."))
	bool bEnablePreMatchIntro = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Match|Introduction", meta = (DisplayName = "Duree de la cinematique", ClampMin = "0.1", UIMin = "0.1", Units = "s", ToolTip = "Duree totale de la vue aerienne avant l'apparition du joueur et du decompte."))
	float PreMatchIntroDurationSeconds = 5.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Match|Introduction", meta = (DisplayName = "Duree du decompte", ClampMin = "0", ClampMax = "10", UIMin = "0", UIMax = "10", Units = "s", ToolTip = "Nombre de secondes affichees avant le debut officiel du match. Une valeur de 3 affiche 3, 2, 1."))
	int32 PreMatchCountdownSeconds = 3;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Match|Initialisation", meta = (DisplayName = "Joueurs requis en reseau", ClampMin = "1", ClampMax = "4", UIMin = "1", UIMax = "4", ToolTip = "Nombre de joueurs humains que le serveur attend avant de lancer l'introduction. Ignore en mode solo. Utilisez 4 pour un vrai match 2c2."))
	int32 RequiredNetworkPlayersToStart = 4;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Match|Prison", meta = (DisplayName = "Prisonniers pour une prison complete", ClampMin = "1", ClampMax = "3", UIMin = "1", UIMax = "3", ToolTip = "Nombre de joueurs d'une meme equipe qui doivent etre en prison en meme temps pour donner les points de prison complete a l'adversaire. 2 en 2c2 comme en 3c3."))
	int32 PrisonCompletePlayerCount = 2;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Match|Prison", meta = (DisplayName = "Points d'une prison complete", ClampMin = "1", UIMin = "1", ToolTip = "Points marques par l'equipe adverse quand la prison est complete."))
	int32 PrisonCompletePoints = 2;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Match|Prison", meta = (DisplayName = "Delai de liberation apres prison complete", ClampMin = "0.0", UIMin = "0.0", Units = "s", ToolTip = "Temps entre les points de prison complete et la liberation des prisonniers concernes. 0 libere immediatement."))
	float PrisonCompleteReleaseDelaySeconds = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category = "General", meta = (DisplayName = "Début de match", ClampMin = "0", UIMin = "0", ToolTip = "Valeur initiale du compteur de match, en secondes, au lancement de Play."))
	int32 MatchStartSeconds = 300;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Terrain|Spawn", meta = (DisplayName = "Hauteur max de spawn des obstacles", ClampMin = "0.0", UIMin = "0.0", Units = "cm", ToolTip = "Hauteur maximale, en centimetres, a laquelle les obstacles peuvent apparaitre."))
	float ObstacleMaxSpawnHeight = 5000.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Terrain|Spawn", meta = (DisplayName = "Hauteur des piliers", ClampMin = "1.0", UIMin = "1.0", Units = "cm", ToolTip = "Hauteur visuelle totale des piliers volants. Leur centre reste place a mi-hauteur du terrain."))
	float PillarMaxSpawnHeight = 5000.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Terrain|Obstacles temporaires", meta = (DisplayName = "Duree visible", ClampMin = "0.1", UIMin = "0.1", Units = "s", ToolTip = "Temps pendant lequel les obstacles temporaires restent visibles avant de disparaitre."))
	float TimedObstacleStateDurationSeconds = 10.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Terrain|Obstacles temporaires", meta = (DisplayName = "Delai avant reapparition", ClampMin = "0.1", UIMin = "0.1", Units = "s", ToolTip = "Temps pendant lequel les obstacles temporaires restent caches avant de reapparaitre a de nouveaux emplacements."))
	float TimedObstacleHiddenDurationSeconds = 3.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Terrain|Obstacles temporaires", meta = (DisplayName = "Taille des obstacles", ClampMin = "0.1", UIMin = "0.1", ToolTip = "Echelle uniforme des obstacles temporaires qui bloquent les ballons."))
	float TimedObstacleScale = 3.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Deplacement", meta = (DisplayName = "Vitesse de marche", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse de deplacement de base quand le joueur court normalement."))
	float BaseWalkSpeed = 2500.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Deplacement", meta = (DisplayName = "Acceleration max", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Acceleration horizontale appliquee au personnage."))
	float MaxAcceleration = 9000.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Deplacement", meta = (DisplayName = "Freinage", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Freinage applique quand le joueur relache le deplacement."))
	float BrakingDeceleration = 2048.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Deplacement", meta = (DisplayName = "Controle aerien", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Part du controle du joueur conservee en l'air."))
	float AirControl = 1.3f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Deplacement|Elan", meta = (DisplayName = "Frottement en l'air", ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "2.0", ToolTip = "Perte de vitesse horizontale en l'air. Bas = l'elan d'un dash, d'un grappin ou d'un saut se conserve longtemps."))
	float AirMomentumFriction = 0.15f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Deplacement|Elan", meta = (DisplayName = "Perte d'elan au sol", ClampMin = "0.0", UIMin = "0.0", Units = "cm/s", ToolTip = "Vitesse perdue par seconde quand le joueur court au-dessus de sa vitesse max (apres un atterrissage, un dash ou un grappin). 0 = l'ancien comportement (vitesse coupee net par la friction)."))
	float GroundMomentumDecay = 3500.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Dash", meta = (DisplayName = "Impulsion du dash", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Force principale appliquee lors d'un dash."))
	float DashImpulse = 8000.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Dash", meta = (DisplayName = "Duree du dash", ClampMin = "0.01", UIMin = "0.01", ToolTip = "Duree pendant laquelle le dash reste actif."))
	float DashDurationSeconds = 0.2f;

	UPROPERTY(Config, EditAnywhere, Category = "Dash", meta = (DisplayName = "Cooldown du dash", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Temps necessaire avant de relancer un dash."))
	float DashCooldownSeconds = 0.2f;

	UPROPERTY(Config, EditAnywhere, Category = "Dash", meta = (DisplayName = "Controle pendant dash", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Part du controle de direction conservee pendant le dash."))
	float DashSteeringRatio = 0.2f;

	UPROPERTY(Config, EditAnywhere, Category = "Saut", meta = (DisplayName = "Nombre max de sauts", ClampMin = "1", UIMin = "1", ToolTip = "Nombre maximum de sauts consecutifs autorises."))
	int32 MaxJumpCount = 2;

	UPROPERTY(Config, EditAnywhere, Category = "Saut", meta = (DisplayName = "Vitesse de saut", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse verticale donnee au joueur au moment du saut."))
	float JumpVelocity = 3500.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Saut", meta = (DisplayName = "Coyote time", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Fenetre de coyote time pour accepter un saut juste apres avoir quitte le sol."))
	float CoyoteTimeSeconds = 0.12f;

	UPROPERTY(Config, EditAnywhere, Category = "Balle", meta = (DisplayName = "Rayon de controle", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Distance a laquelle la balle peut etre detectee et attrapee plus facilement."))
	float BallControlRadius = 520.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Balle", meta = (DisplayName = "Marge zone de touche joueur", ClampMin = "0.0", UIMin = "0.0", Units = "cm", ToolTip = "Petite marge ajoutee autour de la capsule du joueur pour determiner si un tir le touche. Cette zone est independante de la taille visuelle de la balle."))
	float BallPlayerHitMargin = 30.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Balle", meta = (DisplayName = "Marge verticale zone de touche joueur", ClampMin = "0.0", UIMin = "0.0", Units = "cm", ToolTip = "Prolonge uniquement le sommet de la capsule de detection afin que les grosses balles passant juste au-dessus du personnage le touchent aussi."))
	float BallPlayerHitVerticalMargin = 120.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Balle", meta = (DisplayName = "Taille de la balle apres le tir", ClampMin = "0.01", UIMin = "0.01", ToolTip = "Echelle visuelle uniforme appliquee a la balle apres un tir. La taille temporaire pendant l'orbite reste a 1."))
	float BallScaleAfterShot = 4.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Balle", meta = (DisplayName = "Delai anti-recapture apres tir", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Duree pendant laquelle une balle qui vient d'etre tiree ne peut pas etre automatiquement recapturee par le meme joueur."))
	float BallRecaptureDelayAfterShot = 0.35f;

	UPROPERTY(Config, EditAnywhere, Category = "Balle", meta = (DisplayName = "Puissance de passe", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Puissance de passe par defaut avant modificateurs."))
	float BasePassPower = 1600.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Balle", meta = (DisplayName = "Aide a la visee", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Aide a la visee appliquee lors des passes et tirs."))
	float AimAssistStrength = 0.35f;

	UPROPERTY(Config, EditAnywhere, Category = "Balle", meta = (DisplayName = "Distance de trace", ClampMin = "100.0", UIMin = "100.0", ToolTip = "Distance maximale de trace pour la visee de balle."))
	float AimTraceDistance = 2500.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Balle|Anti-stagnation", meta = (DisplayName = "Delai avant avertissement", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Temps continu passe dans le meme camp avant d'afficher le compte a rebours."))
	float BallCampGraceSeconds = 5.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Balle|Anti-stagnation", meta = (DisplayName = "Duree du compte a rebours", ClampMin = "0.1", UIMin = "0.1", ToolTip = "Temps laisse pour toucher la balle apres l'apparition de l'avertissement."))
	float BallCampCountdownSeconds = 5.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Balle - Tir enroule", meta = (DisplayName = "Vitesse balle en orbite", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse de rotation de la balle quand elle orbite autour du joueur, en degres par seconde."))
	float CurvedShotOrbitBallSpeed = 480.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Balle - Tir enroule", meta = (DisplayName = "Vitesse de balle sur courbe", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse reelle de la balle quand elle suit la courbe d'enroule. Plus haut = la balle parcourt la courbe plus vite."))
	float CurvedShotSplineSpeed = 5600.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Balle - Tir enroule", meta = (DisplayName = "Vitesse maximale sur courbe", ClampMin = "100.0", UIMin = "100.0", ToolTip = "Plafond absolu de vitesse pendant et apres le tir enroule. Empeche la vitesse capturee d'etre remultipliee sans limite a chaque tir."))
	float CurvedShotMaxSplineSpeed = 20000.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Balle - Tir enroule", meta = (DisplayName = "Multiplicateur vitesse conservee", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Multiplie la vitesse que la balle avait juste avant son arret. 1 conserve exactement sa vitesse sur toute la spline."))
	float CurvedShotCapturedSpeedMultiplier = 2.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Balle - Tir enroule|Sous tir enroule", meta = (DisplayName = "Distance gauche/droite", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Distance maximale d'enroule vers la gauche ou la droite."))
	float CurvedShotHorizontalDistance = 560.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Balle - Tir enroule|Sous tir enroule", meta = (DisplayName = "Distance haut/bas", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Distance maximale d'enroule vers le haut ou le bas."))
	float CurvedShotVerticalDistance = 560.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Balle - Tir enroule|Sous tir enroule", meta = (DisplayName = "Coefficient reduction proche", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Reduction appliquee a l'enroule quand la cible est proche. 1 = aucune reduction, plus bas = enroule plus compact a courte distance."))
	float CurvedShotCloseRangeReduction = 0.45f;

	UPROPERTY(Config)
	float CurvedShotMaxDirection = 560.0f;

	UPROPERTY(Config)
	float CurvedShotScaleMultiplier = 3.0f;

	UPROPERTY(Config)
	float CurvedShotAimTraceDistance = 6500.0f;

	UPROPERTY(Config)
	float CurvedShotMidPointAlpha = 0.5f;

	UPROPERTY(Config)
	float CurvedShotMidLateralMultiplier = 1.75f;

	UPROPERTY(Config)
	float CurvedShotMidHeightMultiplier = 1.75f;

	UPROPERTY(Config)
	float CurvedShotInputChangeSpeed = 3.4f;

	UPROPERTY(Config)
	float CurvedShotResponseExponent = 1.0f;

	UPROPERTY(Config)
	int32 CurvedShotCollisionSampleCount = 16;

	UPROPERTY(Config)
	float CurvedShotPostObstacleOpacityMultiplier = 0.22f;

	UPROPERTY(Config)
	float CurvedShotVisualStartOffset = 70.0f;

	UPROPERTY(Config)
	float CurvedShotInitialStraightDistance = 220.0f;

	UPROPERTY(Config)
	float CurvedShotInitialStraightDistanceRatio = 0.12f;

	UPROPERTY(Config, EditAnywhere, Category = "Combat", meta = (DisplayName = "Puissance de tir", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Puissance de tir de base de la competence principale."))
	float ShootPower = 1800.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Combat", meta = (DisplayName = "Cooldown du tir", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Temps de recharge minimal entre deux tirs."))
	float ShootCooldownSeconds = 0.35f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin", meta = (DisplayName = "Portee du grappin", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Portee maximale du grappin."))
	float GrappleRange = 2200.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin", meta = (DisplayName = "Force de traction", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Force de traction appliquee lorsque le grappin touche une cible valide."))
	float GrapplePullStrength = 2600.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin", meta = (DisplayName = "Duree du grappin", ClampMin = "0.1", UIMin = "0.1", ToolTip = "Duree maximale de vie du grappin avant annulation."))
	float GrappleDurationSeconds = 1.2f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Simple", meta = (DisplayName = "Force horizontale", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Multiplicateur principal de la force vers l'obstacle. Augmente si le grab ne tire pas assez fort vers l'avant ou vers la cible."))
	float GrappleSimpleHorizontalForce = 1.15f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Simple", meta = (DisplayName = "Force hauteur", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Multiplicateur principal de la hauteur du grab. Augmente si le grab ne monte pas assez."))
	float GrappleSimpleHeightForce = 0.85f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Simple", meta = (DisplayName = "Puissance distance", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Controle combien la distance augmente la puissance du grab. Augmente si les obstacles loin doivent tirer plus fort."))
	float GrappleSimpleDistancePower = 1.05f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Simple", meta = (DisplayName = "Puissance difference hauteur", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Controle combien la difference de hauteur avec l'obstacle augmente la puissance verticale. Augmente si les cibles au-dessus doivent faire monter plus fort."))
	float GrappleSimpleHeightDifferencePower = 0.27f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Simple", meta = (DisplayName = "Puissance difference horizontale", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Controle combien la distance horizontale avec l'obstacle augmente la puissance vers l'avant. Augmente si les cibles loin devant doivent tirer plus fort sans forcement ajouter de hauteur."))
	float GrappleSimpleHorizontalDifferencePower = 1.15f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Mouvement", meta = (DisplayName = "Vitesse max de lancement", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse maximale appliquee au joueur quand le grappin le tire vers une cible eloignee."))
	float GrappleVelocityMax = 8500.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Mouvement", meta = (DisplayName = "Vitesse min de lancement", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse minimale garantie au lancement du grappin."))
	float GrappleVelocityMin = 1800.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Depart et puissance", meta = (DisplayName = "Force proche", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse minimale appliquee au depart du grab. Plus haut = les grabs proches tirent plus fort."))
	float GrappleLaunchMinSpeed = 4200.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Depart et puissance", meta = (DisplayName = "Force loin max", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse maximale visee au depart quand l'obstacle est loin. Ne change pas la hauteur, seulement la puissance vers l'obstacle."))
	float GrappleLaunchMaxSpeed = 10500.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Depart et puissance", meta = (DisplayName = "Debut montee puissance", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Distance a partir de laquelle la force commence a monter entre proche et loin. Plus bas = le grab devient puissant plus tot."))
	float GrappleLaunchDistanceStart = 120.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Depart et puissance", meta = (DisplayName = "Transition puissance", ClampMin = "1.0", UIMin = "1.0", ToolTip = "Distance utilisee pour passer progressivement de Force proche a Force loin max. Plus petit = transition plus rapide."))
	float GrappleLaunchDistanceRange = 2400.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Depart et puissance", meta = (DisplayName = "Courbe puissance", ClampMin = "0.1", UIMin = "0.1", ToolTip = "Forme de la montee de puissance selon la distance. 1 = lineaire, plus haut = demarre plus doux puis accelere."))
	float GrappleLaunchDistanceExponent = 0.85f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Depart et puissance", meta = (DisplayName = "Conservation elan", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Part d'elan deja dirigee vers l'obstacle conservee au depart du grab."))
	float GrappleLaunchCarryBoost = 0.55f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Mouvement", meta = (DisplayName = "Acceleration de traction", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Acceleration constante vers le point d'ancrage pendant le grappin."))
	float GrapplePullAcceleration = 62000.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Mouvement", meta = (DisplayName = "Attraction active minimale", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Acceleration minimale appliquee pendant que le grab est actif. Plus haut = le joueur continue d'etre attire plus fort."))
	float GrappleActiveMinPullAcceleration = 72000.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Mouvement", meta = (DisplayName = "Debut bonus traction loin", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Distance a partir de laquelle la traction active recoit un bonus lie a l'eloignement."))
	float GrappleActiveDistanceBoostStart = 400.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Mouvement", meta = (DisplayName = "Bonus traction loin", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Force ajoutee a la traction active quand l'obstacle est loin."))
	float GrappleActiveDistanceBoostScale = 7.5f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Mouvement", meta = (DisplayName = "Vitesse active minimale", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse minimale garantie vers l'obstacle pendant le grab actif."))
	float GrappleActiveMinTowardSpeed = 3400.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Mouvement", meta = (DisplayName = "Vitesse active maximale", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse maximale autorisee pendant le grab actif."))
	float GrappleActiveMaxSpeed = 11500.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Mouvement", meta = (DisplayName = "Controle de trajectoire", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Force tangentielle ajoutee par l'input joueur pour courber la trajectoire en l'air."))
	float GrappleSwingAcceleration = 10500.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Mouvement", meta = (DisplayName = "Vitesse max pendant grappin", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse maximale autorisee tant que le grappin est actif."))
	float GrappleSwingMaxSpeed = 11500.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Mouvement", meta = (DisplayName = "Vitesse de raccourcissement", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse a laquelle la corde se raccourcit pendant la traction."))
	float GrappleRopeShortenSpeed = 2800.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Mouvement", meta = (DisplayName = "Boost a la liberation", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Multiplicateur applique a la vitesse du joueur quand le grappin se coupe."))
	float GrappleReleaseVelocityBoost = 1.08f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Delais", meta = (DisplayName = "Delai de relance", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Temps a attendre apres la fin d'un grappin avant de pouvoir en relancer un."))
	float GrappleRestartDelay = 1.2f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Delais", meta = (DisplayName = "Delai restauration rotation", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Delai avant de restaurer les flags de rotation du personnage apres la fin du grappin."))
	float GrappleReleaseDelay = 0.2f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Detection", meta = (DisplayName = "Distance min cible", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Distance minimale avant qu'un obstacle puisse etre cible par le grappin."))
	float GrappleMinTargetDistance = 425.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Detection", meta = (DisplayName = "Taille demi-box trace", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Demi-taille de la box utilisee par l'assistance de detection du grappin."))
	FVector GrappleTraceHalfSize = FVector(250.0f);

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Detection", meta = (DisplayName = "Offset debut trace", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Distance devant la camera a laquelle commence la box trace assistee."))
	float GrappleTraceStartOffset = 1000.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Detection", meta = (DisplayName = "Distance fin trace", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Distance maximale de detection du grappin depuis la camera."))
	float GrappleTraceEndDistance = 10000.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Detection", meta = (DisplayName = "Rayon assist min ecran", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Rayon minimum en pixels autour du viseur pour l'assistance de selection du grappin."))
	float GrappleAimAssistScreenRadiusMin = 72.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Detection", meta = (DisplayName = "Rayon assist ratio ecran", ClampMin = "0.0", ClampMax = "0.5", UIMin = "0.0", UIMax = "0.5", ToolTip = "Part de la plus petite dimension d'ecran utilisee pour l'assistance. Plus bas = selection plus precise et plus competitive."))
	float GrappleAimAssistScreenRadiusRatio = 0.10f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Detection", meta = (DisplayName = "Angle max assist", ClampMin = "0.0", ClampMax = "45.0", UIMin = "0.0", UIMax = "45.0", ToolTip = "Angle maximal depuis la camera pour qu'une cible puisse etre selectionnee par assistance."))
	float GrappleAimAssistMaxAngleDegrees = 6.5f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Trajectoire", meta = (DisplayName = "Pitch min arc", ClampMin = "-89.0", ClampMax = "89.0", UIMin = "-89.0", UIMax = "89.0", ToolTip = "Angle vertical minimum applique au debut de la trajectoire du grappin."))
	float GrappleArcMinPitchDegrees = 18.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Trajectoire", meta = (DisplayName = "Pitch max arc", ClampMin = "-89.0", ClampMax = "89.0", UIMin = "-89.0", UIMax = "89.0", ToolTip = "Angle vertical maximum applique au debut de la trajectoire du grappin."))
	float GrappleArcMaxPitchDegrees = 28.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Trajectoire", meta = (DisplayName = "Duree blend arc", ClampMin = "0.01", UIMin = "0.01", ToolTip = "Temps avant que l'arc initial devienne une traction directe vers l'ancre. Plus bas = trajectoire plus lisible."))
	float GrappleArcBlendOutTime = 0.42f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Trajectoire - hauteur", meta = (DisplayName = "Angle grab plat", ClampMin = "-89.0", ClampMax = "89.0", UIMin = "-89.0", UIMax = "89.0", ToolTip = "Si l'angle vertical vers l'obstacle est sous cette valeur, le grab part presque tout droit vers l'avant avec peu de hauteur."))
	float GrappleFlatForwardPitchThreshold = 12.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Trajectoire - hauteur", meta = (DisplayName = "Angle hauteur max", ClampMin = "-89.0", ClampMax = "89.0", UIMin = "-89.0", UIMax = "89.0", ToolTip = "Angle a partir duquel le grab utilise toute sa hauteur. Entre Angle grab plat et cette valeur, la hauteur monte progressivement."))
	float GrappleFullVerticalPitchThreshold = 42.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Trajectoire - hauteur", meta = (DisplayName = "Angle activation sous obstacle", ClampMin = "-89.0", ClampMax = "89.0", UIMin = "-89.0", UIMax = "89.0", ToolTip = "Angle vertical minimum pour activer le mode special quand le joueur est sous l'obstacle."))
	float GrappleUnderObstaclePitchThreshold = 35.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Trajectoire - hauteur", meta = (DisplayName = "Angle lancement sous obstacle", ClampMin = "0.0", ClampMax = "89.0", UIMin = "0.0", UIMax = "89.0", ToolTip = "Angle vise pour le lancement quand le joueur est sous l'obstacle. Plus haut = trajectoire plus verticale."))
	float GrappleUnderObstacleLaunchPitch = 68.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Trajectoire - hauteur", meta = (DisplayName = "Melange sous obstacle", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Melange entre trajectoire normale et trajectoire verticale sous obstacle."))
	float GrappleUnderObstacleVerticalBlend = 0.85f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Trajectoire - hauteur", meta = (DisplayName = "Bonus vertical sous obstacle", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse verticale ajoutee quand le joueur est sous l'obstacle."))
	float GrappleUnderObstacleExtraUpSpeed = 1100.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Trajectoire - hauteur", meta = (DisplayName = "Debut boost vertical loin", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Distance a partir de laquelle un grab lointain peut recevoir un bonus vertical. Ce bonus est attenue si l'obstacle est surtout devant."))
	float GrappleFarUpBoostStart = 1100.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Trajectoire - hauteur", meta = (DisplayName = "Transition boost vertical", ClampMin = "1.0", UIMin = "1.0", ToolTip = "Distance de transition du bonus vertical longue distance."))
	float GrappleFarUpBoostRange = 5000.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Trajectoire - hauteur", meta = (DisplayName = "Vitesse verticale depart loin", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse verticale ajoutee au depart quand l'obstacle est loin et suffisamment au-dessus."))
	float GrappleFarLaunchExtraUpSpeed = 1100.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Trajectoire - hauteur", meta = (DisplayName = "Acceleration verticale active loin", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Acceleration verticale ajoutee pendant le grab actif quand l'obstacle est loin et suffisamment au-dessus."))
	float GrappleFarActiveUpAcceleration = 4500.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Trajectoire - hauteur", meta = (DisplayName = "Vitesse verticale active loin", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse verticale minimale visee pendant un grab lointain quand l'obstacle est suffisamment au-dessus."))
	float GrappleFarActiveMinUpSpeed = 900.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Visee surface", meta = (DisplayName = "Zone morte centre", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Zone au centre de la surface ou le grappin reste une traction directe."))
	float GrappleSurfaceAimDeadZone = 0.12f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Visee surface", meta = (DisplayName = "Vitesse surface depart", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse laterale ajoutee au depart selon le point vise sur le cube."))
	float GrappleSurfaceAimLaunchSpeed = 1700.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Visee surface", meta = (DisplayName = "Acceleration surface", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Acceleration laterale maintenue selon le point vise sur le cube pendant le grappin."))
	float GrappleSurfaceAimAcceleration = 9500.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Coupure", meta = (DisplayName = "Buffer auto-detach", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Marge de securite autour du joueur et de l'obstacle pour couper le grappin."))
	float GrappleAutoDetachBuffer = 120.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Coupure", meta = (DisplayName = "Duree min avant coupure", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Temps minimum pendant lequel le grappin reste actif avant une coupure automatique de proximite."))
	float GrappleMinActiveDuration = 0.45f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Coupure", meta = (DisplayName = "Distance coupure auto", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Distance stable a partir de laquelle le grappin se coupe pres de l'obstacle ou de l'ancre."))
	float GrappleAutoReleaseDistance = 155.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Coupure", meta = (DisplayName = "Zone coupure orbite", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Zone supplementaire autour de l'ancre ou le grappin se coupe si le joueur commence a tourner autour."))
	float GrappleOrbitReleaseBuffer = 620.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Coupure", meta = (DisplayName = "Vitesse laterale orbite", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse laterale minimale pour considerer que le joueur orbite autour de l'ancre."))
	float GrappleOrbitReleaseMinLateralSpeed = 350.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Coupure", meta = (DisplayName = "Buffer coupure anticipee", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Marge supplementaire utilisee pour eviter que le cable ramene le joueur en arriere pres de l'obstacle."))
	float GrappleEarlyDetachBuffer = 70.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Coupure", meta = (DisplayName = "Prediction coupure", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Temps d'anticipation utilise pour couper les grappins rapides avant une zone dangereuse."))
	float GrappleEarlyDetachLeadTime = 0.045f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Grappin|Depart et puissance", meta = (DisplayName = "Duree de montee de la traction", ClampMin = "0.01", UIMin = "0.01", Units = "s", ToolTip = "Temps pour passer de la vitesse actuelle du joueur a la pleine vitesse du grappin. Plus court = plus sec, plus long = plus doux."))
	float GrapplePullBlendTime = 0.12f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Grappin|Coupure", meta = (DisplayName = "Vitesse de relance", ClampMin = "0.0", ClampMax = "1.5", UIMin = "0.0", UIMax = "1.5", ToolTip = "Juste avant l'impact, le joueur est relance dans la direction ou il regarde avec cette part de la vitesse du grappin. S'il regarde l'obstacle, il longe sa surface."))
	float GrappleArrivalSpeedKeep = 0.8f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Grappin|Coupure", meta = (DisplayName = "Impulsion vers le haut a la relance", ClampMin = "0.0", UIMin = "0.0", Units = "cm/s", ToolTip = "Petite vitesse verticale ajoutee a la relance pour faciliter l'enchainement."))
	float GrappleArrivalUpBoost = 300.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Grappin|Detection", meta = (DisplayName = "Agrandissement de la zone de visee", ClampMin = "0.0", UIMin = "0.0", Units = "cm", ToolTip = "Marge ajoutee autour des obstacles uniquement pour le rayon de visee du grappin (les collisions ne changent pas). Plus grand = plus facile de viser vite."))
	float GrappleAimHitboxExpansion = 250.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Point de visee", meta = (DisplayName = "Afficher le point de visee", ToolTip = "Petit point au centre de l'ecran. Il grossit et devient cyan quand un obstacle grappinable est vise."))
	bool bShowAimDot = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Point de visee", meta = (DisplayName = "Taille du point de visee", ClampMin = "1.0", ClampMax = "30.0", UIMin = "1.0", UIMax = "30.0", ToolTip = "Taille du point en pixels pour un ecran de 1080 lignes (adaptee automatiquement a la resolution)."))
	float AimDotSize = 5.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Corde", meta = (DisplayName = "Longueur min corde", ClampMin = "1.0", UIMin = "1.0", ToolTip = "Longueur minimale gardee pour eviter que le grappin s'effondre directement sur l'ancre."))
	float GrappleMinCableLength = 150.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Corde", meta = (DisplayName = "Duree affichage corde", ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0", Units = "s", ToolTip = "Temps pendant lequel la corde reste visible apres l'unique propulsion du grab. N'ajoute aucune attraction."))
	float GrappleRopeDisplayDuration = 0.25f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Corde", meta = (DisplayName = "Mou corde depart", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Longueur supplementaire ajoutee au debut du grappin pour eviter un snap trop sec."))
	float GrappleCableSlack = 36.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Obstacle", meta = (DisplayName = "Duree disparition obstacle", ClampMin = "0.01", UIMin = "0.01", ToolTip = "Duree du fade de consommation de l'obstacle apres utilisation du grappin."))
	float GrappleConsumedFadeDuration = 2.5f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Obstacle", meta = (DisplayName = "Delai respawn obstacle", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Temps pendant lequel l'obstacle reste cache apres sa disparition avant de redevenir utilisable."))
	float GrappleObstacleRespawnDelay = 0.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Camera", meta = (DisplayName = "FOV du joueur", ClampMin = "60.0", ClampMax = "150.0", UIMin = "60.0", UIMax = "150.0", ToolTip = "Champ de vision normal du joueur, hors sprint."))
	float RunFOV = 130.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Camera", meta = (DisplayName = "FOV en sprint", ClampMin = "60.0", ClampMax = "150.0", UIMin = "60.0", UIMax = "150.0", ToolTip = "Champ de vision cible lorsque le joueur sprinte."))
	float SprintFOV = 140.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Camera", meta = (DisplayName = "Vitesse d'interpolation FOV", ClampMin = "0.1", UIMin = "0.1", ToolTip = "Vitesse d'interpolation du FOV quand la camera change d'etat."))
	float FOVInterpSpeed = 8.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Camera", meta = (DisplayName = "Amplitude du bob", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Amplitude du head bob de course."))
	float CameraBobAmplitude = 1.5f;

	UPROPERTY(Config, EditAnywhere, Category = "Camera", meta = (DisplayName = "Roulis max", ClampMin = "0.0", ClampMax = "15.0", UIMin = "0.0", UIMax = "15.0", ToolTip = "Inclinaison laterale maximale de la camera pendant la course."))
	float CameraRollDegrees = 4.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Effets de vitesse", meta = (DisplayName = "Vitesse de debut des effets", ClampMin = "0.0", UIMin = "0.0", Units = "cm/s", ToolTip = "Vitesse reelle (dash, wall run, chute, grappin, sprint...) a partir de laquelle FOV, vignettage et aberration chromatique commencent a s'intensifier."))
	float SpeedEffectsStartSpeed = 800.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Effets de vitesse", meta = (DisplayName = "Vitesse des effets au maximum", ClampMin = "1.0", UIMin = "1.0", Units = "cm/s", ToolTip = "Vitesse reelle a laquelle les effets atteignent leur maximum (FOV de sprint)."))
	float SpeedEffectsFullSpeed = 3600.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Effets de vitesse", meta = (DisplayName = "Reactivite des effets", ClampMin = "0.1", UIMin = "0.1", ToolTip = "Vitesse a laquelle les effets suivent la vitesse du joueur. Plus haut = plus nerveux."))
	float SpeedEffectsInterpSpeed = 6.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Effets de vitesse", meta = (DisplayName = "Bonus FOV a tres grande vitesse", ClampMin = "0.0", ClampMax = "40.0", UIMin = "0.0", UIMax = "40.0", Units = "deg", ToolTip = "FOV ajoute au-dela du FOV de sprint quand la vitesse depasse 'Vitesse des effets au maximum' (dash, grappin, grandes chutes). Le FOV total reste plafonne a 150."))
	float SpeedFOVOverSpeedBoost = 15.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Effets de vitesse", meta = (DisplayName = "Vitesse du bonus FOV maximum", ClampMin = "1.0", UIMin = "1.0", Units = "cm/s", ToolTip = "Vitesse a laquelle le bonus FOV a tres grande vitesse est complet."))
	float SpeedFOVOverSpeedMaxSpeed = 7000.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Effets de vitesse", meta = (DisplayName = "Tassement max a l'atterrissage", ClampMin = "0.0", UIMin = "0.0", Units = "cm", ToolTip = "Descente maximale de la camera a l'atterrissage d'une grosse chute. 0 desactive l'effet."))
	float LandingDipMaxDistance = 12.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Effets de vitesse", meta = (DisplayName = "Chute pour un tassement complet", ClampMin = "1.0", UIMin = "1.0", Units = "cm/s", ToolTip = "Vitesse de chute a l'impact qui donne le tassement maximal. Les petits sauts (sous 30 % de cette valeur) ne tassent pas la camera."))
	float LandingDipFullFallSpeed = 2000.0f;
};

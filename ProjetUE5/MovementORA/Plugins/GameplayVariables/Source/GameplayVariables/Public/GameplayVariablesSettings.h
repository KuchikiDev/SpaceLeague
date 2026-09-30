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

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Match|Prison", meta = (DisplayName = "Duree de prison", ClampMin = "1", ClampMax = "60", UIMin = "1", UIMax = "30", Units = "s", ToolTip = "Secondes passees en prison apres avoir ete touche par la balle, avant le retour sur le terrain."))
	int32 PrisonDurationSeconds = 8;

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
	float BaseWalkSpeed = 2800.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Deplacement", meta = (DisplayName = "Vitesse de sprint", ClampMin = "0.0", UIMin = "0.0", Units = "cm/s", ToolTip = "Vitesse de deplacement quand le joueur sprinte."))
	float SprintWalkSpeed = 4000.0f;

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

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Deplacement|Elan", meta = (DisplayName = "Virage en l'air", ClampMin = "0.0", ClampMax = "3600.0", UIMin = "0.0", UIMax = "1800.0", Units = "deg", ToolTip = "Degres par seconde dont la vitesse tourne vers la direction voulue en l'air, sans freiner. 720 = demi-tour en 0,25 s. 0 = ancien comportement (freinage puis reacceleration)."))
	float AirTurnRate = 720.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Deplacement|Elan", meta = (DisplayName = "Vitesse gardee au demi-tour en l'air", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Part de la vitesse horizontale conservee apres un demi-tour complet en l'air (moins de perte pour un virage plus petit). 1 = aucune perte."))
	float AirTurnSpeedKeep = 0.9f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mur", meta = (DisplayName = "Vitesse du wall run", ClampMin = "0.0", UIMin = "0.0", Units = "cm/s", ToolTip = "Vitesse de course le long d'un mur, stick a fond."))
	float WallRunSpeed = 5500.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mur", meta = (DisplayName = "Vitesse gardee en arrivant sur le mur", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Part de la vitesse horizontale conservee le long du mur quand on arrive en biais (hors dash). 0 = seule la partie deja le long du mur est gardee."))
	float WallRunEntrySpeedKeep = 0.9f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mur", meta = (DisplayName = "Perte d'elan au mur", ClampMin = "0.0", UIMin = "0.0", Units = "cm/s", ToolTip = "Vitesse perdue par seconde quand on court sur le mur plus vite que la vitesse du wall run (arrivee en sprint, apres un saut ou un grappin)."))
	float WallRunMomentumDecay = 1500.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mur", meta = (DisplayName = "Vitesse de recadrage de la camera au mur", ClampMin = "0.0", UIMin = "0.0", Units = "deg", ToolTip = "Degres par seconde dont la camera est ramenee quand on regarde trop vers le mur."))
	float WallLookPushBackSpeed = 120.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mur", meta = (DisplayName = "Angle de vue max vers le mur", ClampMin = "0.0", ClampMax = "179.0", UIMin = "90.0", UIMax = "179.0", Units = "deg", ToolTip = "Accroche au mur sans courir : angle maximum entre le regard et la direction qui s'eloigne du mur. Au-dela, la camera est ramenee (vitesse de recadrage). 179 = presque libre, 90 = on ne peut pas regarder vers le mur."))
	float WallSlideLookAngleLimit = 140.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mur", meta = (DisplayName = "Angle pour se decrocher en dash", ClampMin = "0.0", ClampMax = "89.0", UIMin = "0.0", UIMax = "89.0", Units = "deg", ToolTip = "Dash sur un mur : si le regard s'ecarte du mur de plus de cet angle (vers l'exterieur), le joueur se decroche et dash la ou il regarde sans perdre de vitesse. Sinon (regard vers le mur ou le long du mur), il dash le long du mur."))
	float WallDashDetachLookAngle = 20.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mur", meta = (DisplayName = "Vitesse min du dash sur le mur", ClampMin = "0.0", UIMin = "0.0", Units = "cm/s", ToolTip = "Dash depuis un mur (le long du mur ou en se decrochant) : vitesse minimum donnee par le dash."))
	float WallDashMinSpeed = 10000.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mur", meta = (DisplayName = "Boost du dash sur le mur", ClampMin = "0.0", UIMin = "0.0", Units = "cm/s", ToolTip = "Dash depuis un mur : vitesse ajoutee a la vitesse actuelle (on garde au moins la vitesse min)."))
	float WallDashSpeedBoost = 4000.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mur", meta = (DisplayName = "Temps de decollage du mur", ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "1.0", Units = "s", ToolTip = "Apres un saut ou un dash qui quitte le mur : pendant ce temps le joueur ne peut ni etre ramene vers ce mur ni s'y raccrocher, et le virage en l'air est suspendu. Evite l'effet de collage."))
	float WallLeaveGraceSeconds = 0.35f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mur", meta = (DisplayName = "Delai avant de se raccrocher au meme mur", ClampMin = "0.0", ClampMax = "3.0", UIMin = "0.0", UIMax = "2.0", Units = "s", ToolTip = "Apres un saut ou un dash qui quitte le mur, on ne peut pas se raccrocher a ce meme mur pendant ce temps. Un autre mur reste possible."))
	float WallReattachSameWallSeconds = 0.8f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mur", meta = (DisplayName = "Rotation de la camera vers la course", ClampMin = "0.0", ClampMax = "3600.0", UIMin = "0.0", UIMax = "1800.0", Units = "deg", ToolTip = "Vitesse max (degres par seconde) de la rotation de la camera vers le sens de la course, quand on arrive sur un mur en le regardant ou qu'on change de sens. Elle ralentit en fin de mouvement. 0 = desactive."))
	float WallRunCameraTurnSpeed = 300.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mur", meta = (DisplayName = "Force de la rotation de la camera", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Part de l'angle vers le sens de la course que la camera parcourt. 1 = s'aligne completement, 0.5 = fait la moitie du chemin, 0 = ne tourne pas."))
	float WallRunCameraTurnStrength = 0.5f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mur", meta = (DisplayName = "Duree max sur un mur", ClampMin = "0.0", ClampMax = "30.0", UIMin = "0.0", UIMax = "15.0", Units = "s", ToolTip = "Temps maximum accroche a un meme mur. Le compteur repart de zero a chaque changement de mur (autre mur ou autre face). Le mur tenu jusqu'a la limite reste interdit jusqu'au sol. 0 = sans limite."))
	float WallSlideMaxDuration = 5.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Dash", meta = (DisplayName = "Impulsion du dash", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Force principale appliquee lors d'un dash."))
	float DashImpulse = 8000.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Dash", meta = (DisplayName = "Duree du dash", ClampMin = "0.01", UIMin = "0.01", ToolTip = "Duree pendant laquelle le dash reste actif."))
	float DashDurationSeconds = 0.2f;

	UPROPERTY(Config, EditAnywhere, Category = "Dash", meta = (DisplayName = "Cooldown du dash", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Temps necessaire avant de relancer un dash."))
	float DashCooldownSeconds = 0.2f;

	UPROPERTY(Config, EditAnywhere, Category = "Saut", meta = (DisplayName = "Nombre max de sauts", ClampMin = "1", UIMin = "1", ToolTip = "Nombre maximum de sauts consecutifs autorises."))
	int32 MaxJumpCount = 2;

	UPROPERTY(Config, EditAnywhere, Category = "Saut", meta = (DisplayName = "Vitesse de saut", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse verticale donnee au joueur au moment du saut."))
	float JumpVelocity = 4800.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Saut", meta = (DisplayName = "Gravite a la montee du saut", ClampMin = "0.1", ClampMax = "40.0", UIMin = "1.0", UIMax = "20.0", ToolTip = "Echelle de gravite pendant la montee d'un saut. Plus haut = montee plus rapide (a monter avec la vitesse de saut pour garder la hauteur). La gravite de base du personnage est 8."))
	float JumpRiseGravityScale = 12.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Saut", meta = (DisplayName = "Gravite a la descente", ClampMin = "0.1", ClampMax = "40.0", UIMin = "1.0", UIMax = "20.0", ToolTip = "Echelle de gravite quand le joueur retombe (hors grappin, mur et dash). La gravite de base du personnage est 8."))
	float FallGravityScale = 9.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Saut", meta = (DisplayName = "Coyote time", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.3", Units = "s", ToolTip = "Juste apres avoir quitte le sol sans sauter (bord de plateforme), le saut compte encore comme un saut depuis le sol : le double saut reste disponible. 0 = desactive."))
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
	float CurvedShotScaleMultiplier = 3.0f;

	UPROPERTY(Config)
	float CurvedShotAimTraceDistance = 6500.0f;

	UPROPERTY(Config)
	float CurvedShotInputChangeSpeed = 3.4f;

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

	UPROPERTY(Config, EditAnywhere, Category = "Grappin", meta = (DisplayName = "Portee du grappin", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Portee maximale du grappin."))
	float GrappleRange = 2200.0f;

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

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Mouvement", meta = (DisplayName = "Vitesse max pendant grappin", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse maximale autorisee tant que le grappin est actif."))
	float GrappleSwingMaxSpeed = 11500.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Mouvement", meta = (DisplayName = "Boost a la liberation", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Multiplicateur applique a la vitesse du joueur quand le grappin se coupe."))
	float GrappleReleaseVelocityBoost = 1.08f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Delais", meta = (DisplayName = "Delai de relance", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Temps a attendre apres la fin d'un grappin avant de pouvoir en relancer un."))
	float GrappleRestartDelay = 1.2f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Delais", meta = (DisplayName = "Delai restauration rotation", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Delai avant de restaurer les flags de rotation du personnage apres la fin du grappin."))
	float GrappleReleaseDelay = 0.2f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Detection", meta = (DisplayName = "Distance min cible", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Distance minimale avant qu'un obstacle puisse etre cible par le grappin."))
	float GrappleMinTargetDistance = 425.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Detection", meta = (DisplayName = "Offset debut trace", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Distance devant la camera a laquelle commence la box trace assistee."))
	float GrappleTraceStartOffset = 1000.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Trajectoire - hauteur", meta = (DisplayName = "Debut boost vertical loin", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Distance a partir de laquelle un grab lointain peut recevoir un bonus vertical. Ce bonus est attenue si l'obstacle est surtout devant."))
	float GrappleFarUpBoostStart = 1100.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Trajectoire - hauteur", meta = (DisplayName = "Transition boost vertical", ClampMin = "1.0", UIMin = "1.0", ToolTip = "Distance de transition du bonus vertical longue distance."))
	float GrappleFarUpBoostRange = 5000.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Trajectoire - hauteur", meta = (DisplayName = "Vitesse verticale depart loin", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse verticale ajoutee au depart quand l'obstacle est loin et suffisamment au-dessus."))
	float GrappleFarLaunchExtraUpSpeed = 1100.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Visee surface", meta = (DisplayName = "Zone morte centre", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Zone au centre de la surface ou le grappin reste une traction directe."))
	float GrappleSurfaceAimDeadZone = 0.12f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Visee surface", meta = (DisplayName = "Vitesse surface depart", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Vitesse laterale ajoutee au depart selon le point vise sur le cube."))
	float GrappleSurfaceAimLaunchSpeed = 1700.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Coupure", meta = (DisplayName = "Buffer auto-detach", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Marge de securite autour du joueur et de l'obstacle pour couper le grappin."))
	float GrappleAutoDetachBuffer = 120.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Grappin|Coupure", meta = (DisplayName = "Distance coupure auto", ClampMin = "0.0", UIMin = "0.0", ToolTip = "Distance stable a partir de laquelle le grappin se coupe pres de l'obstacle ou de l'ancre."))
	float GrappleAutoReleaseDistance = 155.0f;

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

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Grappin|Balancier", meta = (DisplayName = "Profondeur du balancier", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Creux de la trajectoire en U, en part de la distance au point vise. 0 = ligne droite."))
	float GrappleSwingSagRatio = 0.3f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Grappin|Balancier", meta = (DisplayName = "Creux maximum", ClampMin = "0.0", UIMin = "0.0", Units = "cm", ToolTip = "Profondeur maximale du creux. Le creux ne descend jamais dans le sol."))
	float GrappleSwingMaxSag = 800.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Grappin|Balancier", meta = (DisplayName = "Acceleration au creux", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Vitesse en plus au point le plus bas du balancier (0,25 = +25 %), comme un pendule."))
	float GrappleSwingSpeedBoost = 0.25f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Grappin|Balancier", meta = (DisplayName = "Hauteur max sous la cible", ClampMin = "0.0", UIMin = "0.0", Units = "cm", ToolTip = "Si le joueur est plus bas que la cible de plus que cette hauteur, le grappin l'emmene tout droit, sans balancier."))
	float GrappleSwingMaxHeightBelowTarget = 300.0f;

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

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (DisplayName = "Lissage de la camera sur les murs", ClampMin = "1.0", ClampMax = "40.0", UIMin = "1.0", UIMax = "40.0", ToolTip = "Vitesse a laquelle la camera suit l'orientation du mur en wall run. Plus bas = plus doux (moins de tremblement sur les murs courbes ou a facettes), plus haut = plus reactif."))
	float WallCameraNormalSmoothing = 12.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (DisplayName = "Retard max de la camera", ClampMin = "0.0", ClampMax = "2000.0", UIMin = "0.0", UIMax = "600.0", Units = "cm", ToolTip = "Distance maximale dont la camera peut trainer derriere le joueur (effet de retard qui adoucit les mouvements). Sans limite, elle trainait de plusieurs metres a grande vitesse et un dash semblait partir en retard. 0 = pas de limite."))
	float CameraLagMaxDistance = 150.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Effets de vitesse", meta = (DisplayName = "Vitesse de debut des effets", ClampMin = "0.0", UIMin = "0.0", Units = "cm/s", ToolTip = "Vitesse reelle (dash, wall run, chute, grappin, sprint...) a partir de laquelle FOV, vignettage et aberration chromatique commencent a s'intensifier."))
	float SpeedEffectsStartSpeed = 2900.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Effets de vitesse", meta = (DisplayName = "Vitesse des effets au maximum", ClampMin = "1.0", UIMin = "1.0", Units = "cm/s", ToolTip = "Vitesse reelle a laquelle les effets atteignent leur maximum (FOV de sprint)."))
	float SpeedEffectsFullSpeed = 5400.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Effets de vitesse", meta = (DisplayName = "Reactivite des effets", ClampMin = "0.1", UIMin = "0.1", ToolTip = "Vitesse a laquelle les effets suivent la vitesse du joueur. Plus haut = plus nerveux."))
	float SpeedEffectsInterpSpeed = 6.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Effets de vitesse", meta = (DisplayName = "Bonus FOV a tres grande vitesse", ClampMin = "0.0", ClampMax = "40.0", UIMin = "0.0", UIMax = "40.0", Units = "deg", ToolTip = "FOV ajoute au-dela du FOV de sprint quand la vitesse depasse 'Vitesse des effets au maximum' (dash, grappin, grandes chutes). Le FOV total reste plafonne a 150."))
	float SpeedFOVOverSpeedBoost = 5.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Effets de vitesse", meta = (DisplayName = "Vitesse du bonus FOV maximum", ClampMin = "1.0", UIMin = "1.0", Units = "cm/s", ToolTip = "Vitesse a laquelle le bonus FOV a tres grande vitesse est complet."))
	float SpeedFOVOverSpeedMaxSpeed = 8000.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Effets de vitesse", meta = (DisplayName = "Trainees de vent", ToolTip = "Fines trainees posees dans le monde autour du joueur, qui defilent a grande vitesse (dash, grappin). Test : commande console ora.SpeedLines.Force 1 (-1 pour revenir au normal)."))
	bool bShowSpeedLines = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Effets de vitesse", meta = (DisplayName = "Intensite des trainees de vent", ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "2.0", ToolTip = "Luminosite maximale des trainees de vent (atteinte a tres grande vitesse)."))
	float SpeedLinesIntensity = 1.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Effets de vitesse", meta = (DisplayName = "Tassement max a l'atterrissage", ClampMin = "0.0", UIMin = "0.0", Units = "cm", ToolTip = "Descente maximale de la camera a l'atterrissage d'une grosse chute. 0 desactive l'effet."))
	float LandingDipMaxDistance = 12.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera|Effets de vitesse", meta = (DisplayName = "Chute pour un tassement complet", ClampMin = "1.0", UIMin = "1.0", Units = "cm/s", ToolTip = "Vitesse de chute a l'impact qui donne le tassement maximal. Les petits sauts (sous 30 % de cette valeur) ne tassent pas la camera."))
	float LandingDipFullFallSpeed = 2000.0f;
};

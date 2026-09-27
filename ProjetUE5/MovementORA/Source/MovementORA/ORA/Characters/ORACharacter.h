#pragma once

#include "CoreMinimal.h"
#include "ORA/Characters/ORACharacterBase.h"
#include "ORA/Core/ORAPlayerState.h"
#include "ORACharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USplineComponent;
class UCableComponent;
class USphereComponent;
class UCapsuleComponent;
class UStaticMeshComponent;
class UInputAction;

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EORAGait : uint8
{
	Walk UMETA(DisplayName = "Walk"),
	Run  UMETA(DisplayName = "Run")
};

UENUM(BlueprintType)
enum class EMovementStickMode : uint8
{
	FixedSpeedSingleGait    UMETA(DisplayName = "Fixed Speed - Single Gait"),
	FixedSpeedWalkRun       UMETA(DisplayName = "Fixed Speed - Walk / Run"),
	VariableSpeedSingleGait UMETA(DisplayName = "Variable Speed - Single Gait"),
	VariableSpeedWalkRun    UMETA(DisplayName = "Variable Speed - Walk / Run")
};

UCLASS(BlueprintType, Blueprintable)
class MOVEMENTORA_API AORACharacter : public AORACharacterBase
{
	GENERATED_BODY()

public:
	AORACharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;

	// -----------------------------------------------------------------------
	// Components
	// -----------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Distance de caméra rapprochée pour garder le personnage lisible à l'écran. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Framing", meta = (ClampMin = "100.0"))
	float DefaultCameraArmLength = 300.0f;

	/** Hauteur du cadrage, centrée sur le haut du torse. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Framing")
	float DefaultCameraSocketHeight = 70.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Grapple")
	TObjectPtr<USplineComponent> EnroulerDebug;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Grapple")
	TObjectPtr<UCableComponent> GrappleCable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Grapple")
	TObjectPtr<USphereComponent> NoGrappleZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Obstacle")
	TObjectPtr<UCapsuleComponent> TurnAroundCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Visual")
	TObjectPtr<UStaticMeshComponent> MeshForExemple;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Grapple")
	TObjectPtr<UStaticMeshComponent> GrappleStart;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Grapple")
	TObjectPtr<UStaticMeshComponent> GrappleEnd;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Grapple|VFX")
	TObjectPtr<UStaticMeshComponent> GrappleLineMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Grapple|VFX")
	TArray<TObjectPtr<UStaticMeshComponent>> GrappleRopeSegments;

	/** Rayon câble affiché entre le côté droit de la caméra et l'obstacle focalisé. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Grapple|VFX")
	TObjectPtr<UCableComponent> GrabFocusCable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Grapple|VFX")
	TObjectPtr<UStaticMeshComponent> GrabFocusLineMesh;

	// -----------------------------------------------------------------------
	// Input
	// -----------------------------------------------------------------------

	// -----------------------------------------------------------------------
	// Input
	// -----------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Shoot")
	TObjectPtr<UInputAction> ShootInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Grapple")
	TObjectPtr<UInputAction> GrappleInputAction;

	// -----------------------------------------------------------------------
	// Shoot config / state
	// -----------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shoot|Config", meta = (ClampMin = "0.0"))
	float AntiSpamDelay = 0.3f;

	/** Vitesse initiale appliquée à la balle au moment du tir (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shoot|Config", meta = (ClampMin = "0.0"))
	float ShootBallSpeed = 5600.0f;

	/** Multiplicateur appliqué une seule fois à la vitesse mesurée avant l'arrêt de la balle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shoot|Config", meta = (ClampMin = "0.0"))
	float ShootCapturedSpeedMultiplier = 2.0f;

	/** Plafond absolu de vitesse du tir pour eviter une multiplication recursive entre tirs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shoot|Config", meta = (ClampMin = "100.0"))
	float ShootBallMaxSplineSpeed = 20000.0f;

	/** Multiplicateur de scale appliqué à la balle au moment du tir (1.0 = taille originale). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shoot|Config", meta = (ClampMin = "0.1"))
	float ShootBallScaleMultiplier = 3.0f;

	/** Accélération appliquée à la balle pendant le suivi de spline (cm/s²). 0 = vitesse constante. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shoot|Config", meta = (ClampMin = "0.0"))
	float ShootBallAcceleration = 11200.0f;

	/** Coefficient pour la vitesse d'orbite basée sur la vitesse de vol de la balle.
	 *  orbit_deg/s = RadToDeg(BallSpeed x Coefficient / OrbitRadius).
	 *  0 = utilise StopBallOrbitSpeedDegrees fixe. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball|Orbit", meta = (ClampMin = "0.0"))
	float OrbitSpeedBallCoefficient = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shoot|State")
	bool bIsShootingBall = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shoot|State")
	bool bIsPassing = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shoot|State")
	bool bAntiSpamIsActif = false;

	/** Progression du verrou anti-spam : 0 au clic, 1 quand un nouveau clic est disponible. */
	UFUNCTION(BlueprintPure, Category = "Shoot|State")
	float GetAntiSpamCooldownProgress() const;

	/** Temps restant avant la fin du verrou anti-spam, en secondes. */
	UFUNCTION(BlueprintPure, Category = "Shoot|State")
	float GetAntiSpamCooldownRemaining() const;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shoot|State")
	TObjectPtr<AActor> CachedBallActor;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shoot|State")
	FVector SavedMovementVector = FVector::ZeroVector;

	// -----------------------------------------------------------------------
	// Grapple config
	// -----------------------------------------------------------------------

	/** Maximum usable distance for a grapple target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleRange = 2200.0f;

	/** Max launch speed (capped when target is very far). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleVelocityMax = 5200.0f;

	/** Min launch speed (guaranteed even for close targets). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleVelocityMin = 1050.0f;

	/** Speed at which the hook end-point interpolates toward the target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.1"))
	float GrappleHookInterpSpeed = 34.0f;

	/** Duration for which the rope remains visible after the single launch impulse. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleRopeDisplayDuration = 0.25f;

	/** Delay (s) before restoring rotation flags after a grapple ends. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleReleaseDelay = 0.2f;

	/** Short delay after a grapple ends before another one can start. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleRestartDelay = 1.2f;

	/** Minimum distance required before an obstacle can become a valid grapple target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleMinTargetDistance = 425.0f;

	/** Extra buffer added around the obstacle before the grapple auto-detaches. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleAutoDetachBuffer = 120.0f;

	/** Minimum time before automatic proximity release can happen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleMinActiveDuration = 0.45f;

	/** Distance from the obstacle/anchor at which the grapple releases automatically. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleAutoReleaseDistance = 155.0f;

	/** Extra distance around the anchor where lateral orbiting releases the grapple. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleOrbitReleaseBuffer = 620.0f;

	/** Minimum lateral speed required to consider the player orbiting around the anchor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleOrbitReleaseMinLateralSpeed = 350.0f;

	/** Extra release distance used so fast grapples cut before the player reaches the obstacle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleEarlyDetachBuffer = 70.0f;

	/** Seconds of forward motion anticipated when deciding if the grapple should release. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleEarlyDetachLeadTime = 0.045f;

	/** Time (s) to ramp from the current momentum to the full grapple pull speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.01"))
	float GrapplePullBlendTime = 0.12f;

	/** Maximum pull time (s) before the grapple releases on its own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.1"))
	float GrappleMaxPullDuration = 1.2f;

	/** Share of the pull speed used to relaunch the player in the look direction just before impact. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0", ClampMax = "1.5"))
	float GrappleArrivalSpeedKeep = 0.8f;

	/** Upward speed (cm/s) added to the relaunch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleArrivalUpBoost = 300.0f;

	/** Time used for the white-to-red consume animation before the obstacle disappears. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.01"))
	float GrappleConsumedFadeDuration = 2.5f;

	/** Time the consumed obstacle stays hidden before it becomes usable again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleObstacleRespawnDelay = 0.0f;

	/** Extra rope length kept at grapple start to avoid an overly rigid snap. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleCableSlack = 36.0f;

	/** Constant pull applied toward the anchor while grappling. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrapplePullAcceleration = 18500.0f;

	/** Player-driven acceleration projected onto the swing plane. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleSwingAcceleration = 8200.0f;

	/** How quickly the rope shortens over time to create a grappling pull. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleRopeShortenSpeed = 2400.0f;

	/** Max speed allowed while the grapple is active. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleSwingMaxSpeed = 8400.0f;

	/** Minimum rope length to prevent collapsing directly into the anchor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "1.0"))
	float GrappleMinCableLength = 150.0f;

	/** Small carry boost preserved when the grapple is released. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleReleaseVelocityBoost = 1.02f;

	/** Reglage simple de force horizontale du grab. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Simple", meta = (DisplayName = "Force horizontale", ClampMin = "0.0", ToolTip = "Multiplicateur principal de la force vers l'obstacle."))
	float GrappleSimpleHorizontalForce = 1.0f;

	/** Reglage simple de hauteur du grab. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Simple", meta = (DisplayName = "Force hauteur", ClampMin = "0.0", ToolTip = "Multiplicateur principal de la hauteur du grab."))
	float GrappleSimpleHeightForce = 1.65f;

	/** Reglage simple du gain de puissance avec la distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Simple", meta = (DisplayName = "Puissance distance", ClampMin = "0.0", ToolTip = "Controle combien la distance augmente la puissance du grab."))
	float GrappleSimpleDistancePower = 1.15f;

	/** Reglage simple du gain vertical selon la difference de hauteur. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Simple", meta = (DisplayName = "Puissance difference hauteur", ClampMin = "0.0", ToolTip = "Controle combien la difference de hauteur augmente la puissance verticale."))
	float GrappleSimpleHeightDifferencePower = 1.35f;

	/** Reglage simple du gain horizontal selon la distance au sol. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Simple", meta = (DisplayName = "Puissance difference horizontale", ClampMin = "0.0", ToolTip = "Controle combien la distance horizontale augmente la puissance vers l'avant."))
	float GrappleSimpleHorizontalDifferencePower = 1.0f;

	/** Force du grab quand le joueur est proche de l'obstacle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning", meta = (DisplayName = "Force proche - lancement", ClampMin = "0.0", ToolTip = "Vitesse minimale appliquee au lancement du grab. Augmente cette valeur si le grab est trop faible quand tu es proche de l'obstacle."))
	float GrappleLaunchMinSpeed = 3900.0f;

	/** Force maximale du grab quand la cible est loin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning", meta = (DisplayName = "Force loin - lancement max", ClampMin = "0.0", ToolTip = "Vitesse maximale appliquee au lancement du grab. Augmente cette valeur si les grabs lointains manquent de puissance."))
	float GrappleLaunchMaxSpeed = 10800.0f;

	/** Distance maximale a laquelle un obstacle peut etre cible par le grab. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning", meta = (DisplayName = "Portee max de detection", ClampMin = "0.0", ToolTip = "Portee maximale utilisable pour detecter et accepter une cible de grab. Augmente si un obstacle tres loin est visible mais refuse ou ne declenche rien."))
	float GrappleEffectiveMaxRange = 10000.0f;

	/** Distance ou la force commence a augmenter entre proche et loin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning", meta = (DisplayName = "Debut montee puissance", ClampMin = "0.0", ToolTip = "Distance a partir de laquelle la puissance commence a monter entre MinSpeed et MaxSpeed. Baisse cette valeur pour avoir plus de force plus tot."))
	float GrappleLaunchDistanceStart = 250.0f;

	/** Longueur de transition entre force proche et force loin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning", meta = (DisplayName = "Distance transition puissance", ClampMin = "1.0", ToolTip = "Distance utilisee pour passer progressivement de la puissance proche a la puissance lointaine. Plus petit = la puissance monte plus vite."))
	float GrappleLaunchDistanceRange = 3600.0f;

	/** Forme de la courbe de force selon la distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning", meta = (DisplayName = "Courbe puissance distance", ClampMin = "0.1", ToolTip = "Forme de la courbe de puissance selon la distance. 1 = lineaire, plus haut = demarre plus doux puis accelere, plus bas = plus fort rapidement."))
	float GrappleLaunchDistanceExponent = 1.05f;

	/** Bonus qui conserve une partie de l'elan deja dirige vers l'obstacle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning", meta = (DisplayName = "Bonus elan lancement", ClampMin = "0.0", ToolTip = "Bonus base sur la vitesse deja dirigee vers l'obstacle au moment du lancement. Augmente pour garder plus d'elan."))
	float GrappleLaunchCarryBoost = 0.35f;

	/** Acceleration de base appliquee pendant le grab actif. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning", meta = (DisplayName = "Attraction active minimale", ClampMin = "0.0", ToolTip = "Acceleration minimale appliquee pendant que le grab est actif. Augmente si le personnage ne continue pas assez a etre attire."))
	float GrappleActiveMinPullAcceleration = 38000.0f;

	/** Distance ou la traction active commence a recevoir un bonus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning", meta = (DisplayName = "Debut bonus attraction loin", ClampMin = "0.0", ToolTip = "Distance a partir de laquelle la traction active recoit un bonus lie a l'eloignement. Baisse pour aider aussi les distances moyennes."))
	float GrappleActiveDistanceBoostStart = 700.0f;

	/** Intensite du bonus d'attraction quand la cible est loin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning", meta = (DisplayName = "Force bonus attraction loin", ClampMin = "0.0", ToolTip = "Force du bonus de traction selon la distance restante. Augmente si les obstacles lointains attirent trop lentement."))
	float GrappleActiveDistanceBoostScale = 3.6f;

	/** Vitesse minimale garantie vers l'obstacle pendant le grab. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning", meta = (DisplayName = "Vitesse active minimale", ClampMin = "0.0", ToolTip = "Vitesse minimale vers l'obstacle pendant le grab actif. Augmente si le grab proche reste trop mou apres le lancement."))
	float GrappleActiveMinTowardSpeed = 2800.0f;

	/** Vitesse maximale autorisee pendant le grab actif. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning", meta = (DisplayName = "Vitesse active maximale", ClampMin = "0.0", ToolTip = "Vitesse maximale autorisee pendant le grab actif. Augmente si le mouvement est bride trop tot."))
	float GrappleActiveMaxSpeed = 10800.0f;

	/** Angle minimum pour activer la trajectoire verticale quand le joueur est sous l'obstacle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning|Under Obstacle", meta = (DisplayName = "Angle activation sous obstacle", ClampMin = "-89.0", ClampMax = "89.0", ToolTip = "Angle vertical minimum vers l'obstacle pour declencher le mode sous obstacle. Plus bas = le grab monte plus souvent."))
	float GrappleUnderObstaclePitchThreshold = 35.0f;

	/** Angle de trajectoire vise quand le joueur est sous l'obstacle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning|Under Obstacle", meta = (DisplayName = "Angle lancement vertical", ClampMin = "0.0", ClampMax = "89.0", ToolTip = "Angle de lancement vise quand tu es sous l'obstacle. Plus haut = trajectoire plus verticale."))
	float GrappleUnderObstacleLaunchPitch = 68.0f;

	/** Melange entre trajectoire normale et trajectoire verticale sous obstacle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning|Under Obstacle", meta = (DisplayName = "Melange vertical sous obstacle", ClampMin = "0.0", ClampMax = "1.0", ToolTip = "Melange entre trajectoire normale et trajectoire verticale sous obstacle. 0 = normal, 1 = tres vertical."))
	float GrappleUnderObstacleVerticalBlend = 0.85f;

	/** Vitesse verticale ajoutee quand le joueur est sous l'obstacle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning|Under Obstacle", meta = (DisplayName = "Bonus vitesse verticale", ClampMin = "0.0", ToolTip = "Vitesse verticale ajoutee quand tu es sous l'obstacle. Augmente si ca ne monte pas assez."))
	float GrappleUnderObstacleExtraUpSpeed = 900.0f;

	/** Distance ou le grab commence a ajouter de la puissance verticale, meme si tu n'es pas pile sous l'obstacle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning|Long Distance Up", meta = (DisplayName = "Debut boost vertical loin", ClampMin = "0.0", ToolTip = "Distance a partir de laquelle les grabs lointains gagnent un bonus vers le haut. Baisse cette valeur si tu veux monter plus tot."))
	float GrappleFarUpBoostStart = 900.0f;

	/** Distance de transition du boost vertical longue distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning|Long Distance Up", meta = (DisplayName = "Transition boost vertical loin", ClampMin = "1.0", ToolTip = "Distance utilisee pour passer du bonus vertical minimum au bonus vertical maximum. Plus petit = le bonus arrive plus vite."))
	float GrappleFarUpBoostRange = 4200.0f;

	/** Vitesse verticale ajoutee au lancement quand la cible est loin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning|Long Distance Up", meta = (DisplayName = "Bonus vertical lancement loin", ClampMin = "0.0", ToolTip = "Vitesse verticale ajoutee au lancement du grab selon la distance. Augmente si un grab lointain part trop a plat."))
	float GrappleFarLaunchExtraUpSpeed = 1600.0f;

	/** Angle sous lequel un grab lointain reste presque plat si l'obstacle est surtout devant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning|Long Distance Up", meta = (DisplayName = "Angle grab plat", ClampMin = "-89.0", ClampMax = "89.0", ToolTip = "Si l'angle vertical vers l'obstacle est sous cette valeur, le grab garde une trajectoire plus directe vers l'avant."))
	float GrappleFlatForwardPitchThreshold = 8.0f;

	/** Angle a partir duquel le grab utilise toute sa hauteur/arc vertical. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning|Long Distance Up", meta = (DisplayName = "Angle hauteur max", ClampMin = "-89.0", ClampMax = "89.0", ToolTip = "Quand l'obstacle est au-dessus de cet angle, le bonus vertical longue distance est applique a pleine force."))
	float GrappleFullVerticalPitchThreshold = 32.0f;

	/** Acceleration verticale ajoutee pendant le grab actif quand la cible est loin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning|Long Distance Up", meta = (DisplayName = "Acceleration verticale active loin", ClampMin = "0.0", ToolTip = "Acceleration verticale appliquee pendant le grab actif selon la distance restante. Augmente si le personnage retombe trop vite sur les grabs lointains."))
	float GrappleFarActiveUpAcceleration = 6000.0f;

	/** Vitesse verticale minimale visee pendant un grab lointain. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning|Long Distance Up", meta = (DisplayName = "Vitesse verticale active loin", ClampMin = "0.0", ToolTip = "Vitesse verticale minimale visee quand le grab est loin. Augmente pour forcer une trajectoire plus montante."))
	float GrappleFarActiveMinUpSpeed = 1200.0f;

	/** Rend les obstacles trop proches transparents au lieu de les colorer comme une cible. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning|No Grapple", meta = (DisplayName = "Transparent si non grabbable", ToolTip = "Si active, un obstacle trop proche ou interdit au grab reste visible mais devient transparent. Le highlight de focus reste desactive."))
	bool bFadeNoGrappleObstacles = true;

	/** Opacite utilisee quand un obstacle est trop proche/interdit au grab. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning|No Grapple", meta = (DisplayName = "Opacite non grabbable", ClampMin = "0.0", ClampMax = "1.0", ToolTip = "Opacite visee pour un obstacle non grabbable. Necessite un material/BP qui utilise un parametre Opacity, Alpha, Transparency ou NoGrappleAlpha."))
	float NoGrappleObstacleOpacity = 0.12f;

	/** Laisse le C++ modifier les parametres d'opacite du material. Desactive par defaut pour eviter les materials noirs si le shader n'est pas prevu pour ca. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Tuning|No Grapple", meta = (DisplayName = "C++ modifie opacite material", ToolTip = "Active seulement si le material de l'obstacle est translucide et expose un parametre Opacity, Alpha, Transparency ou NoGrappleAlpha. Sinon laisse desactive et gere la transparence dans BP_ObstacleGrappin."))
	bool bUseNoGrappleMaterialOpacityParameters = false;

	/** Box half-size for the grapple target trace. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config")
	FVector GrappleTraceHalfSize = FVector(250.0f);

	/** Distance from camera at which the trace box starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleTraceStartOffset = 1000.0f;

	/** Distance from camera at which the trace box ends. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleTraceEndDistance = 10000.0f;

	/** Minimum screen radius used by grapple aim assist, in pixels. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Targeting", meta = (ClampMin = "0.0"))
	float GrappleAimAssistScreenRadiusMin = 72.0f;

	/** Screen radius ratio used by grapple aim assist. Smaller values make targeting more exact. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Targeting", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float GrappleAimAssistScreenRadiusRatio = 0.10f;

	/** Maximum camera angle allowed for assisted grapple target selection. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Targeting", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float GrappleAimAssistMaxAngleDegrees = 6.5f;

	/** Minimum upward pitch applied to the initial grapple arc. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Trajectory", meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float GrappleArcMinPitchDegrees = 18.0f;

	/** Maximum upward pitch applied to the initial grapple arc. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Trajectory", meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float GrappleArcMaxPitchDegrees = 28.0f;

	/** Time before the initial arc fully blends into direct pull toward the anchor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Trajectory", meta = (ClampMin = "0.01"))
	float GrappleArcBlendOutTime = 0.42f;

	/** Dead zone around the obstacle center where aiming stays a direct pull. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Surface Aim", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GrappleSurfaceAimDeadZone = 0.12f;

	/** Initial lateral speed added from the exact surface point aimed on the obstacle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Surface Aim", meta = (ClampMin = "0.0"))
	float GrappleSurfaceAimLaunchSpeed = 1250.0f;

	/** Lateral acceleration maintained from the aimed surface point while the grapple is active. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Surface Aim", meta = (ClampMin = "0.0"))
	float GrappleSurfaceAimAcceleration = 7200.0f;

	// -----------------------------------------------------------------------
	// Grapple state
	// -----------------------------------------------------------------------

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Grapple|State")
	bool bIsGrappling = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Grapple|State")
	bool bHasGrappleLocation = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Grapple|State")
	FVector GrappleTargetLocation = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Grapple|State")
	TObjectPtr<AActor> GrappleTargetActor;

	/** Actors currently inside NoGrappleZone — excluded from grapple traces. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Grapple|State")
	TArray<TObjectPtr<AActor>> ActorsNoGrappable;

	// -----------------------------------------------------------------------
	// Run camera effects
	// -----------------------------------------------------------------------

	/** Camera FOV at rest. 130 gives ORA its deliberate ultra-wide-angle framing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|FOV", meta = (ClampMin = "60.0", ClampMax = "150.0"))
	float DefaultFOV = 130.0f;

	/** Camera FOV when sprinting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|FOV", meta = (ClampMin = "60.0", ClampMax = "150.0"))
	float SprintFOV = 140.0f;

	/** How fast the FOV transitions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|FOV", meta = (ClampMin = "0.1"))
	float SprintFOVInterpSpeed = 5.0f;

	/** Subtle FOV breathing amplitude while at full sprint (adds life to the effect). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|FOV", meta = (ClampMin = "0.0"))
	float SprintFOVBreathAmplitude = 0.8f;

	/** Head bob vertical amplitude in cm (subtle = 2-4, strong = 6-8). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|Bob", meta = (ClampMin = "0.0"))
	float RunBobAmplitude = 3.0f;

	/** Head bob lateral (side) amplitude in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|Bob", meta = (ClampMin = "0.0"))
	float RunBobSideAmplitude = 1.2f;

	/** Camera drop applied directly to ORA's spring arm while ground sliding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|Slide", meta = (ClampMin = "0.0"))
	float GroundSlideRunCameraDrop = 420.0f;

	/** Camera arm reduction while ground sliding, making the slide feel lower and faster. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|Slide", meta = (ClampMin = "0.0"))
	float GroundSlideRunArmReduction = 110.0f;

	/** Extra FOV applied by ORA's run camera while ground sliding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|Slide", meta = (ClampMin = "0.0"))
	float GroundSlideRunFOVBoost = 2.0f;

	/** Seconds to reach the low slide camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|Slide", meta = (ClampMin = "0.01"))
	float GroundSlideRunCameraEnterDuration = 0.04f;

	/** Seconds to smoothly return from the low slide camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|Slide", meta = (ClampMin = "0.01"))
	float GroundSlideRunCameraReturnDuration = 0.95f;

	/** Bob oscillation frequency (steps per second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|Bob", meta = (ClampMin = "0.1"))
	float RunBobFrequency = 20.0f;

	// -----------------------------------------------------------------------
	// Run camera effects — strafe roll
	// -----------------------------------------------------------------------

	/** Max camera roll when strafing at full speed (degrees). Negative = left, positive = right. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|Roll", meta = (ClampMin = "0.0", ClampMax = "15.0"))
	float StrafeRollMaxAngle = 2.0f;

	/** Interpolation speed for the strafe roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|Roll", meta = (ClampMin = "0.1"))
	float StrafeRollInterpSpeed = 8.0f;

	// -----------------------------------------------------------------------
	// Run camera effects — post-process sprint
	// -----------------------------------------------------------------------

	/** Vignette intensity at rest (no sprint). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SprintVignetteMin = 0.4f;

	/** Vignette intensity at full sprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SprintVignetteMax = 0.65f;

	/** Max chromatic aberration intensity at full sprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|PostProcess", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float SprintChromaticMax = 0.4f;

	/** How fast SprintAlpha transitions (drives post-process and FOV). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|PostProcess", meta = (ClampMin = "0.1"))
	float SprintAlphaInterpSpeed = 5.0f;

	/** Extra FOV layered on top of sprint when the grapple swing reaches full speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Camera", meta = (ClampMin = "0.0"))
	float GrappleCameraFOVBoost = 9.0f;

	/** Extra chromatic aberration layered during fast grapples. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Camera", meta = (ClampMin = "0.0"))
	float GrappleCameraChromaticBoost = 0.55f;

	// -----------------------------------------------------------------------
	// Speed feedback — camera effects driven by the real speed, not only the sprint key
	// -----------------------------------------------------------------------

	/** Real speed (cm/s) where FOV / vignette / chromatic aberration start to ramp up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|SpeedFeedback", meta = (ClampMin = "0.0"))
	float SpeedEffectsStartSpeed = 800.0f;

	/** Real speed (cm/s) where the speed effects reach the sprint FOV. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|SpeedFeedback", meta = (ClampMin = "1.0"))
	float SpeedEffectsFullSpeed = 3600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|SpeedFeedback", meta = (ClampMin = "0.1"))
	float SpeedEffectsInterpSpeed = 6.0f;

	/** Extra FOV (deg) added beyond the sprint FOV between SpeedEffectsFullSpeed and SpeedFOVOverSpeedMaxSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|SpeedFeedback", meta = (ClampMin = "0.0", ClampMax = "40.0"))
	float SpeedFOVOverSpeedBoost = 15.0f;

	/** Real speed (cm/s) where the over-speed FOV bonus is complete. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|SpeedFeedback", meta = (ClampMin = "1.0"))
	float SpeedFOVOverSpeedMaxSpeed = 7000.0f;

	/** Maximum camera drop (cm) when landing from a big fall. 0 disables the dip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|SpeedFeedback", meta = (ClampMin = "0.0"))
	float LandingDipMaxDistance = 12.0f;

	/** Fall speed at impact (cm/s) giving the full landing dip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|SpeedFeedback", meta = (ClampMin = "1.0"))
	float LandingDipFullFallSpeed = 2000.0f;

	/** Time (s) for the landing dip to reach its lowest point before recovering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|SpeedFeedback", meta = (ClampMin = "0.01"))
	float LandingDipTimeToPeak = 0.08f;

	// -----------------------------------------------------------------------
	// Movement config
	// -----------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Gait")
	EMovementStickMode MovementStickMode = EMovementStickMode::FixedSpeedSingleGait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Gait", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnalogWalkRunThreshold = 0.5f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Movement|Gait")
	EORAGait Gait = EORAGait::Walk;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Gait")
	bool bWantsToSprint = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Gait")
	bool bWantsToWalk = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float MaxWalkSpeedBase = 3000.0f;

	// -----------------------------------------------------------------------
	// Ult
	// -----------------------------------------------------------------------

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Ult")
	float UltScore = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ult", meta = (ClampMin = "0.0"))
	float TimeUltRegen = 0.5f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ult")
	bool bCanUlt = false;

	// -----------------------------------------------------------------------
	// Obstacle
	// -----------------------------------------------------------------------

	/** All obstacle actors in the level — used by ResetBorderObstacle. Populate via BP or level setup. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
	TArray<TObjectPtr<AActor>> ObstacleActors;

	// -----------------------------------------------------------------------
	// Public functions — Shoot
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Shoot")
	void ShootBall();

	/** Stops this character's kinematic spline control when another player catches that ball. */
	bool CancelSplineFollowForBall(const AActor* BallActor);

	UFUNCTION(BlueprintCallable, Category = "Shoot")
	void SetControlPasse(bool bNewIsPassing);

	UFUNCTION(BlueprintCallable, Category = "Shoot")
	void SaveLastMovementPlayer();

	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void ApplyGravityParams(float NewGravityScale, float WalkableFloorAngle);

	// -----------------------------------------------------------------------
	// Public functions — Movement
	// -----------------------------------------------------------------------

	/** Returns Run or Walk based on current input, sprint/walk flags and MovementStickMode. */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	EORAGait GetDesiredGait() const;

	/** Updates Gait variable and applies MaxWalkSpeedBase to CharacterMovement. Called every tick. */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void UpdateMovement();

	/** Sets MaxWalkSpeed, MaxWalkSpeedCrouched, MaxFlySpeed, MaxCustomMovementSpeed to NewSpeed. */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ApplyMoveSpeed(float NewSpeed);

	/**
	 * Normalizes the 2D input vector when MovementStickMode is a Fixed Speed mode
	 * (converts analog magnitude to max, like keyboard input). Returns raw input in Variable modes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	FVector2D GetMovementInputScaleValue(FVector2D Input) const;

	// -----------------------------------------------------------------------
	// Public functions — Curve / Shoot helpers
	// -----------------------------------------------------------------------

	/**
	 * Returns (CameraRight * X) + (CameraUp * Z) in world space.
	 * Used for computing ball pass curve offset vectors.
	 */
	UFUNCTION(BlueprintCallable, Category = "Shoot")
	FVector ReturnVectorWithXAndYCurve(float X, float Z) const;

	// -----------------------------------------------------------------------
	// Public functions — Obstacle
	// -----------------------------------------------------------------------

	/**
	 * Resets the border material (M_BordObstacle) on all obstacles in ObstacleActors
	 * except the one passed as Exception (the current target keeps its highlight).
	 */
	UFUNCTION(BlueprintCallable, Category = "Obstacle")
	void ResetBorderObstacle(AActor* Exception);

	// -----------------------------------------------------------------------
	// Public functions — Ult / Player Parameters
	// -----------------------------------------------------------------------

	/** Caches HUD bars and starts the Ult recharge timer. Called once after HUD is ready. */
	UFUNCTION(BlueprintCallable, Category = "Ult")
	void SetBaseData();

	/** Fired every time UltScore changes so the UI (WD_Rounded_Progress) can update. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Ult")
	void OnUltScoreChanged(float Percent);

	UFUNCTION(BlueprintCallable, Category = "Shoot")
	void ApplyAntiSpamBlock(bool bBlocked);

	UFUNCTION(BlueprintImplementableEvent, Category = "Shoot")
	void OnShootStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Shoot")
	void OnShootEnded();

	// -----------------------------------------------------------------------
	// Public functions — Grapple
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Grapple")
	void TryStartGrapple();

	UFUNCTION(BlueprintCallable, Category = "Grapple")
	void EndGrapple();

	/** Called when a valid grapple target is found by the targeting trace. */
	UFUNCTION(BlueprintNativeEvent, Category = "Grapple")
	void OnGrappleTargetFound(AActor* Target, FVector WorldLocation);

	/** Called when the grapple target is lost (trace misses or target leaves). */
	UFUNCTION(BlueprintNativeEvent, Category = "Grapple")
	void OnGrappleTargetLost(AActor* PreviousTarget);

	/**
	 * Called just after the grapple launch.
	 * C++ default starts the hook animation. BP can override to add sound/VFX.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Grapple")
	void OnGrappleStarted(AActor* Target, FVector WorldLocation);

	/** Called when the grapple fully ends. */
	UFUNCTION(BlueprintNativeEvent, Category = "Grapple")
	void OnGrappleEnded();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void HandleStopBallInputPressed(const FInputActionValue& Value) override;
	virtual void HandleStopBallInputReleased(const FInputActionValue& Value) override;

	UFUNCTION(Server, Reliable)
	void ServerShootBall(AActor* BallActor, const TArray<FVector_NetQuantize10>& AimPoints,
		AORACharacterBase* PassTarget);

	/** Disable the proximity-based wall jump — wall slide jump (bWallSlideActive branch) is sufficient. */
	virtual bool ExecuteWallJump_Implementation() override;

private:
	// Run camera effects
	void UpdateRunCamera(float DeltaSeconds);
	float BobTime            = 0.0f;
	float CurrentBobAlpha    = 0.0f; // 0=stopped, 1=full speed
	FVector BaseSocketOffset = FVector::ZeroVector;
	FVector BaseCameraBoomRelativeLocation = FVector::ZeroVector;
	FVector BaseFollowCameraRelativeLocation = FVector::ZeroVector;
	float GroundSlideCameraAlpha = 0.0f;

	// Strafe roll
	float CurrentStrafeRoll = 0.0f;
	bool bRecoveringWallCameraRoll = false;
	bool bWasWallSlidingLastCameraUpdate = false;

	// Sprint alpha (0=stopped/walking, 1=full sprint) — drives post-process and FOV
	float SprintAlpha = 0.0f;
	float GrappleCameraAlpha = 0.0f;

	// Speed feedback (0 = below SpeedEffectsStartSpeed, 1 = at SpeedEffectsFullSpeed)
	float SpeedEffectsAlpha = 0.0f;
	// Over-speed (0 = at SpeedEffectsFullSpeed, 1 = at SpeedFOVOverSpeedMaxSpeed)
	float OverSpeedAlpha = 0.0f;

	// Landing dip: closed-form critically damped curve, peak LandingDipAmplitude at LandingDipTimeToPeak
	float LandingDipAmplitude = 0.0f;
	float LandingDipElapsed = 0.0f;
	float EvaluateLandingDipOffset(float DeltaSeconds);

	// Ground dash slide (friction management)
	void UpdateGroundDashFriction();
	bool  bGroundDashFrictionCleared  = false;
	float CachedGroundFriction        = 8.0f;
	float CachedBrakingDeceleration   = 2048.0f;

	// Shoot helpers
	void HandleShootInputTriggered();
	FTimerHandle AntiSpamTimerHandle;
	void StartAntiSpamCooldown();
	void ClearAntiSpamLock();

	// Ult recharge timer
	FTimerHandle RechargeUltTimerHandle;
	void RechargeUlt();

	// Grapple helpers
	void HandleGrappleInputTriggered();
	void HandleGrappleInputReleased();
	bool bGrappleInputLatched = false;
	/** Met à jour position + activation du rayon Niagara vers l'obstacle focalisé. */
	void UpdateGrabFocusBeam();
	void SyncGrappleObstacles();
	void UpdateGrappleTargeting(float DeltaSeconds);
	void UpdateActiveGrapple(float DeltaSeconds);
	void UpdateGrapplePull(float DeltaSeconds);
	void ApplyGrappleArrival();
	FVector ResolveGrappleAnchorNormal() const;
	void StartGrappleVisual();
	void UpdateGrappleHookVisual(float DeltaSeconds);
	FVector CalculateGrappleVelocity() const;
	FVector GetGrappleVisualStartLocation() const;
	FVector ResolveGrappleSurfaceAnchor(AActor* TargetActor, const FVector& PreferredLocation, const FVector& FromLocation) const;
	FVector CalculateGrappleSurfaceAimVector(AActor* TargetActor, const FVector& AnchorLocation, const FVector& PullDirection) const;
	float GetGrappleObstacleSurfaceDistance(const FVector& WorldLocation) const;
	AActor* ResolveBlueprintFocusedGrappleTarget(FVector& OutTargetLocation) const;
	bool IsManagedObstacle(const AActor* CandidateActor) const;
	bool CanUseGrappleTarget(AActor* CandidateActor, const FVector& CandidateLocation) const;
	bool IsGrappleObstacleOwnedByPlayerTeam(const AActor* CandidateActor) const;
	void ResolveGrappleTerrainManager();
	void RestoreObstacleEditorVisuals();
	void RefreshObstacleVisualState(AActor* Obstacle);
	void SetObstacleHighlightState(AActor* Obstacle, bool bHighlighted) const;
	void ApplyObstacleTint(AActor* Obstacle, const FLinearColor& TintColor) const;
	void UpdateConsumedObstacleFade(float DeltaSeconds);
	void UpdateConsumedObstacleRespawns(float DeltaSeconds);
	void RespawnGrappleObstacle(AActor* Obstacle);
	void BeginConsumeGrappleObstacle(AActor* Obstacle);
	void ConsumeGrappleObstacle(AActor* Obstacle);
	void FinishGrappleRelease();
	void StartGrappleCooldown();
	void ClearGrappleCooldown();

	FTimerHandle GrappleReleaseTimerHandle;
	FTimerHandle GrappleCooldownTimerHandle;
	float ObstacleVisualRestoreTimeRemaining = 0.35f;
	TWeakObjectPtr<AActor> GrappleTerrainManager;
	TMap<TWeakObjectPtr<AActor>, EORATeam> GrappleObstacleTeams;
	static constexpr float GrappleObstacleSyncInterval = 0.20f;
	float NextGrappleObstacleSyncTime = 0.0f;

	// Hook visual animation state
	bool bGrappleHookAnimating = false;
	bool bGrappleRopeRetracting = false;
	bool bGrappleCooldownActive = false;
	FVector GrappleHookCurrentPos = FVector::ZeroVector;
	FVector GrappleRopeRetractAnchor = FVector::ZeroVector;
	float GrappleRopeRetractElapsed = 0.0f;

	// Cached previous grapple target for OnGrappleTargetLost calls
	TObjectPtr<AActor> PreviousGrappleTargetActor;
	TWeakObjectPtr<AActor> ActiveGrappleObstacle;
	FVector GrappleAnchorLocation = FVector::ZeroVector;

	// Rope pull state (see UpdateGrapplePull)
	bool bGrapplePulling = false;
	float GrapplePullElapsed = 0.0f;
	float GrapplePullSpeed = 0.0f;
	FVector GrapplePullStartVelocity = FVector::ZeroVector;
	FVector GrapplePullLaunchDirection = FVector::ZeroVector;
	FVector GrappleAnchorNormal = FVector::ZeroVector;
	FVector GrappleArrivalPoint = FVector::ZeroVector;
	FVector GrappleInitialApproachDirection = FVector::ZeroVector;
	FVector GrappleSurfaceAimVector = FVector::ZeroVector;
	float GrappleCurrentCableLength = 0.0f;
	float ActiveGrappleReleaseDistance = 0.0f;
	float GrappleClosestDistanceToAnchor = 0.0f;
	float GrappleActiveTime = 0.0f;
	float GrappleNotApproachingTime = 0.0f;
	float GrappleSavedGravityScale = 1.75f;
	float GrappleSavedAirControl = 0.4f;
	float GrappleSavedBrakingDecelerationFalling = 0.0f;
	TSet<TWeakObjectPtr<AActor>> ConsumedGrappleObstacles;
	TSet<TWeakObjectPtr<AActor>> HiddenConsumedGrappleObstacles;
	TMap<TWeakObjectPtr<AActor>, float> GrappleObstacleConsumeProgress;
	TMap<TWeakObjectPtr<AActor>, float> GrappleObstacleRespawnProgress;

	// NoGrappleZone — tick-based distance check
	void UpdateNoGrappleZone();

	// NoGrappleZone overlap callbacks
	UFUNCTION()
	void OnNoGrappleZoneBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnNoGrappleZoneEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// -----------------------------------------------------------------------
	// Enrouler — suivi cinématique de la spline d'aim (non-UPROPERTY)
	// -----------------------------------------------------------------------
	bool  bSplineFollowActive           = false;
	float SplineFollowDistanceTraveled  = 0.0f;
	float SplineFollowTotalLength       = 0.0f;
	float SplineFollowSpeed             = 0.0f;
	bool  bSplineFollowPreservesCapturedSpeed = false;
	FVector SplineFollowExitTangent     = FVector::ZeroVector;
	TWeakObjectPtr<UPrimitiveComponent> SplineFollowPrimitive;
	TWeakObjectPtr<AActor> SplineFollowPassTarget;
	TArray<FVector> SplineFollowBasePoints;
	FVector SplineFollowInitialPassTargetLocation = FVector::ZeroVector;
	void UpdateSplineFollow(float DeltaSeconds);
	void UpdatePassHomingSpline();
	bool ShouldShowShotSplineDebug() const;
};

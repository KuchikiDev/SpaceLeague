# Capture des images 3D des croquis du site des variables.
# Monte une scene temporaire (jamais enregistree) avec le personnage ORA, place une camera par croquis
# et exporte une image PNG par croquis dans ORA_CAPTURE_OUT. Le site dessine ensuite les fleches et
# les valeurs par-dessus, en direct.
#
# Lancement (seconde instance de l'editeur, l'autre peut rester ouverte) :
#   UnrealEditor.exe "<projet>.uproject" -ExecCmds="py <ce fichier>" -unattended -nosound -windowed -ResX=1280 -ResY=720
#   (pas -ExecutePythonScript : l editeur se ferme juste apres le script, avant les captures)
# avec la variable d'environnement ORA_CAPTURE_OUT = dossier de sortie.

import os
import math
import unreal

OUT = os.environ.get('ORA_CAPTURE_OUT', os.path.join(unreal.Paths.project_saved_dir(), 'SketchCapture'))
ONLY = [s for s in os.environ.get('ORA_CAPTURE_ONLY', '').split(',') if s]
W, H = 1280, 720
CHARACTER = '/Game/Legends/BP_Paradoxe.BP_Paradoxe_C'
BALL = '/Game/Terrain/Blueprints/BP_Ball.BP_Ball_C'

log = lambda *a: unreal.log('[SketchCapture] ' + ' '.join(str(x) for x in a))
os.makedirs(OUT, exist_ok=True)

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
V = unreal.Vector
R = unreal.Rotator


def spawn(cls, loc=V(0, 0, 0), rot=R(0, 0, 0)):
    return eas.spawn_actor_from_class(cls, loc, rot)


def spawn_mesh(path, loc, scale, rot=R(0, 0, 0), material=None):
    a = spawn(unreal.StaticMeshActor, loc, rot)
    c = a.static_mesh_component
    c.set_static_mesh(unreal.load_asset(path))
    a.set_actor_scale3d(scale)
    if material:
        c.set_material(0, material)
    return a


def look_at(src, dst):
    d = dst - src
    yaw = math.degrees(math.atan2(d.y, d.x))
    pitch = math.degrees(math.atan2(d.z, math.hypot(d.x, d.y)))
    return R(roll=0, pitch=pitch, yaw=yaw)


# --- scene ---------------------------------------------------------------
world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

# Soleil cote camera (les cameras sont du cote +Y et regardent vers -Y, donc +X va vers la droite de l image) :
# personnage eclaire de face, ombre portee derriere lui.
sun = spawn(unreal.DirectionalLight, V(0, 0, 1000), R(roll=0, pitch=-40, yaw=float(os.environ.get('ORA_CAPTURE_SUN_YAW', '-55'))))
sun.set_actor_rotation(R(roll=0, pitch=-40, yaw=float(os.environ.get('ORA_CAPTURE_SUN_YAW', '-55'))), False)
log('soleil direction', sun.get_actor_forward_vector())
sun.light_component.set_intensity(8.0)
sun.light_component.set_editor_property('atmosphere_sun_light', True)
spawn(unreal.SkyAtmosphere)
sky = spawn(unreal.SkyLight, V(0, 0, 500))
sky.light_component.set_editor_property('real_time_capture', True)
sky.light_component.set_intensity(3.0)

shape_mat = unreal.load_asset('/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial')


def paint(actor, rgb):
    """Couleur unie (materiau de base des formes Unreal, parametre Color)."""
    comp = actor.static_mesh_component
    mid = comp.create_dynamic_material_instance(0, shape_mat)
    mid.set_vector_parameter_value('Color', unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1))


floor = spawn_mesh('/Engine/BasicShapes/Plane.Plane', V(0, 0, 0), V(3000, 3000, 1))
paint(floor, (0.2, 0.21, 0.24))
# Lignes au sol tous les 5 m : donnent l'echelle sur les images.
for k in range(-20, 41):
    paint(spawn_mesh('/Engine/BasicShapes/Plane.Plane', V(k * 500, 0, 1.0), V(0.05, 80, 1)), (0.12, 0.13, 0.15))

char_cls = unreal.load_class(None, CHARACTER)
char = spawn(char_cls, V(0, 0, 92))
# Le personnage a plusieurs maillages (corps, tete, vetements...) : on anime celui qui porte le corps,
# les autres suivent via leur leader pose.
mesh = None
for comp in char.get_components_by_class(unreal.SkeletalMeshComponent):
    asset = comp.get_skeletal_mesh_asset()
    log('maillage', comp.get_name(), asset.get_path_name() if asset else None)
    if asset and (mesh is None or comp.get_name().lower() in ('mesh', 'charactermesh0', 'body')):
        mesh = comp
# Le skin est applique en jeu : en editeur le maillage est vide. On prend celui des skins de legendes.
if mesh is None:
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
    deps = [str(d) for d in (reg.get_dependencies('/Game/Legends/DA_LegendSkins', opts) or [])]
    deps += [str(d) for d in (reg.get_dependencies('/Game/Legends/BP_Paradoxe', opts) or [])]
    log('dependances skins', deps)
    candidates = deps + ['/Game/Legends/SM_Personnage', '/Game/Legends/Models/Meshy_AI_Character_output']
    comp = char.get_component_by_class(unreal.SkeletalMeshComponent)
    for path in candidates:
        asset = unreal.load_asset(path.split('.')[0])
        if isinstance(asset, unreal.SkeletalMesh):
            comp.set_skeletal_mesh_asset(asset)
            mesh = comp
            log('skin utilise', path)
            break
log('personnage', char.get_name(), 'maillage anime', mesh.get_name() if mesh else None)

# Animations compatibles avec le squelette du personnage : on les liste pour choisir les poses.
anims = {}
if mesh:
    skel = mesh.get_skeletal_mesh_asset().skeleton
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    for data in reg.get_assets_by_class(unreal.TopLevelAssetPath('/Script/Engine', 'AnimSequence'), True):
        tag = str(data.get_tag_value('Skeleton') or '')
        if skel and skel.get_path_name() in tag:
            anims[str(data.asset_name)] = str(data.package_name) + '.' + str(data.asset_name)
    log('animations compatibles :', len(anims), sorted(anims)[:200])


def pose(*keywords, t=0.3):
    """Pose fixe tiree d'une animation dont le nom contient un des mots-cles."""
    if not mesh:
        return
    for k in keywords:
        for name, path in sorted(anims.items(), key=lambda x: len(x[0])):
            if k.lower() in name.lower():
                anim = unreal.load_asset(path)
                mesh.set_update_animation_in_editor(True)
                mesh.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
                mesh.set_animation(anim)
                mesh.set_position(anim.get_play_length() * t, False)
                log('pose', name, 'a', t)
                return
    log('pas d\'animation pour', keywords)


# Balle : sphere jaune de 50 cm (la BP_Ball du jeu n'a pas d'echelle lisible hors jeu).
ball = spawn_mesh('/Engine/BasicShapes/Sphere.Sphere', V(0, 0, -5000), V(0.5, 0.5, 0.5))
paint(ball, (1.0, 0.7, 0.1))
wall = spawn_mesh('/Engine/BasicShapes/Cube.Cube', V(0, 0, -5000), V(30, 0.4, 8))
paint(wall, (0.55, 0.6, 0.7))
target = spawn_mesh('/Engine/BasicShapes/Sphere.Sphere', V(0, 0, -5000), V(1.2, 1.2, 1.2))
paint(target, (0.15, 0.5, 0.95))
hidden = V(0, 0, -5000)

# --- camera --------------------------------------------------------------
rt = unreal.RenderingLibrary.create_render_target2d(world, W, H, unreal.TextureRenderTargetFormat.RTF_RGBA8)
cap = spawn(unreal.SceneCapture2D)
cc = cap.capture_component2d
cc.set_editor_property('texture_target', rt)
cc.set_editor_property('capture_source', unreal.SceneCaptureSource.SCS_FINAL_TONE_CURVE_HDR)
cc.set_editor_property('capture_every_frame', False)
cc.set_editor_property('capture_on_movement', False)
# Exposition fixe : sinon l'auto-exposition assombrit la scene (ciel tres lumineux).
pp = cc.get_editor_property('post_process_settings')
pp.set_editor_property('override_auto_exposure_method', True)
pp.set_editor_property('auto_exposure_method', unreal.AutoExposureMethod.AEM_MANUAL)
pp.set_editor_property('override_auto_exposure_apply_physical_camera_exposure', True)
pp.set_editor_property('auto_exposure_apply_physical_camera_exposure', False)
pp.set_editor_property('override_auto_exposure_bias', True)
pp.set_editor_property('auto_exposure_bias', float(os.environ.get('ORA_CAPTURE_EXPOSURE', '1.8')))
cc.set_editor_property('post_process_settings', pp)
cc.set_editor_property('post_process_blend_weight', 1.0)
cameras = {}


current = {'name': ''}


def setup(cam, at, fov=50, ball_at=hidden, wall_at=hidden, wall_rot=R(0, 0, 0), target_at=hidden, char_at=V(0, 0, 92), char_yaw=0):
    char.set_actor_location_and_rotation(char_at, R(roll=0, pitch=0, yaw=char_yaw), False, True)
    # Pieds au sol : on recale la hauteur sur le bas du maillage.
    if mesh:
        origin, extent, _ = unreal.SystemLibrary.get_component_bounds(mesh)
        low = origin.z - extent.z
        log('bas du maillage', low)
        char.set_actor_location(V(char_at.x, char_at.y, char_at.z - low + 1), False, True)
    if ball:
        ball.set_actor_location(ball_at, False, True)
    wall.set_actor_location_and_rotation(wall_at, wall_rot, False, True)
    target.set_actor_location(target_at, False, True)
    cap.set_actor_location_and_rotation(cam, look_at(cam, at), False, True)
    cc.set_editor_property('fov_angle', fov)
    rot = look_at(cam, at)
    # Camera exportee pour que le site projette ses dessins (positions en cm) sur l'image.
    cameras[current['name']] = {'loc': [cam.x, cam.y, cam.z], 'pitch': rot.pitch, 'yaw': rot.yaw, 'fov': fov, 'w': W, 'h': H,
                                'char': [char_at.x, char_at.y, char_at.z - 92]}


# Chaque croquis : reglage de la scene. Le joueur court vers +X ; camera de profil = depuis -Y.
P = lambda: pose('Meshy_AI_Character_output_Anim', t=0.0)
SHOTS = {
    # Saut : profil tres large (un saut fait ~25 m de long), joueur au sol a gauche, place pour un double saut.
    'jump': lambda: (setup(V(2300, 5200, 800), V(2300, 0, 750), fov=60, char_at=V(0, 0, 92), char_yaw=0), P()),
    # Mur : trois quarts plongeant, mur le long de X derriere le joueur, le joueur court vers +X.
    'wall': lambda: (setup(V(-500, 1000, 700), V(350, -60, 180), fov=55, wall_at=V(600, -110, 400), char_at=V(0, -40, 92), char_yaw=0), P()),
    # Grappin : profil, cible bleue en hauteur a droite.
    'grapple': lambda: (setup(V(900, 2700, 500), V(900, 0, 450), fov=60, target_at=V(1800, 0, 900), char_at=V(0, 0, 92), char_yaw=0), P()),
    # Balle : trois quarts arriere, balle en orbite a cote du joueur.
    'ball': lambda: (setup(V(-380, 330, 240), V(30, 0, 130), fov=55, ball_at=V(80, -110, 160), char_at=V(0, 0, 92), char_yaw=0), P()),
}

names = [n for n in SHOTS if not ONLY or n in ONLY]
state = {'frame': 0, 'i': 0, 'phase': 'warmup'}
WARMUP = 400  # images avant la premiere capture : compilation des shaders, ciel, streaming


def tick(dt):
    state['frame'] += 1
    f = state['frame']
    if state['phase'] == 'warmup':
        if f >= WARMUP:
            state['phase'] = 'setup'
        return
    if state['i'] >= len(names):
        if state['phase'] != 'done':
            state['phase'] = 'done'
            import json
            # Fusion avec les cameras deja capturees (recapture partielle via ORA_CAPTURE_ONLY).
            path = os.path.join(OUT, 'cameras.json')
            merged = json.load(open(path)) if os.path.exists(path) else {}
            merged.update(cameras)
            with open(path, 'w') as fh:
                json.dump(merged, fh, indent=1)
            log('termine', OUT)
            unreal.unregister_slate_post_tick_callback(handle)
            unreal.SystemLibrary.quit_editor()
        return
    name = names[state['i']]
    if state['phase'] == 'setup':
        current['name'] = name
        SHOTS[name]()
        state['phase'] = 'settle'
        state['wait'] = f + 60
    elif state['phase'] == 'settle' and f >= state['wait']:
        cc.capture_scene()
        state['phase'] = 'export'
        state['wait'] = f + 5
    elif state['phase'] == 'export' and f >= state['wait']:
        unreal.RenderingLibrary.export_render_target(world, rt, OUT, name + '.png')
        log('image', name)
        state['i'] += 1
        state['phase'] = 'setup'


handle = unreal.register_slate_post_tick_callback(tick)
log('scene prete, captures :', names)

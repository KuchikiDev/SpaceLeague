import unreal

paths = [
    "/Game/Instance/GI_SpaceLeague",
    "/Game/Game/Core/GI_SpaceLeague",
]

for p in paths:
    cls = unreal.EditorAssetLibrary.load_blueprint_class(p)
    if not cls:
        unreal.log(f"GI_CLASS3:{p}:load_failed")
        continue
    unreal.log(f"GI_CLASS3:{p}:name={cls.get_name()}:path={cls.get_path_name()}")

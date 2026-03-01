import unreal

paths = [
    "/Game/Instance/GI_SpaceLeague",
    "/Game/Game/Core/GI_SpaceLeague",
]

for p in paths:
    cls = unreal.EditorAssetLibrary.load_blueprint_class(p)
    if not cls:
        unreal.log(f"GI_CLASS2:{p}:load_failed")
        continue

    super_name = "None"
    try:
        s = cls.get_super_struct()
        super_name = s.get_name() if s else "None"
    except Exception:
        pass

    unreal.log(f"GI_CLASS2:{p}:{cls.get_name()}:super={super_name}")

    # list non-transient properties for rough comparison
    prop_names = []
    try:
        for prop in cls.properties():
            n = prop.get_name()
            if n not in ("UberGraphFrame",):
                prop_names.append(n)
    except Exception:
        pass

    unreal.log(f"GI_PROPS2:{p}:count={len(prop_names)}")
    unreal.log(f"GI_PROPS2_NAMES:{p}:{','.join(sorted(set(prop_names)))}")

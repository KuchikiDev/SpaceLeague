import unreal

paths = [
    "/Game/Instance/GI_SpaceLeague",
    "/Game/Game/Core/GI_SpaceLeague",
]

for p in paths:
    cls = unreal.EditorAssetLibrary.load_blueprint_class(p)
    if not cls:
        unreal.log(f"GI_CLASS:{p}:load_failed")
        continue
    super_cls = cls.get_super_class()
    super_name = super_cls.get_name() if super_cls else "None"
    unreal.log(f"GI_CLASS:{p}:{cls.get_name()}:super={super_name}")

    cdo = unreal.get_default_object(cls)
    if cdo:
        props = []
        for prop_name in ["LegendDataMap", "LegendData", "CurrentLegend", "PlayerData"]:
            try:
                val = cdo.get_editor_property(prop_name)
                props.append(f"{prop_name}=set")
            except Exception:
                pass
        unreal.log(f"GI_CDO_PROPS:{p}:{';'.join(props)}")

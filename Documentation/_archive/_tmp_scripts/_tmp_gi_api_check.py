import unreal

ASSETS = {
    "instance": "/Game/Instance/GI_SpaceLeague",
    "core": "/Game/Game/Core/GI_SpaceLeague",
}
FUNC_CHECK = [
    "GetSelectedLegendData",
    "SetHubSelection",
    "GetHubSelection",
]
PROP_CHECK = [
    "SelectedCharacterClass",
    "OnHubDataChanged",
]

for label, path in ASSETS.items():
    cls = unreal.EditorAssetLibrary.load_blueprint_class(path)
    bp = unreal.EditorAssetLibrary.load_asset(path)
    unreal.log(f"GI_API:{label}:class_loaded={cls is not None}:bp_loaded={bp is not None}")
    if not cls:
        continue

    fn_names = set()
    try:
        for fn in cls.functions():
            fn_names.add(fn.get_name())
    except Exception:
        pass

    prop_names = set()
    try:
        for p in cls.properties():
            prop_names.add(p.get_name())
    except Exception:
        pass

    for fn in FUNC_CHECK:
        unreal.log(f"GI_API:{label}:func:{fn}:{fn in fn_names}")
    for pr in PROP_CHECK:
        unreal.log(f"GI_API:{label}:prop:{pr}:{pr in prop_names}")

import unreal

paths = [
    "/Game/Game/Data/Structs/ST_AbilityPresentationOverride",
    "/Game/Game/Data/Structs/ST_ParamOverrideEntry",
    "/Game/Game/Data/Structs/ST_CooldownOverrideEntry",
    "/Game/Game/Data/Structs/ST_SkinList",
]

for p in paths:
    obj = unreal.EditorAssetLibrary.load_asset(p)
    unreal.log(f"STRUCT_PROBE:{p}:loaded={obj is not None}")
    if not obj:
        continue
    unreal.log(f"STRUCT_PROBE:{p}:class={obj.get_class().get_name()}")
    # Best effort property introspection for UserDefinedStruct asset object
    try:
        props = [prop.get_name() for prop in obj.get_class().properties()]
        unreal.log(f"STRUCT_PROBE:{p}:asset_props={','.join(props)}")
    except Exception as e:
        unreal.log(f"STRUCT_PROBE:{p}:asset_props_error={e}")
    try:
        desc = unreal.get_type_from_class(obj.get_class())
        unreal.log(f"STRUCT_PROBE:{p}:type_from_class={desc}")
    except Exception:
        pass
    try:
        for name in ["Guid", "guid", "EditorData", "editor_data", "Status", "PrimaryStruct"]:
            try:
                v = obj.get_editor_property(name)
                unreal.log(f"STRUCT_PROBE:{p}:editor_property:{name}={v}")
            except Exception:
                pass
    except Exception:
        pass
    unreal.log(f"STRUCT_PROBE:{p}:dir_sample={','.join(sorted([n for n in dir(obj) if ('struct' in n.lower() or 'member' in n.lower() or 'field' in n.lower())])[:50])}")

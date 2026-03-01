import unreal

paths = [
    "/Game/Instance/GI_SpaceLeague",
    "/Game/Game/Core/GI_SpaceLeague",
]

ar = unreal.AssetRegistryHelpers.get_asset_registry()
for p in paths:
    exists = unreal.EditorAssetLibrary.does_asset_exist(p)
    ad = ar.get_asset_by_object_path(f"{p}.{p.split('/')[-1]}")
    unreal.log(f"GI_INFO:{p}:exists={exists}:valid={ad.is_valid()}")
    if ad.is_valid():
        unreal.log(f"GI_META:{p}:asset_class={ad.asset_class_path.asset_name}:package={ad.package_name}")
    obj = unreal.EditorAssetLibrary.load_asset(p)
    if obj:
        cls = obj.get_class()
        unreal.log(f"GI_LOADED:{p}:class={cls.get_name()}")
        if hasattr(obj, 'generated_class'):
            gc = obj.generated_class
            unreal.log(f"GI_GENCLASS:{p}:{gc}")
        if hasattr(obj, 'parent_class'):
            pc = obj.parent_class
            unreal.log(f"GI_PARENT:{p}:{pc}")
    else:
        unreal.log(f"GI_LOADED:{p}:None")

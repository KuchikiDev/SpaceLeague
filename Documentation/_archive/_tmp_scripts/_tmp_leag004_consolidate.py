import unreal

TARGET_PATH = "/Game/Instance/GI_SpaceLeague"
SOURCE_PATH = "/Game/Game/Core/GI_SpaceLeague"

unreal.log(f"LEAG004:target={TARGET_PATH}")
unreal.log(f"LEAG004:source={SOURCE_PATH}")

target = unreal.EditorAssetLibrary.load_asset(TARGET_PATH)
source = unreal.EditorAssetLibrary.load_asset(SOURCE_PATH)

if not target:
    unreal.log_error("LEAG004:target_load_failed")
    raise RuntimeError("Target GI not found")
if not source:
    unreal.log_warning("LEAG004:source_already_missing")
else:
    ok = unreal.EditorAssetLibrary.consolidate_assets(target, [source])
    unreal.log(f"LEAG004:consolidate_ok={ok}")

# Save all dirty packages to persist redirector/ref updates
saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log(f"LEAG004:save_dirty_packages={saved}")

exists_target = unreal.EditorAssetLibrary.load_asset(TARGET_PATH) is not None
exists_source = unreal.EditorAssetLibrary.load_asset(SOURCE_PATH) is not None
unreal.log(f"LEAG004:exists_target={exists_target}")
unreal.log(f"LEAG004:exists_source={exists_source}")

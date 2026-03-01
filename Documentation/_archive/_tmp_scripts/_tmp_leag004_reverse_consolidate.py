import unreal

TARGET_PATH = "/Game/Game/Core/GI_SpaceLeague"
SOURCE_PATH = "/Game/Instance/GI_SpaceLeague"

unreal.log(f"LEAG004_REV:target={TARGET_PATH}")
unreal.log(f"LEAG004_REV:source={SOURCE_PATH}")

target = unreal.EditorAssetLibrary.load_asset(TARGET_PATH)
source = unreal.EditorAssetLibrary.load_asset(SOURCE_PATH)

if not target:
    unreal.log_error("LEAG004_REV:target_load_failed")
    raise RuntimeError("Target GI not found")
if not source:
    unreal.log_warning("LEAG004_REV:source_missing")
else:
    ok = unreal.EditorAssetLibrary.consolidate_assets(target, [source])
    unreal.log(f"LEAG004_REV:consolidate_ok={ok}")

saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log(f"LEAG004_REV:save_dirty_packages={saved}")

exists_target = unreal.EditorAssetLibrary.load_asset(TARGET_PATH) is not None
exists_source = unreal.EditorAssetLibrary.load_asset(SOURCE_PATH) is not None
unreal.log(f"LEAG004_REV:exists_target={exists_target}")
unreal.log(f"LEAG004_REV:exists_source={exists_source}")

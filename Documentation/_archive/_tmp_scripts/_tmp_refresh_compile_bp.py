import unreal

TARGETS = [
    "/Game/Instance/GI_SpaceLeague",
    "/Game/Game/Core/GI_SpaceLeague",
    "/Game/Instance/GI_Proxy",
    "/Game/Legends/BP_Paradoxe",
    "/Game/UI/Tmp_HUD/UW_HUD",
    "/Game/Levels/HUB/UW_MainMenu",
    "/Game/Maps/BPPC_Paradoxe",
    "/Game/Levels/SandboxMap/GM_Sandbox",
]

has_refresh = hasattr(unreal, "BlueprintEditorLibrary") and hasattr(unreal.BlueprintEditorLibrary, "refresh_all_nodes")
unreal.log(f"REFRESH_COMPILE:has_refresh={has_refresh}")

for path in TARGETS:
    bp = unreal.EditorAssetLibrary.load_asset(path)
    if not bp:
        unreal.log_warning(f"REFRESH_COMPILE:missing:{path}")
        continue

    is_bp = isinstance(bp, unreal.Blueprint)
    is_widget_bp = isinstance(bp, unreal.WidgetBlueprint)
    if not (is_bp or is_widget_bp):
        unreal.log_warning(f"REFRESH_COMPILE:not_blueprint:{path}:{bp.get_class().get_name()}")
        continue

    try:
        if has_refresh:
            unreal.BlueprintEditorLibrary.refresh_all_nodes(bp)
            unreal.log(f"REFRESH_COMPILE:refreshed:{path}")
    except Exception as e:
        unreal.log_warning(f"REFRESH_COMPILE:refresh_failed:{path}:{e}")

    try:
        unreal.KismetEditorUtilities.compile_blueprint(bp)
        unreal.log(f"REFRESH_COMPILE:compiled:{path}")
    except Exception as e:
        unreal.log_warning(f"REFRESH_COMPILE:compile_failed:{path}:{e}")

saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log(f"REFRESH_COMPILE:save_dirty_packages={saved}")

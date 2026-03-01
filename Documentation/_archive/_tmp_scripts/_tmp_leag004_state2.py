import unreal
import json
import os
from datetime import datetime

OUT = r"s:\Programmation\Jeu\UE5\Byakuya\Leagues\Documentation\reports\leag004-state-current.json"
TARGETS = [
    "/Game/Instance/GI_SpaceLeague",
    "/Game/Game/Core/GI_SpaceLeague",
    "/Game/Levels/SandboxMap/GM_Sandbox",
    "/Game/Instance/GI_Proxy",
    "/Game/UI/Tmp_HUD/UW_HUD",
    "/Game/Legends/BP_Paradoxe",
    "/Game/Maps/BPPC_Paradoxe",
    "/Game/Levels/HUB/UW_MainMenu",
]

ar = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(
    include_hard_package_references=True,
    include_soft_package_references=True,
    include_searchable_names=False,
    include_soft_management_references=True,
    include_hard_management_references=True,
)

def info(path):
    obj = unreal.EditorAssetLibrary.load_asset(path)
    refs = ar.get_referencers(path, opts) or []
    deps = ar.get_dependencies(path, opts) or []
    return {
        "exists": obj is not None,
        "class": obj.get_class().get_name() if obj else None,
        "referencers": sorted(str(x) for x in refs),
        "dependencies": sorted(str(x) for x in deps),
    }

result = {
    "generated_at": datetime.utcnow().isoformat() + "Z",
    "assets": {p: info(p) for p in TARGETS},
}

os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(result, f, indent=2)

unreal.log("LEAG004_STATE2_WRITTEN:" + OUT)

import unreal
import json
import os
from datetime import datetime

OUT = r"s:\Programmation\Jeu\UE5\Byakuya\Leagues\Documentation\reports\leag004-state-post-rollback.json"
TARGETS = [
    "/Game/Instance/GI_SpaceLeague",
    "/Game/Game/Core/GI_SpaceLeague",
    "/Game/Levels/SandboxMap/GM_Sandbox",
]

ar = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(
    include_hard_package_references=True,
    include_soft_package_references=True,
    include_searchable_names=False,
    include_soft_management_references=True,
    include_hard_management_references=True,
)

result = {"generated_at": datetime.utcnow().isoformat() + "Z", "assets": {}}

for p in TARGETS:
    obj = unreal.EditorAssetLibrary.load_asset(p)
    refs = ar.get_referencers(p, opts) or []
    deps = ar.get_dependencies(p, opts) or []
    result["assets"][p] = {
        "exists": obj is not None,
        "referencers": sorted(str(x) for x in refs),
        "dependencies": sorted(str(x) for x in deps),
    }

os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(result, f, indent=2)

unreal.log("LEAG004_POSTROLLBACK_WRITTEN:" + OUT)

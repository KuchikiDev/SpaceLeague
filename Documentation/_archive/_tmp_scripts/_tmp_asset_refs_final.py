import unreal
import json
import os
from datetime import datetime

TARGETS = [
    "/Game/Instance/GI_SpaceLeague",
    "/Game/Game/Core/GI_SpaceLeague",
    "/Game/Legends/Blueprints/BP_ParadoxeJhin",
    "/Game/Legends/Blueprints/BP_ParadoxeSenna",
]
OLD_ROOT = "/Game/Game/Old"
OUT_FILE = r"s:\Programmation\Jeu\UE5\Byakuya\Leagues\Documentation\reports\asset-referencers-phase0.json"

ar = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(
    include_hard_package_references=True,
    include_soft_package_references=True,
    include_searchable_names=False,
    include_soft_management_references=True,
    include_hard_management_references=True,
)

def exists(path):
    try:
        return unreal.EditorAssetLibrary.load_asset(path) is not None
    except Exception:
        return False

result = {
    "generated_at": datetime.utcnow().isoformat() + "Z",
    "targets": {},
    "old_assets": {},
}

for p in TARGETS:
    refs = ar.get_referencers(p, opts) or []
    result["targets"][p] = {
        "exists": bool(exists(p)),
        "referencers": sorted(str(x) for x in refs),
    }

old_assets = ar.get_assets_by_path(OLD_ROOT, recursive=True) or []
for ad in old_assets:
    p = str(ad.package_name)
    refs = ar.get_referencers(p, opts) or []
    result["old_assets"][p] = {
        "exists": bool(exists(p)),
        "referencers": sorted(str(x) for x in refs),
    }

os.makedirs(os.path.dirname(OUT_FILE), exist_ok=True)
with open(OUT_FILE, "w", encoding="utf-8") as f:
    json.dump(result, f, indent=2)

unreal.log("PHASE0_REFERENCERS_FINAL:" + OUT_FILE)

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

asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
dep_opts = unreal.AssetRegistryDependencyOptions(
    include_hard_package_references=True,
    include_soft_package_references=True,
    include_searchable_names=False,
    include_soft_management_references=True,
    include_hard_management_references=True,
)

result = {
    "generated_at": datetime.utcnow().isoformat() + "Z",
    "targets": {},
    "old_assets": {},
}

def to_str_list(values):
    if not values:
        return []
    return sorted(str(v) for v in values)

def asset_exists(package_name: str) -> bool:
    try:
        obj = unreal.EditorAssetLibrary.load_asset(package_name)
        return obj is not None
    except Exception:
        return False

for package_name in TARGETS:
    exists = asset_exists(package_name)
    refs = asset_registry.get_referencers(package_name, dep_opts)
    result["targets"][package_name] = {
        "exists": bool(exists),
        "referencers": to_str_list(refs),
    }

old_assets = asset_registry.get_assets_by_path(OLD_ROOT, recursive=True)
for asset_data in old_assets:
    package_name = str(asset_data.package_name)
    exists = asset_exists(package_name)
    refs = asset_registry.get_referencers(package_name, dep_opts)
    result["old_assets"][package_name] = {
        "exists": bool(exists),
        "referencers": to_str_list(refs),
    }

os.makedirs(os.path.dirname(OUT_FILE), exist_ok=True)
with open(OUT_FILE, "w", encoding="utf-8") as f:
    json.dump(result, f, indent=2)

unreal.log("WROTE_REFERENCERS:" + OUT_FILE)

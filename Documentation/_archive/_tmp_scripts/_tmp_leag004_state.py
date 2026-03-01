import unreal
import json
import os
from datetime import datetime

OUT = r"s:\Programmation\Jeu\UE5\Byakuya\Leagues\Documentation\reports\leag004-state-after-consolidate-attempt.json"
TARGET = "/Game/Instance/GI_SpaceLeague"
SOURCE = "/Game/Game/Core/GI_SpaceLeague"

ar = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(
    include_hard_package_references=True,
    include_soft_package_references=True,
    include_searchable_names=False,
    include_soft_management_references=True,
    include_hard_management_references=True,
)

def get_info(path):
    obj = unreal.EditorAssetLibrary.load_asset(path)
    exists = obj is not None
    cls = obj.get_class().get_name() if obj else None
    refs = ar.get_referencers(path, opts)
    deps = ar.get_dependencies(path, opts)
    ad = ar.get_asset_by_object_path(f"{path}.{path.split('/')[-1]}")
    asset_class = None
    if ad and ad.is_valid():
        try:
            asset_class = str(ad.asset_class_path.asset_name)
        except Exception:
            asset_class = None
    return {
        "exists": exists,
        "loaded_class": cls,
        "asset_registry_class": asset_class,
        "referencers": sorted(str(x) for x in (refs or [])),
        "dependencies": sorted(str(x) for x in (deps or [])),
    }

result = {
    "generated_at": datetime.utcnow().isoformat() + "Z",
    "target": get_info(TARGET),
    "source": get_info(SOURCE),
}

os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(result, f, indent=2)

unreal.log("LEAG004_STATE_WRITTEN:" + OUT)

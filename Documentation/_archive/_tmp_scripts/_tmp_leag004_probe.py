import unreal, json, os
from datetime import datetime
OUT=r"s:\Programmation\Jeu\UE5\Byakuya\Leagues\Documentation\reports\leag004-probe-2026-02-22.json"
TARGETS=["/Game/Game/Core/GI_SpaceLeague","/Game/Instance/GI_SpaceLeague","/Game/Levels/SandboxMap/GM_Sandbox","/Game/UI/Tmp_HUD/UW_HUD","/Game/Maps/BPPC_Paradoxe","/Game/Instance/GI_Proxy"]
ar=unreal.AssetRegistryHelpers.get_asset_registry()
opts=unreal.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=True,include_searchable_names=False,include_soft_management_references=True,include_hard_management_references=True)
res={"generated_at":datetime.utcnow().isoformat()+"Z","assets":{}}
for p in TARGETS:
    entry={}
    try:
        obj=unreal.EditorAssetLibrary.load_asset(p)
        entry["load_asset_exists"] = obj is not None
        entry["loaded_class"] = obj.get_class().get_name() if obj else None
    except Exception as e:
        entry["load_asset_exists"] = False
        entry["load_error"] = str(e)
    try:
        ad=ar.get_asset_by_object_path(f"{p}.{p.split('/')[-1]}")
        entry["asset_registry_valid"] = bool(ad and ad.is_valid())
        if ad and ad.is_valid():
            try:
                entry["asset_registry_class"] = str(ad.asset_class_path.asset_name)
            except Exception:
                entry["asset_registry_class"] = None
        else:
            entry["asset_registry_class"] = None
    except Exception as e:
        entry["asset_registry_valid"] = False
        entry["asset_registry_error"] = str(e)
    refs=ar.get_referencers(p, opts) or []
    deps=ar.get_dependencies(p, opts) or []
    entry["referencers"] = sorted(str(x) for x in refs)
    entry["dependencies"] = sorted(str(x) for x in deps)
    res["assets"][p]=entry
os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT,'w',encoding='utf-8') as f: json.dump(res,f,indent=2)

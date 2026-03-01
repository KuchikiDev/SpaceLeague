import unreal, json, os
from datetime import datetime
OUT=r"s:\Programmation\Jeu\UE5\Byakuya\Leagues\Documentation\reports\leag004-probe-2026-02-22-after-user-fix.json"
TARGETS=["/Game/Game/Core/GI_SpaceLeague","/Game/Instance/GI_SpaceLeague","/Game/Levels/SandboxMap/GM_Sandbox","/Game/UI/Tmp_HUD/UW_HUD","/Game/Maps/BPPC_Paradoxe","/Game/Instance/GI_Proxy","/Game/Legends/BP_Paradoxe"]
ar=unreal.AssetRegistryHelpers.get_asset_registry()
opts=unreal.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=True,include_searchable_names=False,include_soft_management_references=True,include_hard_management_references=True)
res={"generated_at":datetime.utcnow().isoformat()+"Z","assets":{}}
for p in TARGETS:
    obj=unreal.EditorAssetLibrary.load_asset(p)
    refs=ar.get_referencers(p,opts) or []
    deps=ar.get_dependencies(p,opts) or []
    entry={"exists":obj is not None,"loaded_class":(obj.get_class().get_name() if obj else None),"referencers":sorted(str(x) for x in refs),"dependencies":sorted(str(x) for x in deps)}
    ad=ar.get_asset_by_object_path(f"{p}.{p.split('/')[-1]}")
    entry["asset_registry_valid"] = bool(ad and ad.is_valid())
    if ad and ad.is_valid():
        try: entry["asset_registry_class"] = str(ad.asset_class_path.asset_name)
        except: entry["asset_registry_class"] = None
    res["assets"][p]=entry
os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT,'w',encoding='utf-8') as f: json.dump(res,f,indent=2)

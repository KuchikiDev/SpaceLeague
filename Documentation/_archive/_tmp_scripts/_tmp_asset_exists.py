import unreal

paths = [
    "/Game/Legends/Blueprints/BP_ParadoxeJhin",
    "/Game/Legends/Blueprints/BP_ParadoxeSenna",
    "/Game/Game/Old/DataAsset/ST_CharacterUI",
]

for p in paths:
    exists = unreal.EditorAssetLibrary.does_asset_exist(p)
    unreal.log(f"ASSET_EXISTS:{p}:{exists}")

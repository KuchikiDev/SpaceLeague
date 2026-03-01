import unreal

targets = [
    "/Game/Instance/GI_SpaceLeague",
    "/Game/Game/Core/GI_SpaceLeague",
]

ar = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(
    include_hard_package_references=True,
    include_soft_package_references=True,
    include_searchable_names=False,
    include_soft_management_references=True,
    include_hard_management_references=True,
)

for p in targets:
    deps = ar.get_dependencies(p, opts)
    unreal.log(f"GI_DEPS:{p}:count={len(deps)}")
    for d in sorted(str(x) for x in deps):
        unreal.log(f"GI_DEP_ITEM:{p}:{d}")

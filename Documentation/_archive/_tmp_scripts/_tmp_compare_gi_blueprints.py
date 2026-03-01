import unreal

paths = [
    "/Game/Instance/GI_SpaceLeague",
    "/Game/Game/Core/GI_SpaceLeague",
]

for p in paths:
    bp = unreal.EditorAssetLibrary.load_asset(p)
    if not bp:
        unreal.log(f"GI_BP:{p}:load_failed")
        continue

    parent = None
    try:
        parent = bp.parent_class.get_name() if bp.parent_class else "None"
    except Exception:
        parent = "<unavailable>"

    vars_names = []
    try:
        for v in bp.new_variables:
            vars_names.append(v.var_name)
    except Exception:
        pass

    funcs = []
    try:
        for g in bp.function_graphs:
            funcs.append(g.get_name())
    except Exception:
        pass

    macros = []
    try:
        for g in bp.macro_graphs:
            macros.append(g.get_name())
    except Exception:
        pass

    unreal.log(f"GI_BP:{p}:parent={parent}:vars={len(vars_names)}:funcs={len(funcs)}:macros={len(macros)}")
    unreal.log(f"GI_BP_VARS:{p}:{','.join(str(x) for x in vars_names)}")
    unreal.log(f"GI_BP_FUNCS:{p}:{','.join(funcs)}")
    unreal.log(f"GI_BP_MACROS:{p}:{','.join(macros)}")

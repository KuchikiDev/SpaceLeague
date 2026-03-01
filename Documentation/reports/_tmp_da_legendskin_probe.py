import unreal
bp = unreal.EditorAssetLibrary.load_asset('/Game/Game/Data/Skins/DA_LegendSkin')
unreal.log(f'DA_LEGENDSKIN_PROBE:loaded={bp is not None}')
if bp:
    unreal.log(f'DA_LEGENDSKIN_PROBE:class={bp.get_class().get_name()}')
    try:
        vars_list = getattr(bp, 'new_variables', [])
        unreal.log(f'DA_LEGENDSKIN_PROBE:new_variables_count={len(vars_list)}')
        for v in vars_list:
            try:
                name = str(v.var_name)
            except Exception:
                name = '<unknown>'
            unreal.log(f'DA_LEGENDSKIN_VAR:{name}')
    except Exception as e:
        unreal.log(f'DA_LEGENDSKIN_PROBE:new_variables_error={e}')
    try:
        cls = unreal.EditorAssetLibrary.load_blueprint_class('/Game/Game/Data/Skins/DA_LegendSkin')
        unreal.log(f'DA_LEGENDSKIN_PROBE:generated_class={cls is not None}')
        if cls:
            props = []
            try:
                for p in cls.properties():
                    props.append(p.get_name())
            except Exception as e:
                unreal.log(f'DA_LEGENDSKIN_PROBE:properties_error={e}')
            for n in sorted(set(props)):
                if 'Override' in n or 'Skin' in n or 'Ability' in n:
                    unreal.log(f'DA_LEGENDSKIN_PROP:{n}')
    except Exception as e:
        unreal.log(f'DA_LEGENDSKIN_PROBE:generated_class_error={e}')

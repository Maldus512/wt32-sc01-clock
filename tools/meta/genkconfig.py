import os
import kconfiglib


def getKconfig(kconfig):
    if os.path.isfile('sdkconfig'):
        config = kconfiglib.Kconfig(kconfig)
        config.load_config('sdkconfig')
    else:
        print('Not configured!')
        exit(1)
    return config


def generate_sdkconfig_header(target, source, env):
    content = """
#ifndef SDKCONFIG_H_INCLUDED
#define SDKCONFIG_H_INCLUDED
// Automatically generated file. Do not edit.

    """
    for config in list(
            map(lambda x: getKconfig(str(x)),
                [kconf for kconf in source if 'Kconfig' in str(kconf)])):
        config.load_config('sdkconfig')

        for (key, sym) in [(x, config.syms[x]) for x in config.syms.keys()]:
            if sym.type == kconfiglib.BOOL and sym.str_value == 'y':
                content += "#define CONFIG_{} 1\n".format(key)
            elif sym.type == kconfiglib.INT:
                content += "#define CONFIG_{} {}\n".format(key, sym.str_value)
            elif sym.type == kconfiglib.STRING and len(sym.str_value) > 0:
                if key in ['LV_USER_DATA_FREE', 'LV_MEM_CUSTOM_FREE', 'LV_MEM_CUSTOM_ALLOC', 'LV_TICK_CUSTOM_SYS_TIME_EXPR']:
                    value = sym.str_value.replace('"', '')
                else:
                    value = f'"{sym.str_value}"'
                content += '#define CONFIG_{} {}\n'.format(key, value)

    content += "#endif"

    with open(str(target[0]), "w") as f:
        f.write(content)
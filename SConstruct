import kconfiglib
import os
import multiprocessing
from pathlib import Path
import tools.meta.csv2carray as csv2carray
from tools.meta.genkconfig import generate_sdkconfig_header


def PhonyTargets(
    target,
    action,
    depends,
    env=None,
):
    # Creates a Phony target
    if not env:
        env = DefaultEnvironment()
    t = env.Alias(target, depends, action)
    env.AlwaysBuild(t)


PROGRAM = "application"
MAIN = "main"
ASSETS = "assets"
SIMULATOR = "simulator"
COMPONENTS = "components"
FREERTOS = f"{SIMULATOR}/freertos-simulator"
CJSON = f"{SIMULATOR}/cJSON"
B64 = f"{SIMULATOR}/b64"
LVGL = f"{COMPONENTS}/lvgl"
DRIVERS = f"{SIMULATOR}/lv_drivers"
STRING_TRANSLATIONS = f"{MAIN}/view/intl"

CFLAGS = [
    "-Wall",
    "-Wextra",
    "-Wno-unused-function",
    "-g",
    "-O0",
    "-DSIMULATOR",
    "-DESP_PLATFORM",
    "-Desp_err_t=int",
    "-DLV_CONF_INCLUDE_SIMPLE",
    "-DLV_HOR_RES_MAX=480",
    "-DLV_VER_RES_MAX=320",
    '-DprojCOVERAGE_TEST=1',
    '-DGEL_PARAMETER_CONFIGURATION_HEADER="\\"gel_parameter_conf.h\\""',
    '-DGEL_PAGEMANAGER_CONFIGURATION_HEADER="\\"gel_pman_conf.h\\""',
    "-Wno-unused-parameter",
    "-static-libgcc",
    "-static-libstdc++",
]
LDLIBS = ["-lSDL2", "-lpthread", "-lm"]

CPPPATH = [
    COMPONENTS, f'#{SIMULATOR}/port', f'#{MAIN}',
    f"#{MAIN}/config", f"#{SIMULATOR}", B64, CJSON, f"#{LVGL}", f"#{DRIVERS}",
]

TRANSLATIONS = [
    {
        "generated_files": [f"{STRING_TRANSLATIONS}/AUTOGEN_FILE_strings.c", f"{STRING_TRANSLATIONS}/AUTOGEN_FILE_strings.h"],
        "input_folder": f"{ASSETS}/translations/strings",
        "output_folder": STRING_TRANSLATIONS
    },
]


def main():
    num_cpu = multiprocessing.cpu_count()
    SetOption('num_jobs', num_cpu)
    print("Running with -j {}".format(GetOption('num_jobs')))

    env_options = {
        "ENV": os.environ,
        "CC": ARGUMENTS.get('cc', 'gcc'),
        "ENV": os.environ,
        "CPPPATH": CPPPATH,
        'CPPDEFINES': [],
        "CCFLAGS": CFLAGS,
        "LIBS": LDLIBS,
    }

    env = Environment(**env_options)
    env.Tool('compilation_db')

    translations = []
    for translation in TRANSLATIONS:
        translations += csv2carray.create_scons_target(env, **translation)
    env.Alias("intl", translations)

    sdkconfig = env.Command(
        f"{SIMULATOR}/sdkconfig.h",
        [str(filename) for filename in Path(
            "components").rglob("Kconfig")] + ["sdkconfig"],
        generate_sdkconfig_header)

    freertos_env = env
    (freertos, include) = SConscript(
        f'{FREERTOS}/SConscript', exports=['freertos_env'])
    env['CPPPATH'] += [include]

    gel_env = env
    gel_selected = ["pagemanager", "collections",
                    "parameter", "timer", "data_structures"]
    (gel, include) = SConscript(
        f'{COMPONENTS}/generic_embedded_libs/SConscript', exports=['gel_env', 'gel_selected'])
    env['CPPPATH'] += [include]

    sources = Glob(f'{SIMULATOR}/*.c')
    sources += Glob(f'{SIMULATOR}/port/*.c')
    sources += [File(filename) for filename in Path('main/model').rglob('*.c')]
    sources += [File(filename)
                for filename in Path('main/config').rglob('*.c')]
    sources += [File(filename) for filename in Path('main/view').rglob('*.c')]
    sources += [File(filename)
                for filename in Path('main/controller').glob('*.c')]
    sources += [File(filename)
                for filename in Path('main/utils').rglob('*.c')]
    sources += [File(filename)
                for filename in Path(f'{LVGL}/src').rglob('*.c')]
    sources += [File(filename) for filename in Path(DRIVERS).rglob('*.c')]
    sources += [File(f'{CJSON}/cJSON.c')]
    sources += [File(f'{B64}/encode.c'),
                File(f'{B64}/decode.c'), File(f'{B64}/buffer.c')]

    prog = env.Program(PROGRAM, sdkconfig + sources +
                       freertos + gel)
    env.Depends(prog, translations)
    PhonyTargets("run", f"./{PROGRAM}", prog, env)
    compileDB = env.CompilationDatabase('build/compile_commands.json')
    env.Depends(prog, compileDB)


main()

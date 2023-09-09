import kconfiglib
import os
import multiprocessing
from pathlib import Path
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


PROGRAM = "app"
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
    "-g",
    "-O0",
    "-DSIMULATED_APPLICATION",
    "-DESP_PLATFORM",
    "-DLV_CONF_INCLUDE_SIMPLE",
    "-DLV_HOR_RES_MAX=480",
    "-DLV_VER_RES_MAX=320",
    '-DprojCOVERAGE_TEST=0',
    '-DIDF_VER="\\"SIMULATED\\""',
]
LDLIBS = ["-lSDL2", "-lpthread", "-lm"]

CPPPATH = [
    COMPONENTS, f'#{SIMULATOR}/port', f'#{MAIN}',
    f"#{MAIN}/config", f"#{SIMULATOR}", B64, CJSON, f"#{LVGL}", f"#{DRIVERS}", 
]


def main():
    num_cpu = multiprocessing.cpu_count()
    SetOption("num_jobs", num_cpu)
    print("Running with -j {}".format(GetOption('num_jobs')))

    env_options = {
        "ENV": os.environ,
        "CC": ARGUMENTS.get("cc", "gcc"),
        "ENV": os.environ,
        "CPPPATH": CPPPATH,
        'CPPDEFINES': [],
        "CCFLAGS": CFLAGS,
        "LIBS": LDLIBS,
    }

    env = Environment(**env_options)
    env.Tool('compilation_db')

    sdkconfig = env.Command(
        f"{SIMULATOR}/sdkconfig.h",
        [str(filename) for filename in Path(
            "components").rglob("Kconfig")] + ["sdkconfig"],
        generate_sdkconfig_header)

    freertos_env = env
    (freertos, include) = SConscript(
        f'{FREERTOS}/SConscript', exports=['freertos_env'])
    env['CPPPATH'] += [include]

    pman_env = env
    (pman, include) = SConscript(
        f'{COMPONENTS}/c-page-manager/SConscript', exports=['pman_env'])
    env['CPPPATH'] += [include]

    c_watcher_env = env
    (watcher, include) = SConscript(
        f'{COMPONENTS}/c-watcher/SConscript', exports=['c_watcher_env'])
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
                for filename in Path(f'{LVGL}/src').rglob('*.c')]
    sources += [File(filename) for filename in Path(DRIVERS).rglob('*.c')]
    sources += [File(f'{CJSON}/cJSON.c')]
    sources += [File(f'{B64}/encode.c'),
                File(f'{B64}/decode.c'), File(f'{B64}/buffer.c')]

    prog = env.Program(PROGRAM, sdkconfig + sources + freertos + pman + watcher)
    PhonyTargets("run", f"./{PROGRAM}", prog, env)
    compileDB = env.CompilationDatabase('build/compile_commands.json')
    env.Depends(prog, compileDB)


main()

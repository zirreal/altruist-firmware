Import("env")
import configparser
import hashlib
import os
import pathlib
import shutil
from base64 import b64decode

# Touch html-content.h so PlatformIO recompiles files that include it.
# This keeps __DATE__ fresh instead of stale from cached object files.
pathlib.Path(os.path.join(env.subst("$PROJECT_DIR"), "webserver", "html-content.h")).touch()

config = configparser.ConfigParser()
config.read("platformio.ini")

def _file_md5_hexdigest(fname):
    return hashlib.md5(open(fname, 'rb').read()).hexdigest()

def after_build(source, target, env):
    if not os.path.exists("builds"):
        os.mkdir("builds")

    configName = b64decode(ARGUMENTS.get("PIOENV")).decode('utf-8')
    sectionName = 'env:' + configName
    lang = config.get(sectionName, "lang")
    target_name = lang.lower()

    # Проверяем build_type в platformio.ini для текущего окружения
    is_dev = config.get(sectionName, "build_type", fallback="release") == "debug"
    dev_postfix = "_dev" if is_dev else ""
    # --------------------------------------

    if "esp32c3" in sectionName:
        print("Program has been built!", sectionName)
        firmaware_prefix_name = "latest32c3"
    elif "esp32c6_inside" in sectionName:
        print("Program has been built!", sectionName)
        firmaware_prefix_name = "latest32c6ins"
    elif "esp32c6_urban" in sectionName:
        print("Program has been built!", sectionName)
        firmaware_prefix_name = "latest32c6urb"
    else:
        firmaware_prefix_name = "latest"

    # Формируем финальное имя файла с учетом dev_postfix
    final_file_name = f"{firmaware_prefix_name}_{target_name}{dev_postfix}.bin"
    
    # Запись MD5
    with open(f"builds/{final_file_name}.md5", "w") as md5:
        print(_file_md5_hexdigest(target[0].path), file = md5)
    
    # Копирование BIN
    shutil.copy(target[0].path, f"builds/{final_file_name}")

    # Обработка специального случая для 'en' -> 'beta'
    if target_name == "en":
        beta_name = f"{firmaware_prefix_name}_beta{dev_postfix}.bin"
        with open(f"builds/{beta_name}.md5", "w") as md5:
            print(_file_md5_hexdigest(target[0].path), file = md5)
        shutil.copy(target[0].path, f"builds/{beta_name}")

env.AddPostAction("$BUILD_DIR/firmware.bin", after_build)

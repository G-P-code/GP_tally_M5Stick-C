# Copies the build output to release/ under versioned names:
#
#   GP_tally_M5StickC_v<ver>.bin       app only: web portal Update, OTA
#   GP_tally_M5StickC_v<ver>_full.bin  bootloader + partitions + app, flash at
#                                      0x0 on a new or erased device
#
# The version is FW_VERSION from include/config.h. The full image is the
# platform's firmware.factory.bin, made by a post action that runs before this one.

import re
import shutil
from pathlib import Path

Import("env")  # noqa: F821  (SCons)


def fw_version():
    text = Path(env.subst("$PROJECT_INCLUDE_DIR"), "config.h").read_text()
    m = re.search(r'#define\s+FW_VERSION\s+"([^"]+)"', text)
    if not m:
        raise SystemExit("release_bins.py: FW_VERSION not found in include/config.h")
    return m.group(1)


def copy_release(source, target, env):
    build = Path(env.subst("$BUILD_DIR"))
    out = Path(env.subst("$PROJECT_DIR"), "release")
    out.mkdir(exist_ok=True)
    base = "GP_tally_M5StickC_v" + fw_version()
    for src, name in ((build / "firmware.bin", base + ".bin"),
                      (build / "firmware.factory.bin", base + "_full.bin")):
        if not src.is_file():
            raise SystemExit("release_bins.py: missing " + str(src))
        shutil.copyfile(src, out / name)
        print("Release image: release/" + name)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_release)  # noqa: F821

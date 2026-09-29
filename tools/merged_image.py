# PlatformIO extra script: adds a `merged` target that builds a single
# firmware image to be written starting at address 0x0, which is the form
# M5Burner expects.
#
#     pio run -e sticks3 -t merged
#
# `pio run -t upload` writes four files, each at its own address: the
# bootloader at 0x0, the partition table at 0x8000, boot_app0 at 0xe000 and
# the application at 0x10000. M5Burner writes a single file starting at 0x0,
# so a bare firmware.bin would land on top of the bootloader. This target
# merges the same four files into one image with esptool merge_bin.
#
# Addresses, paths and flash parameters come from the upload settings that
# PlatformIO already has, so the image is exactly what `upload` would have
# written.

import os

Import("env")  # noqa: F821 (injected by SCons)


def _upload_flag(name):
    """The value that follows `name` in UPLOADERFLAGS, e.g. --flash_mode dio."""
    flags = env["UPLOADERFLAGS"]
    return env.subst(flags[flags.index(name) + 1])


def build_merged(source, target, env):
    app = env.subst("$BUILD_DIR/${PROGNAME}.bin")
    output = env.subst("$BUILD_DIR/${PROGNAME}-merged.bin")

    images = []
    for offset, path in env["FLASH_EXTRA_IMAGES"]:
        images += [offset, env.subst(path)]
    images += [env.subst("$ESP32_APP_OFFSET"), app]

    command = [
        env.subst("$PYTHONEXE"),
        env.subst("$UPLOADER"),
        "--chip", env.BoardConfig().get("build.mcu"),
        "merge_bin",
        "-o", output,
        "--flash_mode", _upload_flag("--flash_mode"),
        "--flash_freq", _upload_flag("--flash_freq"),
        "--flash_size", _upload_flag("--flash_size"),
    ] + images

    result = env.Execute(" ".join('"%s"' % part for part in command))
    if result == 0:
        print("Merged image for M5Burner (flash at 0x0): %s, %d bytes"
              % (output, os.path.getsize(output)))
    return result


env.AddCustomTarget(  # noqa: F821
    name="merged",
    dependencies="$BUILD_DIR/${PROGNAME}.bin",
    actions=build_merged,
    title="Merged image",
    description="Build a single image flashed at 0x0, for M5Burner",
)

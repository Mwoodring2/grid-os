Import("env")
from pathlib import Path
import subprocess

def merge(source, target, env):
    environment = env
    output = Path(environment.subst("$PROJECT_DIR")) / "dist"
    output.mkdir(exist_ok=True)
    build = Path(environment.subst("$BUILD_DIR"))
    tool = Path(environment.PioPlatform().get_package_dir("tool-esptoolpy")) / "esptool.py"
    command = [environment.subst("$PYTHONEXE"), str(tool), "--chip", "esp32s3", "merge_bin",
               "-o", str(output / "GRID-OS-ES3C28P-v0.1.0-alpha.bin"),
               "--flash_mode", "dio", "--flash_freq", "80m", "--flash_size", "16MB"]
    for address, filename in environment.get("FLASH_EXTRA_IMAGES", []):
        # Arduino boot_app0 initializes ota_0, which would bypass our factory
        # launcher. An erased OTA data region selects factory on first boot.
        if int(str(address), 0) == 0xe000:
            continue
        command.extend([str(address), environment.subst(filename)])
    ota_data = output / "ota-data-factory.bin"
    ota_data.write_bytes(b"\xff" * 8192)
    command.extend(["0xe000", str(ota_data)])
    command.extend(["0x10000", str(build / "firmware.bin")])
    subprocess.run(command, check=True)
    (output / "GRID-OS-application.bin").write_bytes((build / "firmware.bin").read_bytes())
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge)

Import("env")
from pathlib import Path

def copy_payload(source, target, env):
    output = Path(env.subst("$PROJECT_DIR")) / "dist"
    output.mkdir(exist_ok=True)
    firmware = Path(env.subst("$BUILD_DIR")) / "firmware.bin"
    (output / "grid-return-test.bin").write_bytes(firmware.read_bytes())
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_payload)

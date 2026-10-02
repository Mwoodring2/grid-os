# GRID//OS v0.1.0-alpha validation

Target: Hosyond ES3C28P ESP32-S3, 16 MiB flash, OPI PSRAM, landscape 320×240.

- PlatformIO release build: PASS (clean build).
- Static RAM: 50,368 / 327,680 bytes (15.4%). Does not include runtime heap/sprite allocations.
- Application program size report: 1,089,742 bytes. PlatformIO reports against the 6 MiB OTA slot; the release checker separately enforces the 3 MiB factory limit.
- Merged factory image: 1,155,680 bytes.
- Host image-policy checks: PASS, 0 failures. Covers chip ID, segment bounds, short input, oversize image and application descriptor classification.
- Python build/release script syntax: PASS.
- Release verification: PASS. Bootloader/app chip ID 9 (ESP32-S3), erased OTA data, exact application bytes at factory offset, expected factory/payload partitions, 16 MiB bounds and no overlap.
- SHA256: `67c953788105b01c6d0a3cf67fcc87137b6fd3b76523d4821a7472c408d671fd`

Warnings: TFT_eSPI's TOUCH_CS warning concerns its unused resistive-touch API; this build uses the separate uploaded FT6336G capacitive driver. esptool warns about deprecated merge option spellings; generation and layout verification succeed.

Not measured: physical display/touch/SD behavior, brightness polarity, live free heap, Wi-Fi connection, real flash installation, power-interruption behavior and cooperative app return. No hardware acceptance claimed.

Universal bootloader-based recovery, metadata/icons, on-screen keyboard, online app catalog and startup countdown remain unimplemented. The current return helper requires a compatible payload; unmodified payloads require USB recovery. See README before installing one.

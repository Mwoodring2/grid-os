# GRID//OS v0.2.0-alpha
Cyberpunk firmware launcher foundation for **Hosyond ES3C28P 2.8-inch ESP32-S3 N16R8**. Landscape 320×240, ILI9341V, FT6336G, 1-bit SD_MMC. Separate project from ESP-Goblin. GPL-3.0-or-later.

## Phase 2: cooperative switching
The APPS list now includes `[FLASH]` when the payload slot contains an application image. Discovery reads that image directly, including after a return to the launcher and with the SD card removed. It does not use an NVS installed flag or the current boot selection. Tap `[FLASH]` and BOOT APP to reopen the payload; that action selects the boot partition and does not rewrite the image. Returning clears only the OTA boot record and leaves the payload bytes in place. Serial output names the running and selected partitions, the factory and payload geometry, the descriptor result, and the reason a payload was rejected. Complete image validation occurs when selecting the payload boot target.

Build output `dist/grid-return-test.bin` is an application-only test payload. Copy it into `/apps` on SD, install it through GRID//OS, then tap RETURN TO GRID//OS. The test app also accepts USB command `return`, or a two-second BOOT hold **while the app is already running**. These are cooperative app features, not universal bootloader recovery.

Return and launch require the exact shared partition layout and report selection/verification failures without rebooting. Returning does not erase NVS or rewrite the launcher. Hold neither BOOT nor RESET during the in-app return test.

USB commands `app installed` and `app boot` inspect/launch the payload. See `docs/PHASE2_ACCEPTANCE.md` for the test sequence.

## Implemented
- SYS: real CPU frequency, heap, PSRAM, flash, uptime and SD state.
- APPS: `/apps/*.bin` discovery, paging, details, install confirmation and progress.
- FILES: SD directory navigation, file sizes and paging (64-entry cap per directory).
- NET: connection, SSID, IP and RSSI. Connect via USB command; credentials not logged or explicitly saved by GRID//OS.
- TERM: bounded USB serial command input at 115200 baud. This is a USB terminal, not an on-screen keyboard.
- CFG: persistent brightness and cyan/green/amber themes, SD retry.
- Installer rejects wrong-chip, oversized and bootloader/merged images. ESP-IDF validates application integrity before boot selection. Header checks alone are not signature or trust verification.
- `grid_return.h`: cooperative payload return helper.

## Build on Windows
Install Python and run from this folder:
```
py -m pip install platformio
py -m platformio run -e grid_es3c28p -e grid_return_test
py scripts/verify_release.py
py scripts/run_host_tests.py
```
Outputs after a successful build:
- `dist/GRID-OS-ES3C28P-v0.2.0-alpha.bin`: merged factory image, USB flash at **0x0000**.
- `dist/GRID-OS-application.bin`: application image only. Not the initial-install image.

**Use the merged image at 0x0000 for initial flashing. Do not use PlatformIO `upload`: its default app offset follows ota_0 rather than our factory launcher.**

Back up needed app settings before first install: GRID//OS introduces a different partition table. Initial installation replaces the current flash layout. Only flash to ES3C28P ESP32-S3 16 MB; not the 4-inch ESP32 board.

## SD library
Copy application-only binaries into `/apps/`. Use `.pio/build/<environment>/firmware.bin` from a compatible app build, not a merged web-flasher/factory binary. The ESP-Goblin merged binary previously flashed at 0x0000 cannot be used directly.

## Recovery and firmware compatibility — READ BEFORE INSTALLING APPS
This alpha preserves the launcher at 0x10000 in a 3 MiB factory partition and installs one payload in a 6 MiB ota_0 partition. Installation erases/replaces the previous payload. The SD card is the library, not executable storage.

**Universal hardware return is NOT implemented.** With an unmodified payload, reboot continues booting that payload. Holding BOOT during reset enters the ESP32-S3 ROM downloader; it does not open GRID//OS. To return, use USB to flash the merged GRID//OS image again (or an advanced OTA-data reset procedure). Install confirmation states this limitation.

A payload adapted to this partition layout can copy **both** `include/grid_return.h` and `include/grid_boot.h` into its include folder, then include `grid_return.h` and call `gridReturnToLauncher()` from its menu. It must not erase/replace the partition table or factory region. Rebuilding payloads against `partitions.csv` is recommended. Sharing the NVS partition may affect app settings. Apps expecting their own storage partitions, OTA scheme or flash offsets need adaptation even when their chip header passes.

A custom bootloader with a dedicated recovery input is the next required step for M5Launcher-like universal return. No such bootloader or hardware acceptance is claimed here. Image integrity checks do not prove board/pin compatibility or protect against a payload that deliberately writes arbitrary flash.

## USB commands
```
help
sysinfo
sd ls
app list
wifi status
wifi connect YOUR_SSID|YOUR_PASSWORD
brightness 80
theme cyan
reboot
clear
```
No auto-launch countdown, metadata/icons, app download catalog, on-screen terminal keyboard, SD theme files, temperature or battery percentage in this alpha. Those require further work or calibrated sensors; fake readings are not displayed.

## Hardware acceptance
1. Verify landscape display, all six tabs and accurate touch targeting.
2. Verify SD absent boot and CFG retry, directory paging, brightness and theme across reboot.
3. Verify serial console and Wi-Fi connection.
4. Reject merged and wrong-chip images; verify malformed-image rejection leaves launcher bootable.
5. Build a small cooperative test payload using this partition table, install it, and test the return helper.
6. Test power interruption during writing: factory remains, but the old payload is not preserved.
7. Only then try adapted ESP-Goblin. Monitor heap, SD I/O and touch responsiveness.

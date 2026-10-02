# Phase 2 — cooperative install, return and reopen

Target and partition layout are unchanged: Hosyond ES3C28P ESP32-S3 N16R8. Visual polish remains deferred.

1. Build both environments using build.ps1, or the commands in README. Use the v0.2 merged image at USB offset 0x0000. Do not use PlatformIO upload.
2. Copy `dist/grid-return-test.bin` into `/apps/grid-return-test.bin` on SD. It is an application image; never flash this file at 0x0000.
3. Open APPS, select the SD test app, then INSTALL > WRITE + BOOT. Record any installation error. The previous payload will be replaced.
4. Confirm GRID//RETURN TEST appears. Tap RETURN TO GRID//OS. Confirm the launcher appears without USB reflashing.
5. Open APPS. Select the `[FLASH]` entry, then BOOT APP. Confirm the test app launches without the install progress screen.
6. Return. Power off before removing SD, then power on. Check APPS still lists the installed image; reopen it without SD. Return from the app without SD.
7. In the running test app, hold BOOT for two seconds and confirm return. Release BOOT before resetting/powering on. Holding it through reset invokes the ROM downloader.
8. Reopen and send `return` over USB serial at 115200 baud. Confirm return.
9. Repeat three cycles. Observe responsive touch and no repeated automatic return on a failed selection.

An invalid image must fail selection and remain on the current screen; the launcher and test app display the SDK error. Host tests cover missing/misplaced partitions, unexpected running slots, failed selection and avoiding a reboot loop. They do not emulate actual flash verification or prove physical switching.

Power-interruption testing, ESP-Goblin integration and universal return from unmodified apps remain pending. No bootloader recovery changes are included. Successful build/host tests do not constitute hardware acceptance.

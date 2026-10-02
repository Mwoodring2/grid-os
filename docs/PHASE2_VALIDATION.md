# Phase 2 validation — v0.2.0-alpha

Baseline: `0e1edf62fb1442a22d0711fd05115a532aae075c`.

- Both PlatformIO environments built successfully with pinned platform 55.03.39 and TFT_eSPI 2.5.43.
- Launcher static RAM: 50,392 / 327,680 bytes (15.4%). Program size report: 1,093,614 bytes; actual application file: 1,094,016 bytes.
- Merged launcher image: 1,159,552 bytes. SHA256: `748844e67561b0a018ec25cd4320665642b2fa72262a364721d3958c6d17665f`.
- Return-test static RAM: 24,428 / 327,680 bytes (7.5%). Program size report: 406,030 bytes; actual application file: 406,432 bytes.
- Return-test SHA256: `c6df92c78150892ffb121accadb79ecea7b6319ff29a409df5b02dc0370634db`.
- Release layout verification: PASS. Factory application matches build, OTA data selects factory initially, exact shared factory/payload slots, S3 headers, application descriptors and size limits.
- Host image policy and boot-manager/input-gate suites: PASS, 0 failures. Boot APIs are mocked in the boot-manager host test; actual SDK image validation and reboot behavior remain hardware checks.
- Python script syntax and git diff --check: PASS.

Existing TFT_eSPI TOUCH_CS and deprecated esptool merge spelling warnings remain. FT6336G uses the separate capacitive-touch driver.

Physical install, return/reopen, touch carryover, SD-absent boot, long-press return, USB return and power-loss behavior have not been tested here. Run PHASE2_ACCEPTANCE.md. Universal recovery from unmodified apps and ESP-Goblin integration remain pending.

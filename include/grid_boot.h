#pragma once
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <cstddef>
#include <cstdint>
#if defined(ARDUINO)
#include <Arduino.h>
#endif

namespace grid {
// Validate the exact shared layout before changing a boot target. Payloads
// using a different partition table must not accidentally launch a factory app.
inline const char* layoutError() {
    const esp_partition_t* factory = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, "factory");
    if (!factory) return "factory partition missing";
    if (factory->address != 0x10000 || factory->size != 0x300000)
        return "factory partition geometry mismatch";
    const esp_partition_t* payload = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, "ota_0");
    if (!payload) return "payload partition missing";
    if (payload->address != 0x310000 || payload->size != 0x600000)
        return "payload partition geometry mismatch";
    return nullptr;
}
inline const esp_partition_t* launcherPartition() {
    const esp_partition_t* partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, "factory");
    return partition && partition->address == 0x10000 && partition->size == 0x300000 ? partition : nullptr;
}
inline const esp_partition_t* payloadPartition() {
    const esp_partition_t* partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, "ota_0");
    return partition && partition->address == 0x310000 && partition->size == 0x600000 ? partition : nullptr;
}
inline const esp_partition_t* otaDataPartition() {
    const esp_partition_t* partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, "otadata");
    return partition && partition->address == 0xe000 && partition->size == 0x2000 ? partition : nullptr;
}
inline bool layoutCompatible() { return layoutError() == nullptr; }
inline bool sameBytes(const uint8_t* left, const uint8_t* right, size_t count) {
    if (!left || !right) return false;
    for (size_t index = 0; index < count; index++) {
        if (left[index] != right[index]) return false;
    }
    return true;
}
// Switch to the factory launcher by erasing only the 8 KiB otadata partition.
// ESP-IDF's esp_ota_set_boot_partition(factory) does that erase internally and,
// on other targets, can erase an application partition. This path pins the
// erase to otadata and refuses to restart if the payload header changes.
inline esp_err_t selectFactoryKeepPayload() {
    const esp_partition_t* launcher = launcherPartition();
    const esp_partition_t* payload = payloadPartition();
    if (!launcher || !payload) return ESP_ERR_NOT_FOUND;
    esp_app_desc_t launcher_desc{};
    const esp_err_t described = esp_ota_get_partition_description(launcher, &launcher_desc);
    if (described != ESP_OK) return described;
    uint8_t before[4] = {};
    uint8_t after[4] = {};
    const esp_err_t before_read = esp_partition_read(payload, 0, before, sizeof(before));
    if (before_read != ESP_OK) return before_read;
    const esp_partition_t* ota_data = otaDataPartition();
    if (!ota_data) return ESP_ERR_NOT_FOUND;
    const esp_err_t erased = esp_partition_erase_range(ota_data, 0, ota_data->size);
    if (erased != ESP_OK) return erased;
    const esp_err_t after_read = esp_partition_read(payload, 0, after, sizeof(after));
    if (after_read != ESP_OK) return after_read;
    if (!sameBytes(before, after, sizeof(before))) return ESP_ERR_INVALID_STATE;
#if defined(ARDUINO)
    Serial.printf("[grid] factory selected by clearing otadata 0x%08lx size 0x%lx; payload header %02x %02x %02x %02x unchanged\n",
        static_cast<unsigned long>(ota_data->address), static_cast<unsigned long>(ota_data->size),
        before[0], before[1], before[2], before[3]);
#endif
    return ESP_OK;
}
inline esp_err_t selectPartition(const esp_partition_t* target) {
    if (!layoutCompatible() || !target) return ESP_ERR_NOT_FOUND;
    const esp_partition_t* running = esp_ota_get_running_partition();
    if (!running || running->address == target->address) return ESP_ERR_INVALID_STATE;
    if (running->address != launcherPartition()->address &&
        running->address != payloadPartition()->address) return ESP_ERR_INVALID_STATE;
    if (target->address == launcherPartition()->address) return selectFactoryKeepPayload();
    if (target->address != payloadPartition()->address) return ESP_ERR_NOT_FOUND;
    // ESP-IDF verifies the payload image, then updates OTA boot selection.
    // It does not rewrite the payload. Return the error and do not restart.
    return esp_ota_set_boot_partition(target);
}
inline esp_err_t selectLauncher() { return selectPartition(launcherPartition()); }
inline esp_err_t selectPayload() { return selectPartition(payloadPartition()); }
}

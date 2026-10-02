#pragma once
#include <esp_ota_ops.h>
#include <esp_system.h>

namespace grid {
// Validate the exact shared layout before changing a boot target. Payloads
// using a different partition table must not accidentally launch a factory app.
inline const esp_partition_t* launcherPartition() {
    const esp_partition_t* p = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, "factory");
    return p && p->address == 0x10000 && p->size == 0x300000 ? p : nullptr;
}
inline const esp_partition_t* payloadPartition() {
    const esp_partition_t* p = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, "ota_0");
    return p && p->address == 0x310000 && p->size == 0x600000 ? p : nullptr;
}
inline bool layoutCompatible() { return launcherPartition() && payloadPartition(); }
inline esp_err_t selectPartition(const esp_partition_t* target) {
    if (!layoutCompatible() || !target) return ESP_ERR_NOT_FOUND;
    const esp_partition_t* running = esp_ota_get_running_partition();
    if (!running || running->address == target->address) return ESP_ERR_INVALID_STATE;
    if (running->address != launcherPartition()->address &&
        running->address != payloadPartition()->address) return ESP_ERR_INVALID_STATE;
    // ESP-IDF verifies the complete image before updating OTA boot selection.
    // Return the error to the UI. Never restart after a failed selection.
    return esp_ota_set_boot_partition(target);
}
inline esp_err_t selectLauncher() { return selectPartition(launcherPartition()); }
inline esp_err_t selectPayload() { return selectPartition(payloadPartition()); }
}

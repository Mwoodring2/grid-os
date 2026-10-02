#pragma once
#include "grid_boot.h"
// Compatible payloads call this from a return action; caller displays errors.
// Does not alter factory firmware, erase NVS, or require an SD card.
inline esp_err_t gridReturnToLauncher() {
    esp_err_t result = grid::selectLauncher();
    if (result == ESP_OK) esp_restart();
    return result;
}

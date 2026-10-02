#pragma once
#include <cstdint>
using esp_err_t=int;
constexpr int ESP_OK=0,ESP_ERR_NOT_FOUND=1,ESP_ERR_INVALID_STATE=2;
constexpr int ESP_PARTITION_TYPE_APP=0,ESP_PARTITION_SUBTYPE_APP_FACTORY=0,ESP_PARTITION_SUBTYPE_APP_OTA_0=16;
struct esp_partition_t { uint32_t address,size; };
const esp_partition_t* esp_partition_find_first(int,int,const char*);
const esp_partition_t* esp_ota_get_running_partition();
esp_err_t esp_ota_set_boot_partition(const esp_partition_t*);

#pragma once
#include <esp_ota_ops.h>
#include <esp_system.h>
// Integrate into a compatible payload's menu. This requires GRID//OS's
// partition table and a preserved factory launcher. Does not use GPIO0.
inline esp_err_t gridReturnToLauncher() {
 const esp_partition_t* p=esp_partition_find_first(ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_FACTORY,"factory");
 if(!p)return ESP_ERR_NOT_FOUND;
 esp_err_t result=esp_ota_set_boot_partition(p);
 if(result==ESP_OK)esp_restart();
 return result;
}

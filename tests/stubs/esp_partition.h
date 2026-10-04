#pragma once
#include <cstddef>
#include <cstdint>

esp_err_t esp_partition_read(const esp_partition_t* partition, size_t src_offset, void* dst, size_t size);
esp_err_t esp_partition_erase_range(const esp_partition_t* partition, size_t offset, size_t size);

#pragma once
#include <cstddef>
#include <cstdint>
namespace grid {
inline const char* imageHeaderError(const uint8_t* h, size_t count, size_t size, size_t capacity) {
    if (!h || count < 24 || size < 288) return "Image too short";
    if (size > capacity) return "Image exceeds app slot";
    if (h[0] != 0xe9) return "Not an ESP application image";
    if (h[1] == 0 || h[1] > 16) return "Invalid segment count";
    if (h[12] != 9 || h[13] != 0) return "Requires ESP32-S3 image";
    // App descriptor magic at offset 32 is checked separately. Factory/merged
    // images also start with e9 but contain a bootloader descriptor instead.
    return nullptr;
}
inline bool appDescriptorMagic(const uint8_t* bytes) {
    return bytes && bytes[0] == 0x32 && bytes[1] == 0x54 && bytes[2] == 0xcd && bytes[3] == 0xab;
}
}

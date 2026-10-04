#pragma once
#include "image_policy.h"
#include <cstddef>
#include <cstdint>

namespace grid {
// Result of reading an installed payload image. Discovery uses the bytes in the
// payload slot. It does not consult OTA boot selection or SD-card state.
struct InstalledApp {
    bool present;
    const char* reason;
    char project[33];
    char version[33];
};

inline void copyDescriptorText(char* destination, const uint8_t* source) {
    size_t index = 0;
    if (!destination) return;
    destination[0] = 0;
    if (!source) return;
    for (; index < 32; index++) {
        const char character = static_cast<char>(source[index]);
        if (character == 0) break;
        destination[index] = character;
    }
    destination[index] = 0;
}

// Inspect the start of a payload image. `bytes` must cover the image header and
// application descriptor (112 bytes). `slot_size` is the partition capacity.
// A missing image, bad header, and bad descriptor each return a distinct reason.
inline InstalledApp inspectPayloadImage(const uint8_t* image, size_t bytes, size_t slot_size) {
    InstalledApp app{};
    app.reason = "payload header unreadable";
    if (!image || bytes < 112) return app;
    if (image[0] == 0xff || image[0] == 0x00) {
        app.reason = "payload image missing";
        return app;
    }
    // The stored image length is not known from the header alone. 288 is the
    // policy minimum; the bootloader and boot selection verify the full body.
    const char* header_error = imageHeaderError(image, 24, 288, slot_size);
    if (header_error) {
        app.reason = header_error;
        return app;
    }
    if (!appDescriptorMagic(image + 32)) {
        app.reason = "descriptor magic mismatch";
        return app;
    }
    copyDescriptorText(app.version, image + 48);
    copyDescriptorText(app.project, image + 80);
    if (app.project[0] == 0) {
        app.project[0] = 'P';
        app.project[1] = 'A';
        app.project[2] = 'Y';
        app.project[3] = 'L';
        app.project[4] = 'O';
        app.project[5] = 'A';
        app.project[6] = 'D';
        app.project[7] = 0;
    }
    app.present = true;
    app.reason = nullptr;
    return app;
}
}

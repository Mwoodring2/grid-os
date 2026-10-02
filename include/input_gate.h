#pragma once
namespace grid {
// Require release after startup, so the install-confirmation touch cannot
// carry through a reboot and immediately activate the payload's return button.
class PressGate {
    bool armed_ = false;
    bool held_ = false;
public:
    bool rising(bool pressed) {
        if (!pressed) { armed_ = true; held_ = false; return false; }
        const bool edge = armed_ && !held_;
        held_ = true;
        return edge;
    }
};
}

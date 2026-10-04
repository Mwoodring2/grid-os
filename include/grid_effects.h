#pragma once
#include <stdint.h>

namespace gridui {
// Unsigned elapsed arithmetic keeps short effects correct across millis wrap.
// Polling never sleeps and emits at most one frame every 40 ms.
class Effects {
public:
    enum class Kind { None, Boot, Touch };
    void enable(bool value) { enabled_=value; if(!value)cancel(); }
    bool enabled() const { return enabled_; }
    void cancel() { kind_=Kind::None; first_=false; }
    void boot(uint32_t now) { start(Kind::Boot,now); }
    void touch(uint32_t now) { start(Kind::Touch,now); }
    bool frame(uint32_t now, int& width) {
        if(kind_==Kind::None)return false;
        const uint32_t elapsed=now-start_;
        const uint32_t duration=kind_==Kind::Boot?480:160;
        if(elapsed>=duration){width=78;cancel();return true;}
        if(!first_ && uint32_t(now-last_)<40)return false;
        first_=false;last_=now;
        if(kind_==Kind::Boot)width=int(elapsed*78/duration);
        else width=elapsed<80?78:int(78-(elapsed-80)*39/80);
        return true;
    }
private:
    void start(Kind kind,uint32_t now) {
        if(!enabled_)return;
        kind_=kind;start_=last_=now;first_=true;
    }
    bool enabled_=true,first_=false;
    Kind kind_=Kind::None;
    uint32_t start_=0,last_=0;
};
}

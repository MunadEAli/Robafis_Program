#include "desk_io.h"
#include "config.h"

static_assert(cfg::BT_SERIAL_PORT == 2 || cfg::BT_SERIAL_PORT == 3, "Bluetooth is on Serial2 or Serial3");

namespace desk {
namespace {

HardwareSerial& BT = (cfg::BT_SERIAL_PORT == 2) ? Serial2 : Serial3;

constexpr uint16_t CHATTER_SHOW_EVERY_MS = 5000;

class BothLinks : public Print {
public:
    size_t write(uint8_t c) override {
        Serial.write(c);
        BT.write(c);
        return 1;
    }
};
BothLinks both;

struct Link {
    HardwareSerial* port;
    const char*     name;
    uint32_t        lastByteMs;
    bool            atLineStart;
    char            raw[48];
    uint8_t         rawLen;
    uint16_t        hidden;       // chatter lines not shown since the last one shown
    uint32_t        lastShownMs;
};
Link links[2] = {
    {&Serial, "USB", 0, true, {0}, 0, 0, 0},
    {&BT,     "BT",  0, true, {0}, 0, 0, 0},
};
const Link* current = &links[0];   // link of the command being handled

// Shows collected non-command text on USB only.
void flushRaw(Link& l, uint32_t now) {
    if (l.rawLen == 0) return;
    const bool isBt = (&l == &links[1]);
    if (isBt && now - l.lastShownMs < CHATTER_SHOW_EVERY_MS && l.lastShownMs != 0) {
        ++l.hidden;                 // the module repeating itself: count it, show it later
        l.rawLen = 0;
        return;
    }
    Serial.print(F("# "));
    Serial.print(l.name);
    Serial.print(isBt ? F(" says: \"") : F(" not a command (h = help): \""));
    Serial.write(reinterpret_cast<const uint8_t*>(l.raw), l.rawLen);
    Serial.print('"');
    if (l.hidden > 0) {
        Serial.print(F("  (+"));
        Serial.print(l.hidden);
        Serial.print(F(" similar lines hidden)"));
    }
    Serial.println();
    l.hidden      = 0;
    l.lastShownMs = now;
    l.rawLen      = 0;
}

void receive(Link& l, char c, uint32_t now, Handler handler) {
    if (now - l.lastByteMs > cfg::CMD_GAP_MS) {   // silence before this byte: a new "line"
        flushRaw(l, now);
        l.atLineStart = true;
    }
    l.lastByteMs = now;

    if (c == '\r' || c == '\n') {
        flushRaw(l, now);
        l.atLineStart = true;
        return;
    }
    if (l.atLineStart) {
        l.atLineStart = false;
        current = &l;
        if (handler(c, now)) return;
    }
    if (l.rawLen == sizeof(l.raw)) flushRaw(l, now);
    l.raw[l.rawLen++] = (c >= 32 && c < 127) ? c : '.';
}

}  // namespace

void begin() {
    Serial.begin(cfg::SERIAL_BAUD);
    BT.begin(cfg::BT_BAUD);
}

Print& out() { return both; }

void poll(uint32_t now, Handler handler) {
    for (Link& l : links) {
        while (l.port->available() > 0) receive(l, static_cast<char>(l.port->read()), now, handler);
        if (l.rawLen > 0 && now - l.lastByteMs > cfg::CMD_GAP_MS) flushRaw(l, now);
    }
}

void endReply(uint32_t now) {
    both.print(F("  (from "));
    both.print(current->name);
    both.print(F(", t="));
    both.print(now);
    both.println(F(" ms)"));
}

}  // namespace desk

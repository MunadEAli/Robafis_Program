// Desk test 1 (plan step 3): Serial + Bluetooth on the real MegaPi.
//
//   Upload:  pio run -e hw_link -t upload
//   Monitor: pio device monitor -e hw_link        (letters are sent as you type, no Enter needed)
//
// Twice a second it sends a heartbeat in the plan's format, HB,<t s>,-,<RUN|STOP>,<slowest loop ms>,
// to USB and to Bluetooth (cfg::BT_SERIAL_PORT).
//
// Single-letter commands, from the USB monitor or the phone:
//   v version   s stop   r resume   p ping   q heartbeat on/off   h help
// A letter counts as a command only at the start of a line or after a short silence (cfg::CMD_GAP_MS),
// i.e. when a person typed it. Every reply goes to both links and says where the command came from
// and when (board time in ms).
//
// Anything else that arrives (for example the module's own "ERROR01" answers while no phone is
// connected) is shown on USB only, one line at a time, and never sent back to Bluetooth: echoing it
// would make the module answer again, forever.
//
// No motors are driven here; "stop" only changes the state shown in the heartbeat and on the LED
// (blinking = RUN, steady = STOP).

#include <Arduino.h>
#include "config.h"

static_assert(cfg::BT_SERIAL_PORT == 2 || cfg::BT_SERIAL_PORT == 3, "Bluetooth is on Serial2 or Serial3");

namespace {

HardwareSerial& BT = (cfg::BT_SERIAL_PORT == 2) ? Serial2 : Serial3;

// Anything printed to `out` goes to USB and to Bluetooth.
class BothLinks : public Print {
public:
    size_t write(uint8_t c) override {
        Serial.write(c);
        BT.write(c);
        return 1;
    }
};
BothLinks out;

// Per-link receive state: is the next byte at a line start, and the text that was not a command.
struct Link {
    HardwareSerial* port;
    const char*     name;          // short, fixed: "USB" or "BT"
    uint32_t        lastByteMs;
    bool            atLineStart;
    char            raw[48];
    uint8_t         rawLen;
};
Link links[2] = {
    {&Serial, "USB", 0, true, {0}, 0},
    {&BT,     "BT",  0, true, {0}, 0},
};

bool     running     = true;
bool     heartbeatOn = true;
uint8_t  hbCount     = 0;
uint32_t lastHbMs    = 0;
uint32_t lastLoopUs  = 0;
uint32_t loopMaxUs   = 0;

// Ends every reply with "  (from BT, t=12345 ms)".
void endReply(const Link& from, uint32_t now) {
    out.print(F("  (from "));
    out.print(from.name);
    out.print(F(", t="));
    out.print(now);
    out.println(F(" ms)"));
}

void printHelp() {
    out.println(F("Commands: v version, s stop, r resume, p ping, q heartbeat on/off, h help"));
    out.print(F("Desk test passes when both links answer and a STOP from the phone comes back within "));
    out.print(cfg::STOP_LATENCY_MAX_MS);
    out.println(F(" ms."));
}

// Returns true if c is a command and was handled.
bool handleCommand(char c, const Link& from, uint32_t now) {
    switch (c) {
        case 'v':
            out.print(F("VER," CFG_FW_VERSION ",hw_link," __DATE__ " " __TIME__));
            endReply(from, now);
            return true;
        case 's':
            running = false;
            out.print(F("STATE,STOP"));
            endReply(from, now);
            return true;
        case 'r':
            running = true;
            out.print(F("STATE,RUN"));
            endReply(from, now);
            return true;
        case 'p':
            out.print(F("PONG"));
            endReply(from, now);
            return true;
        case 'q':
            heartbeatOn = !heartbeatOn;
            out.print(heartbeatOn ? F("HB,ON") : F("HB,OFF"));
            endReply(from, now);
            return true;
        case 'h':
        case '?':
            printHelp();
            return true;
        default:
            return false;
    }
}

// Shows collected non-command text on USB only, e.g.  BT says: "+ERROR01"  (t=46072 ms)
void flushRaw(Link& l, uint32_t now) {
    if (l.rawLen == 0) return;
    Serial.print(l.name);
    Serial.print(F(" says: \""));
    Serial.write(reinterpret_cast<const uint8_t*>(l.raw), l.rawLen);
    Serial.print(F("\"  (t="));
    Serial.print(now);
    Serial.println(F(" ms)"));
    l.rawLen = 0;
}

void receive(Link& l, char c, uint32_t now) {
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
        if (handleCommand(c, l, now)) return;
    }
    // Not a command: keep it (printable characters only, '.' for the rest) to show on USB.
    if (l.rawLen == sizeof(l.raw)) flushRaw(l, now);
    l.raw[l.rawLen++] = (c >= 32 && c < 127) ? c : '.';
}

void sendHeartbeat(uint32_t now) {
    // HB,<t s>,<x cm>,<state>,<loop max ms>; x is "-" until odometry exists.
    out.print(F("HB,"));
    out.print(now / 1000.0f, 1);
    out.print(F(",-,"));
    out.print(running ? F("RUN") : F("STOP"));
    out.print(',');
    out.println(loopMaxUs / 1000.0f, 2);
}

}  // namespace

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);   // pin 13: not used by any MegaPi motor slot
    Serial.begin(cfg::SERIAL_BAUD);
    BT.begin(cfg::BT_BAUD);

    out.println();
    out.println(F("ColonyAFIS hw_link " CFG_FW_VERSION ": desk test 1, Serial + Bluetooth"));
    out.print(F("USB at "));
    out.print(cfg::SERIAL_BAUD);
    out.print(F(" baud, Bluetooth on Serial"));
    out.print(cfg::BT_SERIAL_PORT);
    out.print(F(" at "));
    out.print(cfg::BT_BAUD);
    out.println(F(" baud"));
    printHelp();

    lastLoopUs = micros();
    lastHbMs   = millis();
}

void loop() {
    // Slowest loop since the last heartbeat (the plan's "loop max ms").
    const uint32_t nowUs = micros();
    const uint32_t dtUs  = nowUs - lastLoopUs;
    lastLoopUs = nowUs;
    if (dtUs > loopMaxUs) loopMaxUs = dtUs;

    const uint32_t now = millis();

    // Handle every byte already received (never waits for more), then show any finished raw text.
    for (Link& l : links) {
        while (l.port->available() > 0) receive(l, static_cast<char>(l.port->read()), now);
        if (l.rawLen > 0 && now - l.lastByteMs > cfg::CMD_GAP_MS) flushRaw(l, now);
    }

    if (now - lastHbMs >= cfg::HEARTBEAT_PERIOD_MS) {
        lastHbMs = now;
        ++hbCount;
        if (heartbeatOn) sendHeartbeat(now);
        loopMaxUs = 0;
        digitalWrite(LED_BUILTIN, running ? (hbCount & 1) : HIGH);   // blink = RUN, steady = STOP
    }
}

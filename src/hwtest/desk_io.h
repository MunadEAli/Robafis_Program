#pragma once
// Shared USB + Bluetooth console for the desk-test programs (src/hwtest/*.cpp).
//
// - desk::out() prints to USB and to Bluetooth (cfg::BT_SERIAL_PORT).
// - desk::poll() reads both links. A letter counts as a command only at the start of a line or
//   after a short silence (cfg::CMD_GAP_MS), i.e. when a person typed it.
// - Text that is not a command is shown on USB only and never sent back to Bluetooth. While no
//   phone is connected the Bluetooth module answers whatever it receives with "+ERROR01"; echoing
//   that back would make it answer again forever. Repeated module lines are shown at most every 5 s.
// - Lines starting with '#' are comments, so a saved log can be read as CSV by skipping them.

#include <Arduino.h>

namespace desk {

// Returns true if the letter was a command and was handled.
typedef bool (*Handler)(char c, uint32_t now);

void   begin();
Print& out();
void   poll(uint32_t now, Handler handler);

// Ends a reply line with "  (from USB, t=12345 ms)" for the command being handled.
void endReply(uint32_t now);

}  // namespace desk

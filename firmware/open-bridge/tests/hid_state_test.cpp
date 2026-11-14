// Copyright (C) 2026 DShare-HID contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include "../hid_state.h"
#include <cassert>
#include <cstdint>
#include <vector>
using open_bridge::HidState;
struct Report { HidState::Report type; std::vector<uint8_t> bytes; };
static void send(void *ctx, HidState::Report type, const uint8_t *data, size_t n)
{ static_cast<std::vector<Report> *>(ctx)->push_back({type, {data, data + n}}); }
int main()
{
  std::vector<Report> reports;
  HidState state(send, &reports);
  auto event = [&](uint8_t type, std::vector<uint8_t> payload) {
    std::vector<uint8_t> frame{0x55, 0xaa, type, static_cast<uint8_t>(payload.size())};
    frame.insert(frame.end(), payload.begin(), payload.end());
    return state.handle(frame.data(), frame.size());
  };
  assert(event(1, {2, 4})); assert(event(1, {2, 5}));
  assert(reports.back().bytes[0] == 2 && reports.back().bytes[2] == 4 && reports.back().bytes[3] == 5);
  assert(event(2, {0, 4})); assert(reports.back().bytes[2] == 0 && reports.back().bytes[3] == 5);
  assert(event(4, {1})); assert(event(3, {0xfe, 0xff, 0x01, 0x00}));
  // movement is accumulated until it is flushed
  assert(state.hasPendingMotion()); assert(state.flushMotion()); assert(!state.flushMotion());
  assert((reports.back().bytes == std::vector<uint8_t>{1, 0xfe, 0xff, 1, 0, 0}));
  assert(event(4, {3})); assert(reports.back().bytes[0] == 3);
  assert(event(4, {2})); assert(reports.back().bytes[0] == 7);
  assert(event(5, {3})); assert(reports.back().bytes[0] == 5);
  assert(event(6, {0xff})); assert(reports.back().bytes[5] == 0xff);
  assert(event(7, {0xe9, 0})); assert(reports.back().bytes[0] == 0xe9);
  assert(!event(3, {1}));

  // many small moves become one report with their sum
  const size_t before = reports.size();
  for (int i = 0; i < 50; ++i) assert(event(3, {3, 0, 0xff, 0xff}));
  assert(reports.size() == before);
  assert(state.flushMotion() && reports.size() == before + 1);
  assert((reports.back().bytes == std::vector<uint8_t>{5, 150, 0, 0xce, 0xff, 0}));

  // more than 16 bits per axis is sent over several reports, nothing is lost
  for (int i = 0; i < 3; ++i) assert(event(3, {0x00, 0x80, 0xff, 0x7f}));
  int32_t sumX = 0, sumY = 0;
  while (state.flushMotion()) {
    const auto &b = reports.back().bytes;
    sumX += int16_t(uint16_t(b[1]) | (uint16_t(b[2]) << 8));
    sumY += int16_t(uint16_t(b[3]) | (uint16_t(b[4]) << 8));
  }
  assert(sumX == 3 * -32768 && sumY == 3 * 32767);

  // a button press carries the movement before it, so it lands where the pointer went
  assert(event(3, {10, 0, 20, 0})); assert(event(5, {1}));
  assert((reports.back().bytes == std::vector<uint8_t>{4, 10, 0, 20, 0, 0}));
  assert(!state.hasPendingMotion());

  // so does a wheel report
  assert(event(3, {0xfb, 0xff, 0, 0})); assert(event(6, {1}));
  assert((reports.back().bytes == std::vector<uint8_t>{4, 0xfb, 0xff, 0, 0, 1}));

  // absolute positions: only the latest one is sent, clamped to 0..32767
  assert(!state.absoluteMode());
  assert(event(9, {0x10, 0x00, 0x20, 0x00})); assert(event(9, {0x00, 0x40, 0xff, 0xff}));
  assert(state.absoluteMode() && state.hasPendingMotion());
  const size_t beforeAbs = reports.size();
  assert(state.flushMotion() && reports.size() == beforeAbs + 1 && !state.flushMotion());
  assert(reports.back().type == HidState::Report::MouseAbsolute);
  assert((reports.back().bytes == std::vector<uint8_t>{4, 0x00, 0x40, 0xff, 0x7f, 0}));
  assert(!event(9, {1, 2, 3}));

  // in absolute mode buttons and the wheel are reported at the absolute position
  assert(event(4, {1}));
  assert(reports.back().type == HidState::Report::MouseAbsolute);
  assert((reports.back().bytes == std::vector<uint8_t>{5, 0x00, 0x40, 0xff, 0x7f, 0}));
  assert(event(6, {0xfe}));
  assert(reports.back().type == HidState::Report::MouseAbsolute && reports.back().bytes[5] == 0xfe);

  // releasing everything ends absolute mode and releases its buttons too
  state.releaseAll();
  assert(!state.absoluteMode());
  assert(reports[reports.size()-4].type == HidState::Report::MouseAbsolute);
  assert(reports[reports.size()-4].bytes[0] == 0);
  assert(event(4, {1}) && reports.back().type == HidState::Report::Mouse);
  state.releaseAll();

  // releasing everything drops pending movement
  assert(event(3, {1, 0, 1, 0}));
  state.releaseAll();
  assert(!state.hasPendingMotion());
  assert(reports[reports.size()-3].bytes == std::vector<uint8_t>(8));
  assert(reports[reports.size()-2].bytes == std::vector<uint8_t>(6));
  assert(reports.back().bytes == std::vector<uint8_t>(2));
}

// Copyright (C) 2026 DShare-HID contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include "../hid_state.h"
#include <cassert>
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
  assert((reports.back().bytes == std::vector<uint8_t>{1, 0xfe, 0xff, 1, 0, 0}));
  assert(event(6, {0xff})); assert(reports.back().bytes[5] == 0xff);
  assert(event(7, {0xe9, 0})); assert(reports.back().bytes[0] == 0xe9);
  assert(!event(3, {1}));
  state.releaseAll();
  assert(reports[reports.size()-3].bytes == std::vector<uint8_t>(8));
  assert(reports[reports.size()-2].bytes == std::vector<uint8_t>(6));
  assert(reports.back().bytes == std::vector<uint8_t>(2));
}

/*
 * SPDX-FileCopyrightText: 2026 DShare-HID Open Bridge contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "server/InputFilter.h"

#include <set>
#include <string>
#include <vector>

class IEventQueue;

/**
 * @brief Hotkey action that hands the keyboard and mouse to a bridge client's device, or takes
 * them back: toggleBridgeControl(<computer>[,<computer>...])
 *
 * While a bridge client has control the cursor is locked to it and the server sends it raw
 * relative movement, so the device's own pointer acceleration can't make the tracked position
 * drift. The server keeps no position on the device and there are no edges to cross.
 *
 * With one computer the action toggles between it and this computer. With several it cycles
 * through those that are connected, in the order given, and then back to this computer.
 * The GUI writes these hotkeys into the server configuration itself
 * (gui/core/BridgeProfileShortcuts) and can request the same switch over the core IPC.
 */
class BridgeControlAction : public InputFilter::Action
{
public:
  static constexpr auto kName = "toggleBridgeControl";

  BridgeControlAction(IEventQueue *events, std::vector<std::string> computers);

  const std::vector<std::string> &getComputers() const;

  // Action overrides
  Action *clone() const override;
  std::string format() const override;
  void perform(const Event &) override;

private:
  std::vector<std::string> m_computers;
  IEventQueue *m_events;
};

namespace deskflow::server {

/**
 * @brief The computer that should have control after a bridge control request
 *
 * @param computers the request's computers, in order
 * @param connected the computers that are connected now
 * @param current the computer that has control now, empty if none
 * @return the computer to hand control to, or an empty string to give it back to this computer
 */
std::string nextBridgeControl(
    const std::vector<std::string> &computers, const std::set<std::string> &connected, const std::string &current
);

} // namespace deskflow::server

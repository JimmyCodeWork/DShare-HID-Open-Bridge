/*
 * SPDX-FileCopyrightText: 2026 DShare-HID Open Bridge contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/BridgeControlAction.h"

#include "base/IEventQueue.h"
#include "base/Log.h"
#include "server/Server.h"

#include <algorithm>

BridgeControlAction::BridgeControlAction(IEventQueue *events, std::vector<std::string> computers)
    : m_computers(std::move(computers)),
      m_events(events)
{
  // do nothing
}

const std::vector<std::string> &BridgeControlAction::getComputers() const
{
  return m_computers;
}

InputFilter::Action *BridgeControlAction::clone() const
{
  return new BridgeControlAction(*this);
}

std::string BridgeControlAction::format() const
{
  std::string computers;
  for (const auto &computer : m_computers) {
    computers += computers.empty() ? computer : "," + computer;
  }
  return std::string(kName) + "(" + computers + ")";
}

void BridgeControlAction::perform(const Event &event)
{
  LOG_INFO("bridge control hotkey: %s", format().c_str());
  auto *info = new Server::BridgeControlInfo(m_computers);
  m_events->addEvent(
      Event(EventTypes::ServerBridgeControl, event.getTarget(), info, Event::EventFlags::DeliverImmediately)
  );
}

namespace deskflow::server {

std::string nextBridgeControl(
    const std::vector<std::string> &computers, const std::set<std::string> &connected, const std::string &current
)
{
  std::vector<std::string> available;
  std::ranges::copy_if(computers, std::back_inserter(available), [&connected](const std::string &computer) {
    return connected.contains(computer);
  });

  const auto it = std::ranges::find(available, current);
  if (current.empty() || it == available.end()) {
    // nothing of this request has control yet: hand it to the first one, if any is connected
    return available.empty() ? std::string() : available.front();
  }

  // the next one, and after the last back to this computer
  const auto next = std::next(it);
  return next == available.end() ? std::string() : *next;
}

} // namespace deskflow::server

/*
 * dshare-hid -- created by dshare.hid@gmail.com
 */

#pragma once

#include <cstdint>
#include <vector>

namespace deskflow::bridge {

enum class HidEventType : uint8_t
{
  KeyboardPress = 0x01,
  KeyboardRelease = 0x02,
  MouseMove = 0x03,
  MouseButtonPress = 0x04,
  MouseButtonRelease = 0x05,
  MouseScroll = 0x06,
  ConsumerControlPress = 0x07,
  ConsumerControlRelease = 0x08,
  // Open Bridge firmware (experimental): the pointer position as two little-endian uint16
  // values, X then Y, scaled to 0..32767 across the device's screen (see PointerHoming.h)
  MouseAbsolute = 0x09,
};

/**
 * @brief HID protocol packet wrapper
 *
 * Format (little endian):
 *   header (0xAA55) | type (1 byte) | length (1 byte) | payload (N bytes)
 */
struct HidEventPacket
{
  static constexpr uint16_t MAGIC = 0xAA55;

  HidEventType type;
  std::vector<uint8_t> payload;

  std::vector<uint8_t> serialize() const;
};

} // namespace deskflow::bridge

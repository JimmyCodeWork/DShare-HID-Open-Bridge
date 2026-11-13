# SPDX-License-Identifier: GPL-2.0-only
# Added 2026-11-13 for independently built bridge firmware.
include_guard(GLOBAL)

option(DSH_CUSTOM_BRIDGE "Use independently built bridge firmware and runtime device keys" ON)

if(DSH_CUSTOM_BRIDGE)
  # This is a separate firmware implementation, not an activation bypass for vendor firmware.
  # Do not even configure the vendor's downloader/flasher submodule in this mode.
  set(DSH_VENDOR_FIRMWARE_TOOLS OFF)
else()
  option(DSH_VENDOR_FIRMWARE_TOOLS "Build upstream vendor firmware download and payment tools" ON)
endif()

# SPDX-License-Identifier: GPL-2.0-only
# Added 2026-11-13 for independently built bridge firmware.
include_guard(GLOBAL)

option(DSH_CUSTOM_BRIDGE "Use independently built bridge firmware and an explicit device public key" ON)

if(DSH_CUSTOM_BRIDGE)
  set(DESKFLOW_CDC_PUBLIC_KEY "${CMAKE_SOURCE_DIR}/.local/bridge-keys/public-key.bin"
      CACHE FILEPATH "Uncompressed 65-byte P-256 public key for your bridge firmware")

  if(NOT EXISTS "${DESKFLOW_CDC_PUBLIC_KEY}" OR IS_DIRECTORY "${DESKFLOW_CDC_PUBLIC_KEY}")
    message(FATAL_ERROR
      "Custom bridge requires your own device public key. Run: "
      "pwsh -File devtools/New-BridgeIdentity.ps1 ; or set DESKFLOW_CDC_PUBLIC_KEY "
      "to your uncompressed P-256 public-key.bin. See docs/custom-bridge.md.")
  endif()

  file(SIZE "${DESKFLOW_CDC_PUBLIC_KEY}" _dsh_key_size)
  file(READ "${DESKFLOW_CDC_PUBLIC_KEY}" _dsh_key_prefix LIMIT 1 HEX)
  if(NOT _dsh_key_size EQUAL 65 OR NOT _dsh_key_prefix STREQUAL "04")
    message(FATAL_ERROR "Custom bridge public key must be 65 bytes: 0x04 followed by P-256 X and Y.")
  endif()

  # This is a separate firmware implementation, not an activation bypass for vendor firmware.
  # Do not even configure the vendor's downloader/flasher submodule in this mode.
  set(DSH_VENDOR_FIRMWARE_TOOLS OFF)
else()
  option(DSH_VENDOR_FIRMWARE_TOOLS "Build upstream vendor firmware download and payment tools" ON)
endif()

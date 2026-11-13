# Open bridge firmware (in progress)

This is an **experimental** ESP32-C3 firmware. It compiles, but USB handshake,
Bluetooth pairing, HID behavior and desktop compatibility have not yet been
verified on physical hardware. Do not consider it a finished product.

It supports one profile, BLE keyboard/mouse/consumer reports, signed USB
handshake and settings in NVS. A device-name change takes effect in Bluetooth
advertising after a restart. The firmware releases held inputs on Bluetooth
disconnect or after 90 seconds without USB frames. Bluetooth pairing uses
bonding and encryption, but has no display for authenticated passkey pairing.
The USB handshake authenticates the bridge to the desktop, not the desktop to
the bridge, and USB traffic is not encrypted.

## Windows build and flash

Use a freely flashable ESP32-C3 SuperMini with a data-capable USB cable. Do not
use a board whose boot security was irreversibly configured for vendor firmware.
Install Python, PlatformIO Core (`py -m pip install platformio`), and PowerShell
7. From the repository root in PowerShell:

```powershell
# Only if this directory does not already exist; never overwrite your identity.
pwsh -File devtools/New-BridgeIdentity.ps1
$env:DSH_BRIDGE_PRIVATE_KEY = (Resolve-Path .local/bridge-keys/private-key.pem).Path
py -m platformio run -d firmware/open-bridge
# Confirm the COM port first; replace COM3 with the actual port.
py -m platformio run -d firmware/open-bridge -t upload --upload-port COM3
```

The desktop must be built with the matching `.local/bridge-keys/public-key.bin`
as described in [custom bridge development](../../docs/custom-bridge.md).
The build embeds the private key in the generated binary and the ESP32 flash;
keep the binary, build directory, and device private. Do not upload those files
or the key. A copied key can impersonate the bridge. The board is not configured
with flash encryption by these instructions.

After flashing, restart the board, pair the iPad with “Open Bridge”, and test
the matching custom desktop build. The precise pairing and first-use steps still
need to be established through a hardware test. If flashing fails, a SuperMini
may require holding BOOT while connecting or resetting; avoid changing eFuses.

The decoder accepts payloads up to 256 bytes and discards malformed headers.
The caller must reset it when a USB session ends or a partial frame times out;
otherwise a truncated but plausible frame can hold up subsequent input.
The callback must consume or copy its payload before returning, and must not
recursively feed or reset the decoder.

Run the host-side decoder test with a C++17 compiler:

```sh
g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
  firmware/open-bridge/usb_frame_decoder.cpp \
  firmware/open-bridge/tests/usb_frame_decoder_test.cpp -o /tmp/usb-frame-test
/tmp/usb-frame-test
g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
  firmware/open-bridge/hid_state.cpp \
  firmware/open-bridge/tests/hid_state_test.cpp -o /tmp/hid-state-test
/tmp/hid-state-test
```

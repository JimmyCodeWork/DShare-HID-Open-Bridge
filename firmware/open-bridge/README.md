# Open bridge firmware (in progress)

This is an **experimental** ESP32-C3 firmware. It compiles, but USB handshake,
Bluetooth pairing, HID behavior and desktop compatibility have not yet been
verified on physical hardware. Do not consider it a finished product.

It supports one profile, BLE keyboard/mouse/consumer reports, signed USB
handshake and settings in NVS. Mouse movement from USB is accumulated and sent
as at most one Bluetooth report every 10 ms, because the desktop can send
movement far faster than a Bluetooth HID link delivers reports; a report per
movement would back up the link and stall the USB input. Button and wheel
reports carry any pending movement, so events keep their order.

An experimental absolute pointer (report ID 4, X/Y in 0..32767) is also
exposed. The firmware uses it once the desktop sends absolute positions
(HID event type `0x09`), which it does when **Absolute pointer
(experimental)** is enabled in the bridge client's settings. Whether iPadOS
accepts an absolute pointing device has not been verified on hardware. A device-name change takes effect in Bluetooth
advertising after a restart. The firmware releases held inputs on Bluetooth
disconnect or after 90 seconds without USB frames. Bluetooth pairing uses
bonding and encryption, but has no display for authenticated passkey pairing.
The USB handshake authenticates the bridge to the desktop, not the desktop to
the bridge, and USB traffic is not encrypted.

## Windows build and flash

Use a freely flashable ESP32-C3 SuperMini with a data-capable USB cable. Do not
use a board whose boot security was irreversibly configured for vendor firmware.
Install Python and dependencies (`py -m pip install platformio cryptography pyserial`).
From the repository root in PowerShell, replace COM3 with the actual port:

```powershell
py firmware/open-bridge/provision.py --port COM3
```

In a custom bridge desktop build, the **File → Set up open bridge** dialog can
run the same process. It lists available serial ports and also accepts a port
entered manually. Python and the packages above must be installed separately;
the first PlatformIO build also downloads its toolchain. The dialog pauses bridge
clients while the port is in use and displays the operation log.

The script makes a fresh key, builds and flashes in a temporary directory,
verifies the board's signature, and saves the public key to
`%USERPROFILE%\.dshare-hid\bridge-keys\Open-<serial>.bin` only after verification.
Running it again rotates that board's key; the previous public key remains until
the new handshake succeeds. When using the command line, close the bridge client before provisioning so the serial
port is free. If the flash succeeds but verification fails, run the command again;
the desktop retains its old key and cannot authenticate that new firmware.

The build embeds the private key in the generated binary and the ESP32 flash.
The temporary directory is deleted on normal exit and handled errors, but an
abrupt shutdown can leave it behind and ordinary deletion does not guarantee
secure erasure. A copied key can impersonate the bridge. Flash encryption and
secure boot are not enabled by these instructions.

After flashing, pair the iPad with “Open Bridge”, and test
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

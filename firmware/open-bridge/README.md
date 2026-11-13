# Open bridge firmware (in progress)

This directory is not yet a flashable firmware. The USB link decoder is a
platform-independent first piece; BLE HID, authentication, persistence and
device integration still need implementation and hardware verification.

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
```

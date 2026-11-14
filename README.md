# DShare-HID Open Bridge

**English** | [繁體中文](README.zh-TW.md)

DShare-HID Open Bridge is an independent fork of [DShare-HID](https://github.com/lockekk/dshare-hid), which is itself derived from [Deskflow](https://github.com/deskflow/deskflow). It keeps the desktop keyboard and mouse sharing architecture and develops its own ESP32-C3 Bluetooth HID bridge firmware. It is not an official upstream release.

> [!WARNING]
> **This is an experimental prerelease, not a verified working product.** CI builds the Windows installer and portable package, and an automated check installs and launches the application on a CI runner. It has not yet been tested on a real computer, and ESP32 flashing, the USB handshake, iPad pairing and actual keyboard and mouse operation have not been verified. Do not treat this version as a stable product.

## What it provides

- Experimental ESP32-C3 firmware: USB communication, a signed handshake, and Bluetooth keyboard, mouse and media key reports.
- A new device key is generated each time a board is set up. The desktop application loads the public key at runtime, so changing keys does not require rebuilding the desktop application.
- A desktop setup dialog and a command-line setup tool. Setup builds, flashes and verifies the board, then stores its public key.
- ESP32-C3 cross-compilation with a test key, host-side protocol tests and simulated dialog tests have passed. These tests do not replace testing on real hardware.

The current target is a single-profile ESP32-C3 SuperMini. Other boards, switching between multiple devices and platform compatibility have not been verified.

## Getting started

[v0.1.0-alpha.2 prerelease](https://github.com/JimmyCodeWork/DShare-HID-Open-Bridge/releases/tag/v0.1.0-alpha.2) | [All releases](https://github.com/JimmyCodeWork/DShare-HID-Open-Bridge/releases)

The v0.1.0-alpha.2 prerelease provides an experimental Windows installer (`.msi`) and portable package (`-portable.zip`). Both are unsigned; see [Downloads and verification](#downloads-and-verification).

1. Install the MSI, or extract the portable package and run `dshare-hid.exe`. The portable package keeps its settings in its `settings` folder and does not include the background service, so it cannot control the device at UAC prompts or on the sign-in screen.
2. Connect an ESP32-C3 SuperMini and choose **File → Set up open bridge...** to build and flash the firmware with a new device key. There is no prebuilt firmware, because every build contains its own private key. This step requires Python 3 and the packages listed in [Firmware setup and security](#firmware-setup-and-security).
3. Pair the iPad with **Open Bridge** in its Bluetooth settings.

For development and build information, see:

- [Custom bridge development guide](docs/custom-bridge.md)
- [Firmware, keys and flashing](firmware/open-bridge/README.md)
- [Desktop application build guide](docs/dev/build.md)

## Downloads and verification

Windows installers and portable builds are built by GitHub Actions from release tags and published on the [Releases](https://github.com/JimmyCodeWork/DShare-HID-Open-Bridge/releases) page, together with `SHA256SUMS.txt` and a build provenance attestation. Prerelease builds are **not code-signed yet**, so Chrome or Windows SmartScreen may warn that the file is uncommon. Download only from this repository's Releases page, and verify the file before you run it:

```powershell
Get-FileHash .\<file> -Algorithm SHA256
gh attestation verify .\<file> --repo JimmyCodeWork/DShare-HID-Open-Bridge
```

The hash must match the entry in `SHA256SUMS.txt` from the same release. For signing and verification details, see [Windows code signing and release verification](docs/windows-code-signing.md).

## Firmware setup and security

The setup tool currently requires Python, PlatformIO, `cryptography` and `pyserial`.

- The firmware build contains the device's private key, which is also written to the board's flash memory. Do not share the private key, or firmware and build artifacts that contain it.
- Temporary files created during setup are deleted on normal completion and on handled errors, but ordinary deletion does not guarantee secure erasure.
- Setup does not enable Secure Boot or Flash Encryption and does not burn eFuses that restrict reflashing, so the board can still be reflashed and reused.

## Credits and license

Thanks to the contributors of [DShare-HID](https://github.com/lockekk/dshare-hid) and [Deskflow](https://github.com/deskflow/deskflow). This fork keeps the upstream copyright and license notices. See [LICENSE](LICENSE) and the license notices in each source file.

The original upstream README files are kept as references in [English](README.upstream.md) and [Simplified Chinese](README_zh-CN.md). Their downloads, vendor firmware and feature descriptions apply to upstream, not to this fork.

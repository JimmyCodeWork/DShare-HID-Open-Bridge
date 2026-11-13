# Building DShare-HID

To build DShare-HID you will a minimum of: 
    - [cmake] 3.24+
    - [Qt] 6.7.0+ (Windows builds and CI use the version in `cmake/qt-version.txt`)
    - [openssl] 3.0+
    - [libportal] 0.9.1+ (linux, bsd)
    - [libei] 1.3+ (linux, bsd)
    - [libxkbcommon] with xkbregistry (linux, bsd), e.g. Ubuntu: `sudo apt install libxkbcommon-dev libxkbregistry-dev`

> ^ Will be fetched if not found on the host system.

By default a build of DShare-HID will: 
     - The GUI application `dshare-hid`
     - The Core application `dshare-hid-core`
     - Documentation if [doxygen] was found on your system
     - Tests that will be run as part of the build process

## Configuration

DShare-HID supports the following CMake options:

|         Option           |            Description                  |   Default Value    | Additional requirements |
:-------------------------:|:---------------------------------------:|:------------------:|:-----------------------:|
| BUILD_DEV_DOCS           | Build development documentation         | OFF                | `Doxygen` |
| BUILD_INSTALLER          | Build installers/packages               | ON                 | |
| BUILD_TESTS              | Build unit tests and legacy tests       | ON                 | |
| BUILD_X11_SUPPORT        | Build X11 backend (Linux and BSD only)  | ON                 | `x11 libs`|
| BUILD_OSX_BUNDLE         | Build an app bundle (macOS only)        | ON                 | |
| ENABLE_COVERAGE          | Enable test coverage                    | OFF                | `gcov` |
| SKIP_BUILD_TESTS         | Skip running of tests at build time     | OFF                | |
| VCPKG_QT                 | Build Qt w/ vcpkg (Windows only)        | OFF                | |
| CLEAN_TRS                | Remove obsolete strings from tr files   | OFF                | |
| APPLE_CODESIGN_DEV       | Apple codesign cert ID for development  | Not set            | | 

Example cmake configuration:
`cmake -S. -Bbuild -DCMAKE_INSTALL_PREFIX=<INSTALLPREFIX>`

### Windows Configuration

The recommended setup uses prebuilt Qt and the OpenSSL 3 build that Qt distributes. Nothing has to
be compiled from source, and it matches the CI build. The Qt version is pinned in
[`cmake/qt-version.txt`](../../cmake/qt-version.txt).

#### Prebuilt Qt and OpenSSL (recommended)

1. Install Qt and OpenSSL into one folder, for example `C:\Qt`. Use one of these:
   - `run_task.bat setup-qt`, which installs the pinned Qt version and Qt's OpenSSL 3 package with
     [aqtinstall] (requires Python 3).
   - The [Qt] online installer. Select *Qt → <version> → MSVC 2022 64-bit* and
     *Developer and Designer Tools → OpenSSL 3 Toolkit → OpenSSL 64-bit binaries*.
2. Set the `QT_ROOT` environment variable to that folder (for example `C:\Qt`). `run_task.bat`
   then configures with prebuilt Qt and OpenSSL instead of vcpkg.

To configure by hand:

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH=C:/Qt/<version>/msvc2022_64 ^
  -DOPENSSL_ROOT_DIR=C:/Qt/Tools/OpenSSLv3/Win_x64
```

After each build, the Qt and OpenSSL DLLs are copied next to the executables and unit tests in the
build tree (see [`cmake/WindowsRuntime.cmake`](../../cmake/WindowsRuntime.cmake)), so they run
without changing `PATH`.

#### vcpkg (legacy)

If `QT_ROOT` is not set, `run_task.bat` uses vcpkg from `VCPKG_ROOT` with `-DVCPKG_QT=ON`.
vcpkg builds OpenSSL and Qt from source, so the first build can take hours. vcpkg keeps built
packages in its binary cache (`%LOCALAPPDATA%\vcpkg\archives`), so later builds reuse them as long
as the compiler and port versions do not change.

Do not mix the two setups in one build folder. Delete `build` and the generated `vcpkg.json` in the
project root when switching.

### macOS codesign

The code signing option `APPLE_CODESIGN_DEV` is only for local development and not intended for distributed bundles. 

Signing for local development and signing for the distribution bundle must be different because of development entitlements which are unlikely to be safe for use in production. It is impractical (i.e. very slow and cumbersome) to use the distribution bundle for local development. When developing locally, the app bundle is partial and does not contain dependencies and uses external libs, e.g. installed with Homebrew; the entitlements allow those external libs to be loaded which is not allowed by default.

For development codesign:

1. Install Xcode
2. Go to Settings -> Accounts
3. Add your account (requires a free Apple Developer ID)
4. Manage certificates -> Add -> Apple Development
5. To get your ID, run: `security find-identity -v -p codesigning login.keychain-db`
6. Pass the ID to CMake, e.g. `-DAPPLE_CODESIGN_DEV=Apple Development: bob@exmaple.com (KLGSJHLFXY)`
7. Configure and build
8. To verify, run: `codesign -d -r- build/bin/DShare-HID.app`

## Build

After configuring you should be able to run make to build all targets.

`cmake --build build`

## Install

 To test installation run `DESTDIR=<installDIR> cmake --install build` to install into `<installDir>/<CMAKE_INSTALL_PREFIX>`

 Running `cmake --install build` will install to the `CMAKE_INSTALL_PREFIX`

## Making DShare-HID packages

 DShare-HID can generate several packages using `cpack`.
 
 To generate packages build the `package` or `package_source` target.

 Example: ` cmake --build build --target package package_source` would generate both package and package source packages.
 
 DShare-HID can generate several package types depending on the system. 
 
 Archive-based packages should work on all platforms. On Linux deb and rpm info is set up, Flatpaks can be generated from the included file in deploy/linux and a `PKGBUILD` for Arch linux is generated in the build folder. On macos a dmg file will be created and signed. For windows WiX can be used to create an installer.
 
[Qt]:https://www.qt.io
[doxygen]:http://www.stack.nl/~dimitri/doxygen/
[cmake]:https://cmake.org/
[openssl]:https://www.openssl.org/
[libei]:https://gitlab.freedesktop.org/libinput/libei
[libportal]:https://github.com/flatpak/libportal
[libxkbcommon]:https://xkbcommon.org/
[aqtinstall]:https://github.com/miurahr/aqtinstall

# DShare-HID Open Bridge

[English](#english) | [繁體中文](#繁體中文)

## 繁體中文

DShare-HID Open Bridge 是 [DShare-HID](https://github.com/lockekk/dshare-hid) 的 fork，沿用其桌面鍵鼠共享架構，開發自行實作的 ESP32-C3 藍牙 HID 橋接韌體。DShare-HID 源自 [Deskflow](https://github.com/deskflow/deskflow)。本 fork 不是上游作者的官方版本。

> [!WARNING]
> **目前是實驗性測試版，尚未確認可正常使用。** Windows 桌面程式尚未完成整體建置，也尚未驗證 ESP32 燒錄、USB 握手、iPad 配對及實際鍵鼠操作。請勿將此版本視為穩定產品。

### 目前提供什麼

- 實驗性的 ESP32-C3 韌體：USB 通訊、簽章握手，以及藍牙鍵盤、滑鼠和媒體鍵報告。
- 每次設定板子時產生新的裝置金鑰；桌面端於執行時載入公鑰，換鑰不需要重新編譯桌面程式。
- 桌面設定介面與命令列設定工具；設定時嘗試編譯、燒錄、驗證板子，再儲存公鑰。
- 使用測試金鑰的 ESP32-C3 交叉編譯、部分主機端測試及介面模擬測試已通過；這些測試不能代替實機驗證。

目前目標是單一設定檔的 ESP32-C3 SuperMini；其他型號、多裝置切換及平台相容性尚未驗證。韌體設定流程不啟用 Secure Boot 或 Flash Encryption，也不寫入限制重新燒錄的 eFuse，讓板子保留重刷與改作他用的能力。

### 取得與開發

[查看 v0.1.0-alpha.1 測試版](https://github.com/JimmyCodeWork/DShare-HID-Open-Bridge/releases/tag/v0.1.0-alpha.1)｜[所有發行版本](https://github.com/JimmyCodeWork/DShare-HID-Open-Bridge/releases)

目前發行版本提供原始碼，**沒有已驗證的安裝程式或可直接使用的韌體**。開發與建置資訊：

- [自製橋接開發說明](docs/custom-bridge.md)
- [韌體、金鑰與燒錄說明](firmware/open-bridge/README.md)
- [上游桌面程式建置說明](docs/dev/build.md)

設定工具目前需要 Python、PlatformIO、`cryptography` 和 `pyserial`。私鑰會被放進韌體與板子的快閃記憶體；設定時產生的臨時檔會在正常結束或可處理的錯誤後刪除，但一般刪除不保證資料無法復原。請勿公開分享私鑰或含私鑰的韌體與建置產物。

### 來源與授權

感謝 [DShare-HID](https://github.com/lockekk/dshare-hid) 及 [Deskflow](https://github.com/deskflow/deskflow) 的貢獻者。本 fork 保留上游版權與授權聲明；詳見 [LICENSE](LICENSE) 和各原始檔的授權標記。原始上游說明保留於 [README.upstream.md](README.upstream.md) 與 [README_zh-CN.md](README_zh-CN.md)；其中原廠下載、付費韌體及功能敘述不代表本 fork 已支援或驗證。

## English

DShare-HID Open Bridge is an independent fork of [DShare-HID](https://github.com/lockekk/dshare-hid), itself derived from [Deskflow](https://github.com/deskflow/deskflow). It develops independently implemented ESP32-C3 Bluetooth HID bridge firmware. It is not an official upstream release.

**This is an experimental prerelease, not a verified working product.** The full Windows desktop build, hardware flashing, USB handshake, iPad pairing, and keyboard/mouse operation have not yet been verified. The [alpha release](https://github.com/JimmyCodeWork/DShare-HID-Open-Bridge/releases/tag/v0.1.0-alpha.1) contains source code only; there is no verified installer or ready-to-use firmware.

The prototype includes a single-profile ESP32-C3 firmware, device-key provisioning, runtime public-key loading, and a desktop setup dialog. Cross-compilation with a test key, host-side protocol tests, and simulated dialog tests have passed. See the [development guide](docs/custom-bridge.md) and [firmware instructions](firmware/open-bridge/README.md) for prerequisites, security limitations, and testing status.

The setup does not enable Secure Boot or Flash Encryption or burn eFuses that restrict reflashing. A firmware build contains its private key; keep the board and build artifacts private. Temporary files are deleted on normal completion or handled errors, but ordinary deletion does not guarantee secure erasure.

The original upstream README files are preserved as [English](README.upstream.md) and [Simplified Chinese](README_zh-CN.md) references; their downloads, vendor firmware, and feature claims describe upstream, not this fork. See [LICENSE](LICENSE) and source-file notices for applicable licensing.

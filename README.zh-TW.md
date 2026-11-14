# DShare-HID Open Bridge

[English](README.md) | **繁體中文**

DShare-HID Open Bridge 是 [DShare-HID](https://github.com/lockekk/dshare-hid) 的獨立 fork，而 DShare-HID 源自 [Deskflow](https://github.com/deskflow/deskflow)。本專案沿用其桌面鍵鼠共享架構，並自行開發 ESP32-C3 藍牙 HID 橋接韌體。本專案並非上游作者發布的官方版本。

> [!WARNING]
> **目前為實驗性預覽版，尚未驗證可正常使用。** CI 已能建置 Windows 安裝程式與可攜版，並會在 CI 環境中自動安裝及啟動程式進行檢查，但尚未在實體電腦上實測；ESP32 燒錄、USB 握手、iPad 配對，以及實際的鍵鼠操作也均尚未驗證。請勿將此版本視為穩定產品。

## 功能概覽

- 實驗性 ESP32-C3 韌體：支援 USB 通訊、簽章握手，以及藍牙鍵盤、滑鼠與媒體鍵報告。
- 每次設定板子時都會產生新的裝置金鑰。桌面程式於執行時載入公鑰，因此更換金鑰不需要重新編譯桌面程式。
- 提供桌面設定介面與命令列設定工具。設定流程會編譯、燒錄並驗證板子，然後儲存公鑰。
- 已通過使用測試金鑰的 ESP32-C3 交叉編譯、主機端通訊協定測試，以及設定介面的模擬測試。這些測試無法取代實機驗證。

目前的目標硬體為單一設定檔的 ESP32-C3 SuperMini。其他板子型號、多裝置切換及平台相容性均尚未驗證。

## 開始使用

[v0.1.0-alpha.2 預覽版](https://github.com/JimmyCodeWork/DShare-HID-Open-Bridge/releases/tag/v0.1.0-alpha.2)｜[所有發行版本](https://github.com/JimmyCodeWork/DShare-HID-Open-Bridge/releases)

v0.1.0-alpha.2 預覽版提供實驗性的 Windows 安裝程式（`.msi`）與可攜版（`-portable.zip`），兩者均未經程式碼簽章，請參閱[下載與驗證](#下載與驗證)。

1. 安裝 MSI，或解壓縮可攜版後執行 `dshare-hid.exe`。可攜版的設定存放在其 `settings` 資料夾中，且不含背景服務，因此無法在 UAC 提示或 Windows 登入畫面操作裝置。
2. 連接 ESP32-C3 SuperMini，選擇「**檔案 → 設定 Open Bridge...**」以建立新的裝置金鑰，並編譯、燒錄韌體。由於每份韌體都內含各自的私鑰，因此不提供預先編譯的韌體。此步驟需要 Python 3 及[韌體設定與安全性](#韌體設定與安全性)中列出的套件。
3. 在 iPad 的藍牙設定中與「**Open Bridge**」配對。
4. 按 **Ctrl+Alt+Shift+1**（或橋接用戶端的「**控制**」按鈕）把鍵盤和滑鼠交給 iPad，再按一次即交還；Ctrl+Alt+Shift+0 會依序切換所有橋接用戶端。若要改用游標移過螢幕邊緣的方式，請參閱[自製橋接開發指南](docs/custom-bridge.md)。

開發與建置資訊請參閱：

- [自製橋接開發指南](docs/custom-bridge.md)
- [韌體、金鑰與燒錄說明](firmware/open-bridge/README.md)（英文）
- [桌面程式建置指南](docs/dev/build.md)（英文）

## 下載與驗證

Windows 安裝程式與可攜版由 GitHub Actions 依發行標記建置，並發布於 [Releases](https://github.com/JimmyCodeWork/DShare-HID-Open-Bridge/releases) 頁面，同時附上 `SHA256SUMS.txt` 與建置來源證明（build provenance attestation）。預覽版**尚未進行程式碼簽章**，因此 Chrome 或 Windows SmartScreen 可能會警告這是不常見的檔案。請僅從本專案的 Releases 頁面下載，並在執行前驗證檔案：

```powershell
Get-FileHash .\<檔案> -Algorithm SHA256
gh attestation verify .\<檔案> --repo JimmyCodeWork/DShare-HID-Open-Bridge
```

雜湊值必須與同一版本 `SHA256SUMS.txt` 中的紀錄相符。簽章與驗證的詳細說明請參閱 [Windows 程式碼簽章與發行驗證](docs/windows-code-signing.md)（英文）。

## 韌體設定與安全性

設定工具目前需要 Python、PlatformIO、`cryptography` 與 `pyserial`。

- 韌體建置產物包含裝置的私鑰，私鑰也會寫入板子的快閃記憶體。請勿分享私鑰，或任何包含私鑰的韌體與建置產物。
- 設定過程產生的暫存檔會在正常結束或發生可處理的錯誤後刪除，但一般刪除方式不保證資料無法復原。
- 設定流程不會啟用 Secure Boot 或 Flash Encryption，也不會燒寫限制重新燒錄的 eFuse，因此板子仍可重新燒錄並改作他用。

## 致謝與授權

感謝 [DShare-HID](https://github.com/lockekk/dshare-hid) 與 [Deskflow](https://github.com/deskflow/deskflow) 的貢獻者。本 fork 保留上游的版權與授權聲明，詳見 [LICENSE](LICENSE) 及各原始碼檔案中的授權標示。

上游的原始 README 保留為參考資料：[英文](README.upstream.md)與[簡體中文](README_zh-CN.md)。其中的下載連結、原廠韌體與功能描述適用於上游專案，不代表本 fork 已支援或驗證。

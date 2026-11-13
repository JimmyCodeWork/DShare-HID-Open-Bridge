# 自製 ESP32-C3 橋接開發版

此 Fork 以 **ESP32-C3 Supermini + 自行實作的韌體**為目標。沿用 DShare-HID
的電腦端、螢幕配置與公開的 USB 通訊格式；不使用原供應者的付費韌體。
原專案版權、授權及來源資訊保留。新增及修改內容日期：2026-11-13。

## 目前狀態

- 已加入預設開啟的 `DSH_CUSTOM_BRIDGE` 建置選項。
- 自製橋接模式要求提供自己的 P-256 公鑰，不會退回原作者的內建公鑰。
- 自製模式不載入 `esp32-hid-tools` 子模組，因此不建置其下載、燒錄、付款介面。
- 已加入 ESP32-C3 實驗版韌體，使用測試金鑰交叉編譯通過；尚未完成桌面整體建置、燒錄或 iPad 實機驗證。

這仍是實驗版，不能將編譯成功視為已驗證可用。

韌體的 Windows 建置及燒錄步驟見 [韌體說明](../firmware/open-bridge/README.md)。

## 產生自己的裝置身分

在專案根目錄以 PowerShell 7 執行：

```powershell
pwsh -File devtools/New-BridgeIdentity.ps1
```

檔案會放在 `.local/bridge-keys/`：

| 檔案 | 用途 |
|---|---|
| `private-key.pem` | 自製韌體的 P-256 私鑰，PKCS#8 格式；不可提交 Git 或上傳 |
| `public-key.bin` | 桌面端使用的 65-byte 未壓縮公鑰：`04 + X(32) + Y(32)` |
| `public-key.pem` | 公鑰的 SubjectPublicKeyInfo PEM 格式，供驗證與檢查 |

工具拒絕覆寫既有目錄；輪替金鑰請指定另一個 `-OutputDirectory`。
`.local/` 已由 Git 忽略，身分目錄也有獨立 `.gitignore`。私鑰目前以未加密 PEM
儲存在本機，請維持檔案的適當存取權限。每個橋接板可以使用不同身分。

## 桌面建置設定

先依 [上游建置說明](dev/build.md)準備 CMake、C++20 編譯器、Qt 6.7+、OpenSSL 3+
及各平台相依套件，再將下列參數加入原本的 configure 指令：

```text
-DDSH_CUSTOM_BRIDGE=ON
-DDESKFLOW_CDC_PUBLIC_KEY=<absolute-path-to-public-key.bin>
```

未指定公鑰路徑時，預設使用本專案 `.local/bridge-keys/public-key.bin`。
不要把 `private-key.pem` 傳給這個參數。設定檢查會拒絕長度或前綴錯誤的檔案；
實際曲線座標與簽章的有效性仍由 OpenSSL 在執行時驗證。

如需編譯未修改的上游模式，使用獨立建置目錄並明確指定
`-DDSH_CUSTOM_BRIDGE=OFF`。這不是繞過原廠韌體授權；原廠韌體仍受其自身條款限制。

## 自製韌體需要實作的公開介面

協定的權威來源是 `src/lib/platform/bridge/CdcTransport.cpp`、`CdcTransport.h`
及 `HidFrame.cpp`，下面僅整理這個版本的必要項目。

1. **USB 通訊**：外層為 little-endian `5A C3 | version | type | flags | reserved |
   payload_length(uint16) | payload`。版本為 `01`。控制訊息 type 是 `80`；
   完整 HID 事件 type 是 `01`，內層為 `55 AA | event_type | length | payload`。
   接收端需處理分段輸入、封包重同步、長度上限及不合法事件。
2. **握手**：HELLO 控制 ID `01`；自製韌體回 ACK ID `81`，包含 16-byte 核心欄位、
   32-byte 裝置 nonce 與 64-byte ECDSA `R || S`。目前桌面端驗證的簽章內容是
   `host_nonce || device_nonce` 的 SHA-256。使用自製私鑰簽署，桌面端用對應公鑰驗證。
   這只驗證裝置身分，**不代表 USB 流量加密，也不驗證電腦身分**。
3. **裝置資料**：讀寫名稱、讀取序號、52-byte `DeviceProfile` 讀寫、keep-alive。
   韌體不提供試用、付費啟用、原廠 OTA；協定中既有的 activation 狀態只是相容性欄位，
   自製韌體需清楚標示自己的名稱與版本。
4. **BLE HID**：鍵盤、修飾鍵、滑鼠位移、按鈕與滾輪；先固定單一 iPad，之後再設計多裝置。
   需處理配對、bond 儲存、斷線、解除配對，以及斷線或主機失聯時釋放所有按鍵。
5. **指標跨屏限制**：目前電腦端送的是相對滑鼠位移，不能讀回 iPad 真實游標位置。
   虛擬螢幕座標與 iPad 游標可能因加速度或觸控而偏離；雙向邊緣切換仍需實機驗證。

不要先燒錄原供應者的試用韌體。上游說明其 Secure Boot／flash encryption
會永久限制開發板可執行的韌體；開發時應使用仍可自由刷寫的板子。

## 驗證

```powershell
pwsh -File devtools/Test-BridgeIdentity.ps1
cmake -P devtools/Test-CustomBridge.cmake
```

第一項會驗證產生的私鑰能簽署、匯出的公鑰能驗證、錯誤訊息無法通過驗證，
並確認再次執行不會覆寫既有身分。第二項測試自製模式的設定守門條件。
這些測試不代表桌面程式已編譯或橋接硬體已驗證。

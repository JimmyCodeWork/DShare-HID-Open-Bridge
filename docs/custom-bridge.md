# 自製 ESP32-C3 橋接開發版

此 Fork 以 **ESP32-C3 Supermini + 自行實作的韌體**為目標。沿用 DShare-HID
的電腦端、螢幕配置與公開的 USB 通訊格式；不使用原供應者的付費韌體。
原專案版權、授權及來源資訊保留。新增及修改內容日期：2026-11-13。

## 目前狀態

- 已加入預設開啟的 `DSH_CUSTOM_BRIDGE` 建置選項。
- 自製橋接模式從使用者目錄讀取 P-256 公鑰，不會退回原作者的內建公鑰。
- 自製模式不載入 `esp32-hid-tools` 子模組，因此不建置其下載、燒錄、付款介面。
- 已加入 ESP32-C3 實驗版韌體，使用測試金鑰交叉編譯通過；尚未完成桌面整體建置、燒錄或 iPad 實機驗證。

這仍是實驗版，不能將編譯成功視為已驗證可用。

韌體的 Windows 建置及燒錄步驟見 [韌體說明](../firmware/open-bridge/README.md)。

## 裝置身分與桌面建置

先依 [上游建置說明](dev/build.md)準備桌面程式的依賴套件，再以
`-DDSH_CUSTOM_BRIDGE=ON` 建置。桌面程式不再需要編譯時提供公鑰；執行時從
使用者家目錄 `.dshare-hid/bridge-keys/` 讀取受信任的 65-byte P-256 公鑰。
該目錄沒有有效公鑰時，安全握手會失敗，不會退回原作者公鑰。

[韌體說明](../firmware/open-bridge/README.md)提供目前的命令列設定流程：每次
產生新金鑰並燒錄後，先以新公鑰驗證板子的簽章，再依裝置序號原子替換該板子的
公鑰。桌面「檔案 → Set up open bridge」介面可執行同一流程；需先安裝 Python
及相關套件，整體桌面程式和操作流程尚未經實機驗證。私鑰只放在臨時建置
目錄和板子；正常退出時刪除臨時目錄，但無法保證儲存媒體上的資料已安全抹除。

如需編譯未修改的上游模式，使用獨立建置目錄並明確指定
`-DDSH_CUSTOM_BRIDGE=OFF`。原廠韌體仍受其自身條款限制。

## 自製韌體使用的公開介面

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

第一項測試舊版手動身分工具。第二項確認自製模式使用執行時金鑰且不載入原廠工具。
這些測試不代表桌面程式已編譯或橋接硬體已驗證。

# Omnifix 0.8.0 — helesta ver. (unofficial fork)

Based on aixxe/omnifix commit `08c652060c3c07787a77847f5877746b8b6d3400`
(2026-09-18, Update game compatibility), retrieved 2026-09-21.
This is a new integration of the previous Helesta 0.7.2.4 features into
the upstream 0.8.0 source, not an official release by aixxe.
Original credits and LICENSE are preserved.

## v4 發佈版（關閉診斷 log）

以 v4 為基底，保留已測試的 RDI 歌名修正及原本的 live version 80 策略。
不包含 v5 保留原始 version 的搜尋實驗，因此仍有先前的搜尋限制。

48 處 INFINITAS 測試訊息在編譯時關閉，任何 log level 均不會輸出。
包含 hook 位址、MID／RGB 診斷、layout／route、命中次數、分類插入、
燈號重算、live-entry 對照及正常 version 改寫統計。
保留模組初始化，以及失敗、功能無法啟用、資源缺失等必要訊息。
官方 omnifix 與其他模組的 log 設定維持原狀。

原始碼比對確認，移除診斷巨集定義並將診斷呼叫還原後，
`src/omni_folder.cc` 與 v4 原始包逐位元組相同。
本包為原始碼，尚未在此環境編譯完整 Windows DLL。
下方 v4 驗證文件中的診斷 log 範例僅供歷史參照，本發佈版不會輸出。

## 使用方式

INFINITAS 功能預設關閉。加入以下獨立啟動參數才開啟：

```text
--omnifix-enable-inf-folder
```

包含 INFINITAS 資料夾、歌名顏色、版本與圖像／語音處理、Lightning
副螢幕處理、官方 LV → 歌名排序，以及避免 EVENT 分類出現重複資料夾的修正。
保留 MID 32761 的例外納入。顏色 hook 依 IIDX 33／34 特徵選擇。
80th folder 模組 log 全部使用 misc，移除 phase 編號。
仍需安裝既有的 ss_80th、so_80th、ssm_80th 等資源；本包不含遊戲素材。

隱藏 MID 尾三位為 500～999 的歌曲：

```text
--omnifix-disable-old-charts
```

在 MDB 讀取後，把符合條件歌曲的全部 10 個 SP／DP 難度欄位設為 0，
只修改記憶體，不改原始檔案。這項功能不需要啟用 INFINITAS；兩個參數
一起使用時，符合條件的歌曲也不會加入 INFINITAS 資料夾。
例如 MID 32761、33801 都符合隱藏規則。此選項預設不啟用。
`--omnifix-disable-omnimix` 會一併略過上述兩項功能。

保留細項開關：`--omnifix-disable-80th-phase2`、
`--omnifix-disable-80th-subscreen`、`--omnifix-disable-80th-extended-title`。

啟動畫面：

```text
OMNIFIX : 0.8.0 (helesta ver. - unofficial fork)
```

## 保留的官方更新

- 0.8.0 的 IIDX 34 MDB buffer 特徵搜尋與相容性處理。
- Clear／full-combo rate hook 從指令讀取實際 stack offset。
- XRPC response buffer 擴充至 16 MiB，保留官方
  `--omnifix-disable-xrpc-response-buffer` 選項。
- 原有路徑選項、metadata、解鎖與其他官方功能。

移植時額外保留既有 MSVC 標頭相依性修正及 x64-windows-static CRT 設定。
未修改 graphics-data 路徑機制或其 12 字元限制。

## 編譯與驗證

使用 Visual Studio 2022 C++、CMake 3.31+ 和 vcpkg，在 PowerShell 中執行：

```powershell
cmake -S . -B build -A x64 -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
```

產物為 `build/Release/omnifix.dll`。VCPKG_ROOT 需指向已安裝的 vcpkg。
替換 DLL 後重新啟動遊戲。

已驗證 old-chart C++ 測試（含 AddressSanitizer／UndefinedBehaviorSanitizer；
環境不支援 leak 檢查，因此停用 leak 檢查），以及提供的 IIDX 33／34 DLL
中的歌名顏色 hook、RGBA calls 和 route merge 位置。
結果見 DUAL_VERSION_SCAN_VALIDATION.json；測試原始碼在 tests/。
未在此環境編譯完整 Windows DLL，亦未做實機測試。新增功能驗證範圍為
提供的 33／34 DLL，不代表所有歷史 datecode 均已驗證。

實機請確認：預設 INFINITAS 關閉；enable 後出現單一版本資料夾且有歌名
顏色；disable-old-charts 後目標歌曲消失；兩選項同開時沒有歌曲被加回；
clear rate、登入與選曲正常。

SP 與 DP 使用同一個 INFINITAS 分類 ID（980）。切換模式時保留資料夾開啟
狀態，並按目前模式選取該模式的歌曲、資料夾定義與通關燈號。

IIDX 34 的遊玩／結算 `text_mu` renderer 會先從 `[RDI]` 讀取 live song
entry，再檢查版本欄位。對應的顏色 hook 已修正為同樣解參照後再比對
INFINITAS MID；否則該路徑把 RDI 容器地址誤當歌曲 entry，導致歌名不變色。
此外，遊玩／結算 renderer 會把原生 Version 材質 route 設為 2；現在也會在
共用 route merge 處只對 INFINITAS 歌曲改回預設材質 route 1，避免原生版本
材質蓋掉紫色 RGB。最新版另在同一合併點直接寫入 renderer 最終樣式記錄的
RGB 欄位，並保留 alpha 淡入值；只有原生特殊顏色優先旗標未啟用時才覆寫。
這是針對完整 log 已證明「建構參數已變紫、實際畫面仍未變色」所加的修正。
其樣式記錄欄位依提供的 IIDX 33／34 DLL 靜態反組譯確認，但尚未編譯完整
DLL 或實機測試。

### v4：補上 IIDX 34 的 RDI 共用歌名路徑

`log(20261016-090433).txt` 證明 v3 對 MID 31255 執行了 RGB 寫入，但畫面
仍未變色。進一步比對 1013 DLL，發現另一個共用歌名函式在
RVA `0x81ED42` 以 `[RDI+0x3DC]` 檢查版本；此前掃描器只匹配 RBX，
因此漏接整個函式。IIDX 33 的兩個共用函式都用 RBX，IIDX 34 則為
RDI／RBX 各一個。v4 掃描兩種完整特徵，分別記錄歌曲指標寄存器，
在 RGB 建構與樣式合併兩種 callback 中使用正確的歌曲指標。

已用實際來源中的掃描器與 callback 抽出編譯測試，通過四份 DLL 的掃描、
指標選擇、alpha 保留及特殊顏色優先判斷。詳見
`TITLE_COLOR_V4_VALIDATION.md`。這不是完整 Windows DLL 編譯或實機驗證。

此變更已核對提供的 IIDX 33／34 DLL：兩者都在 active category ID 的
`+0x04` 保存目前 SP／DP 索引（ID 位移為 `0x29860`，模式位移為
`0x29864`）。現有 old-chart 測試通過。此環境沒有 CMake／Windows 編譯器，
尚未編譯 DLL 或進遊戲測試；實機需確認從 SP 資料夾切到 DP、再切回 SP
時都留在資料夾內，離開後重新進入則不會殘留舊的開啟狀態。

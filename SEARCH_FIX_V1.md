# v4 + INFINITAS search category fix v1 (experimental)

基底：v4-release。保持記憶體 version 80、錄影／歌名顏色、980 SP/DP 共用資料夾及 old charts 功能。

## 已定位的原因
IIDX34 的歌名與作者字典重建函式從 MusicItem+8 讀取 MID，計算 MID / 1000 + 26 作為資料夾 ID。檢查五個譜面選項後，呼叫 category lookup，再遍歷資料夾歌曲列確認 MID。移往其他 version 或 INFINITAS 資料夾的歌會因原始資料夾沒有該曲而被排除。
1013: artist rebuild 0x7DD870, title rebuild 0x7DDBF0; category selection hooks at RVA 0x7DD94B and 0x7DDCCB. Membership check 0x5EE0B0.

## 修改
只對 INFINITAS 白名單 MID，在上述兩個搜尋函式中把 R14D（搜尋檢查用資料夾 ID）改為設定的 category_id（預設980）。不改 MID、version、XML 或全域類別查找規則。沿用原生難度／可選條件及結果插入處理。
只在偵測為 IIDX34 且兩處特徵都匹配時安裝。IIDX33 保留 v4 原有行為。安裝失敗時回退既有搜尋行為，不停用整個資料夾。此版本不修正非白名單歌曲的任意 version/MID 不一致。

## 使用
Windows 重新編譯並替換 omnifix.dll，保留 --omnifix-enable-inf-folder。不需要額外參數。不使用 v5 原始碼，也不要停用額外歌名顏色。
啟動 misc log: 80th folder: search category fix v1 enabled (title/artist)
沒有逐首搜尋或每幀診斷 log；其他 v4 測試 log 仍關閉。

## 測試狀態
已靜態驗證 IIDX34 初版／0916／1013 各有且只有兩個特徵匹配；33 沒有匹配且版本門檻會跳過。
已用實際 callback 的抽取程式測試白名單／普通歌曲／無 runtime／無效指標的處理，通過 ASan/UBSan（環境不支援 LeakSanitizer，關閉 leak 檢查）。
未在此環境編譯完整 Windows DLL，未做遊戲實測，因此是測試版。

請優先測試：34327 以 DIN DON DAN 搜尋、作者搜尋；31255 舊版 INF；普通官方曲；SP/DP 切換後搜尋；34327 保存錄影歌名。測試需重新啟動遊戲，以重建搜尋字典。

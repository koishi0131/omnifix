# MDB buffer getter fix for 2026102700

Based on helesta v4-search-fix-v1a, matching the search-v1 initialization in the supplied log. Existing INFINITAS/version-80, title, recording, old-chart and search-v1 code is retained. Search-v1 is still experimental and previously did not resolve searching.

1027 introduces a second match for `E8 ? ? ? ? 85 DB 78`. The old first-match scan modifies RVA 0x64E3C1 (89 -> 8B), an unrelated function prologue, and leaves the actual MDB getter at RVA 0x924D20 unchanged.

This fix reuses the getter already resolved from the MDB loading function, validates `48 8D 05 disp32 C3` before buffer mutations, and patches that getter's opcode at RVA 0x924D21 (8D -> 8B). The 6 MiB buffer and song-count/file-size patches are unchanged.

Static getter resolution passed on supplied IIDX33, IIDX34, 0916, 1013 and 1027 binaries. No full Windows build or game execution was performed here.

Build using your existing VS2022/CMake/vcpkg procedure and replace omnifix.dll while the game is closed, then restart the game. Do not hot-swap into the already corrupted process. The ZIP contains source, not a prebuilt DLL. To apply only this fix to another v4 tree, use MDB_1027_FIX.patch or copy the corresponding getter changes in src/omnifix.cc.

The log separately reports 2dx-gsm.dll initialization error 1114; this source change does not update 2dx-gsm.

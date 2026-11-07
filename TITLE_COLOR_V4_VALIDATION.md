# Title color v4 validation

## Confirmed issue

The v3 runtime log reports MID 31255 at shared layout 0, with final RGB written
and native style mode 0. That log only covers the RBX renderer. It does not
establish that every title path was hooked.

In the supplied 1013 DLL, the shared function beginning at RVA 0x81EC60 has a
version comparison at RVA 0x81ED42 using RDI. It has five direct call sites:
0x6A142C, 0x725F42, 0x81CD89, 0x81D0AD, and 0x8ECE94.
The former scanner matched only the RBX version-comparison instruction and
missed this function. No screen-specific labels are inferred from addresses alone.

## Change

Match both RBX and RDI comparison anchors, sort them by address, and retain the
entry register for each callback slot. The entry register and style record
register are separate: the newly discovered RDI entry path uses RSI for its
style record. Both constructor and merge callbacks use the selected entry.

## Executed checks

The scanner, route-merge decoder, version detection, and modified callbacks were
extracted directly from src/omni_folder.cc into a Linux C++ harness. Platform
logging, memory-query and runtime dependencies were stubbed. Compiled with
GCC 13.3, C++23, warnings as errors, AddressSanitizer and UndefinedBehaviorSanitizer.
Leak detection was disabled. The actual scanner ran on each supplied DLL's .text.

| DLL | Shared entry registers | Shared merge RVAs | text_mu merge RVA |
| --- | --- | --- | --- |
| bm2dx33.dll | RBX / RBX | 0x832695 / 0x832910 | 0xA2F440 |
| bm2dx34.dll | RDI / RBX | 0x81EDA1 / 0x81EFBC | 0xA12C47 |
| bm2dx_0916.dll | RDI / RBX | 0x81EDA1 / 0x81EFBC | 0xA12C47 |
| bm2dx_1013.dll | RDI / RBX | 0x81EDD1 / 0x81EFEC | 0xA12587 |

All four passed. Callback checks verified the selected entry register, retained
alpha, and no RGB overwrite when the native priority flag is set.

This does not compile or exercise Windows SafetyHook or the game's renderer.
Visual correctness remains unverified until tested in game.

## Expected 1013 startup evidence

- shared renderer 0 anchor=0x18081ed42, entry=RDI
- shared renderer 1 anchor=0x18081ef5d, entry=RBX

These addresses assume the image base 0x180000000 shown in the supplied log.
Runtime constructor diagnostics use layouts 5 and 6 for shared slots 0 and 1;
final-style diagnostics use layouts 0 and 1. Successful writes alone do not
prove the visible result; compare gameplay, results, and the secondary screen.

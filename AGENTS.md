# AGENTS.md

## Project context
This is an ESP32 windsurfing GPS logger using:
- LilyGO T5 e-paper ESP32 board
- u-blox GNSS
- SD_MMC / eMMC storage in 1-bit mode
- LittleFS fallback
- WiFi/AP mode for file access and config
- RTOS tasks for GPS/logging and display updates

## Refactor goal
Refactor and clean up the codebase so it is easier to maintain, safer to update, and more professional, without changing existing behaviour unless explicitly requested.

## Hard rules
- Do not rewrite the whole project from scratch.
- Make small, reviewable commits/patches.
- Preserve existing behaviour and config compatibility.
- Do not change GPS speed/stat calculation logic unless asked.
- Do not change e-paper screen behaviour unless asked.
- Do not change file formats: GPY, GPX, SBP, UBX, TXT.
- Do not change existing config keys or meanings.
- Avoid large architectural rewrites in one step.
- Prefer clear modules, small functions, and explicit pin definitions.
- Keep Arduino / PlatformIO compatibility.



## Refactor priorities
1. Improve setup() readability without changing sequence.
2. Separate GPS, storage, display, WiFi/AP, config, and sleep logic.
3. Remove duplicated or dead code only after confirming it is unused.

## Workflow
- Start in Plan mode.
- First inspect the codebase and identify modules/files.
- Produce a staged refactor plan before editing.
- For each stage, explain the risk and expected behaviour.
- After each change, run build/checks if available.
- Keep changes minimal and reversible.
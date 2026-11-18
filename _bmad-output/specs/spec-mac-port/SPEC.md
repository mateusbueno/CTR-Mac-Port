---
id: SPEC-mac-port
companions:
  - architecture-diagrams.md
  - stack.md
sources:
  - README.md
  - README.txt
  - docs/MEMORY_MODEL.md
---

> **Canonical contract.** This SPEC and the files in `companions:` are the complete, preservation-validated contract for what to build, test, and validate. Source documents listed in frontmatter are for traceability — consult them only if you need narrative rationale or prose color this contract intentionally omits.

# Port CTR Native to macOS Apple Silicon

## Why

Crash Team Racing Native (CTR-Native) currently only targets 32-bit x86 Linux and Windows environments through GCC/MinGW with explicit 32-bit pointer and fixed virtual memory assumptions (`-m32`, hardcoded `0x1f800000` scratchpad mapping, and 24-bit pointer truncation). macOS running on modern Apple Silicon (including the user's MacBook Air M-series) does not support 32-bit execution, enforces a 4 GiB protected `__PAGEZERO` that rejects lower-memory mapping, and uses Apple Clang with Mach-O linkers. Porting CTR-Native to macOS Apple Silicon realizes the vision of running CTR natively at full speed and 60 FPS without emulation layers on modern Mac hardware.

## Capabilities

- **CAP-1**
  - **intent:** Build system compiles CTR-Native as a native 64-bit Mach-O (`arm64`) executable and packages both a standalone CLI binary and a macOS Application Bundle (`CTR Native.app`).
  - **success:** Running `./build_mac.sh` (or `cmake --build build`) on Apple Silicon generates both `build/ctr_native` and `build/CTR Native.app` with zero compilation or linker errors.

- **CAP-2**
  - **intent:** Memory subsystem provides a decoupled scratchpad backing store for PS1 scratchpad emulation without fixed low-memory virtual address dependencies.
  - **success:** `Platform_InitScratchpad` initializes without calling `mmap(MAP_FIXED)` at `0x1f800000`, and all game engine scratchpad read/write operations execute against the allocated storage without memory access violations.

- **CAP-3**
  - **intent:** GPU ordering-table and primitive linking subsystem supports 64-bit address spaces while maintaining 24-bit OT tag compatibility.
  - **success:** `CtrGpu_PrimToOTLink24` and ordering-table traversal process all frame primitives without encountering 24-bit address overflow panics or pointer truncation faults.

- **CAP-4**
  - **intent:** Windowing, audio streaming, and controller input initialize and operate through native macOS backends via SDL3.
  - **success:** The game window displays with an active OpenGL 3.3 Core profile context on macOS, plays stereo music and sound effects through CoreAudio, and responds to keyboard and connected GameController inputs.

- **CAP-5**
  - **intent:** Game engine loads retail disc assets and runs gameplay loop on 64-bit Apple Silicon without data-alignment or struct-offset crashes.
  - **success:** With retail assets placed in `assets/`, the title sequence boots, adventure/arcade track loads, and a player can finish a full 3-lap race on Crash Cove with accurate physics and audio at 60 FPS.

## Constraints

- Executable must be native 64-bit ARM (`arm64`); macOS does not support 32-bit binaries (`-m32`) or x86 SSE compiler intrinsics on Apple Silicon.
- macOS Mach-O linkers reject GNU ld arguments (`--image-base`, `-no-pie`, `--allow-multiple-definition`, `-static-libgcc`); Darwin linker invocations must link system frameworks dynamically.
- `__PAGEZERO` on macOS arm64 prohibits virtual memory mapping below 4 GiB; all raw literal `0x1f800...` pointers in game source must be routed through `CTR_SCRATCHPAD_PTR`.
- Retail PS1 asset formats (`BIGFILE.BIG`) and fixed binary structures must remain uncorrupted by 64-bit compiler struct padding.
- Mod scanning resolves mods from both the relative `./mods` directory and `~/Library/Application Support/CTR-Native/mods`.

## Non-goals

- Native Metal or Vulkan backend rewrite (OpenGL 3.3 Core Profile is supported by Apple's OpenGL framework and is the single graphics backend target).
- Support for iOS, iPadOS, tvOS, or watchOS platforms.
- Emulation or runtime translation wrappers (e.g., Box64, FEX, Wine).
- Overhaul of the Mod SDK or Netplay architecture beyond 64-bit cleanliness.

## Success signal

Running `./build_mac.sh` on an Apple Silicon Mac produces `build/ctr_native`, which launches the game, loads `assets/BIGFILE.BIG`, reaches the main menu, and runs a complete 3-lap race at 60 FPS with functional sound and controller input.

## Assumptions

- Host machine has Xcode Command Line Tools installed with Apple Clang and CMake.
- User provides valid retail CTR NTSC-U disc assets in the `assets/` folder.
- Apple OpenGL 3.3 Core Profile remains available in the active macOS release.

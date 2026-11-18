# Technology Stack & Porting Requirements

## Host Platform Details

- **OS:** macOS Darwin (macOS 14+ / Sonoma, Sequoia, and future versions)
- **Target Architecture:** `arm64` (Apple Silicon M-Series: M1 through M5)
- **Compiler:** Apple Clang (Xcode / Command Line Tools) targeting C99
- **Linker:** Apple Mach-O `ld64`
- **Build System:** CMake (>= 3.20) + Unix Makefiles or Ninja

## Key Differences from Linux/Windows 32-bit

| Subsystem | Linux / Windows (x86_32) | macOS Apple Silicon (`arm64`) | Porting Action Required |
|---|---|---|---|
| **Pointer Size** | 32-bit (`sizeof(void*) == 4`) | 64-bit (`sizeof(void*) == 8`) | Remove `-m32`, audit pointer asserts, handle 64-bit pointer alignment |
| **Scratchpad RAM** | `mmap` fixed at `0x1f800000` | Blocked by 4GB `__PAGEZERO` | Allocate `s_scratchpadMemory[4096]`, route all raw pointers via `CTR_SCRATCHPAD_PTR` |
| **OT 24-bit Links** | Lower 24 bits of pointer | Pointers exceed 24 bits (`> 0x100000000`) | Implement arena-relative offset tagging in `CtrGpu_PrimToOTLink24` |
| **SIMD Flags** | `-msse` | ARM NEON (SSE is x86 only) | Remove `-msse` on Apple Silicon |
| **Linker Flags** | GNU ld: `-Wl,--image-base`, `-no-pie` | Mach-O ld: requires PIE, dynamic libSystem | Strip GNU ld flags on `APPLE`, link Cocoa/OpenGL frameworks |
| **Graphics API** | OpenGL 3.3 Core (Mesa / Win32) | OpenGL 3.3 Core (Apple OpenGL framework) | Link `-framework OpenGL -framework Cocoa -framework IOKit` via SDL3 |
| **Audio API** | ALSA / PulseAudio / WASAPI | CoreAudio via SDL3 | Enable SDL CoreAudio / Darwin audio driver |
| **Input API** | evdev / XInput | GameController & IOKit via SDL3 | Enable SDL Darwin joystick/haptic drivers |

## Build Artifacts

- **Primary Binary:** `build/ctr_native` (Mach-O 64-bit arm64 executable)
- **Optional Dedicated Server:** `build/ctr_server`
- **Build Script:** `build_mac.sh`

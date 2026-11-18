# CTR Native

A native PC port of Crash Team Racing (PS1, 1999), built on top of the [CTR-ModSDK](https://github.com/CTR-tools/CTR-ModSDK) decompilation project BUT with mod support and more extras

## Philosophy

- **No byte budget.** Game source lives in `game/` as our own copies. Edit freely.
- **No PSX toolchain.** Targets Windows, Linux, and macOS (Apple Silicon arm64) with SDL3. No MIPS compiler needed.
- **Clean platform layer.** `main.c` owns process startup; host details stay in `platform/native_*`.
- **No build system nonsense.** Just `build.bat` / `build.sh` / `build_mac.sh`.
- **Fully static build.** Single executable, zero dependencies. SDL3 is compiled from vendored source and linked statically.

## Directory Layout

```
ctr_native/
  main.c              Entrypoint and unity include manifest
  platform.h          Platform API the game calls through
  platform/           Native-owned audio, input, memcard, CD, and PSX facade glue
  game_includes.h     Ordered include chain for all game source files
  build.bat           Windows build (MinGW32)
  build.sh            Linux build
  build_mac.sh        macOS Apple Silicon build (Clang arm64 + App Bundle)
  README.md           This file
  game/               Our copies of all decompiled game source (943 files)
  include/            Project headers (structs, globals, declarations)
  externals/
    SDL/              SDL3 source (static build)
```

## Prerequisites

### macOS (Apple Silicon M-Series)

1. Install Xcode Command Line Tools (if not already installed):
   ```bash
   xcode-select --install
   ```
2. Install CMake (via [Homebrew](https://brew.sh)):
   ```bash
   brew install cmake
   ```

### Windows

1. Install [MSYS2](https://www.msys2.org/)
2. In an MSYS2 terminal:
   ```
   pacman -S mingw-w64-i686-gcc mingw-w64-i686-tools-git
   ```
3. Add `C:\msys64\mingw32\bin` to your system PATH

That's it. SDL3 is compiled from vendored source -- no separate install needed.

### Linux (Debian/Ubuntu)

```
sudo apt install gcc-multilib
sudo apt install libx11-dev libxext-dev libgl1-mesa-dev libasound2-dev libudev-dev libdbus-1-dev
```

## Building

```bash
# macOS (Apple Silicon M1/M2/M3/M4/M5, 64-bit arm64)
chmod +x build_mac.sh
./build_mac.sh

# Windows (MinGW32)
build.bat

# Linux
chmod +x build.sh
./build.sh
```

First build compiles SDL3 from source. This is cached as a static library in `build/` -- subsequent builds only recompile touched native sources.

Outputs:
- **macOS:** `build/ctr_native` (CLI binary) and `build/CTR Native.app` (macOS Application Bundle)
- **Windows:** `build/ctr_native.exe`
- **Linux:** `build/ctr_native`

### Clean build

```
rm -rf build/        # Delete cached libraries
build.bat            # Rebuild everything
```

## Running

### Extração Automática de Assets (.bin/.cue)

Você pode extrair automaticamente todos os 33 arquivos necessários a partir de uma imagem de disco NTSC-U (`.bin`/`.cue`) usando o script incluído:

```bash
python3 tools/extract_disc.py "caminho/para/CTR.bin" "$HOME/Library/Application Support/CTR-Native/assets"
```

O script detecta o sistema de arquivos ISO9660, extrai todos os setores de áudio XA (mantendo os cabeçalhos de 2336/2352 bytes), o `BIGFILE.BIG` e arquivos de som `KART.HWL`.

1. Create an `assets/` directory next to the executable for packaged builds, or
   next to the source files for development builds run from `build/` (ou use a pasta global no macOS: `~/Library/Application Support/CTR-Native/assets`)
2. Extract the following from a CTR NTSC-U retail disc image:
   - `BIGFILE.BIG`
   - `SOUNDS/KART.HWL`
   - `TEST.STR`
   - `XA/ENG.XNF`
   - `XA/ENG/EXTRA/S00.XA` through `S05.XA`
   - `XA/ENG/GAME/S00.XA` through `S20.XA`
   - `XA/MUSIC/S00.XA` through `S01.XA`
3. Run the game:
   - **macOS:** Double click `build/CTR Native.app` or run `./build/ctr_native` in terminal
   - **Windows:** Run `build/ctr_native.exe`
   - **Linux:** Run `./build/ctr_native`

On macOS, assets are automatically detected in any of these locations:
- `./assets` (repository root or current working directory)
- `build/CTR Native.app/Contents/Resources/assets` (inside the app bundle)
- `~/Library/Application Support/CTR-Native/assets`

### Mods on macOS

Lua and file override mods are supported and scanned from both:
- `./mods` (relative to the executable / repository)
- `~/Library/Application Support/CTR-Native/mods`

Packaged directory structure:
```
CTR-Native/
  ctr_native.exe
  assets/
    BIGFILE.BIG
    SOUNDS/KART.HWL
    TEST.STR
    XA/
      ENG.XNF
      ENG/EXTRA/S00.XA ... S05.XA
      ENG/GAME/S00.XA ... S20.XA
      MUSIC/S00.XA ... S01.XA
```

Development directory structure:
```
ctr_native/
  build/
    ctr_native.exe
  assets/
    BIGFILE.BIG
    SOUNDS/KART.HWL
    TEST.STR
    XA/
      ENG.XNF
      ENG/EXTRA/S00.XA ... S05.XA
      ENG/GAME/S00.XA ... S20.XA
      MUSIC/S00.XA ... S01.XA
```

## Bug Replays

Internal builds can record a small bug report folder. See `docs/REPLAYS.md`.

## Netplay

CTR-Native soporta partidas online de hasta 8 jugadores vía UDP, con
sincronización de items, chat en lobby, y selección de interfaz de red
100% desde el menú ingame (sin tocar la consola). Ver
[`docs/NETPLAY.md`](docs/NETPLAY.md) para el protocolo completo.

### Flujo ingame (recomendado)

1. Ejecutá `ctr_native.exe` (sin argumentos).
2. Main menu → **Online**.
3. Elegí **Host** o **Connect**.
   - Host: te aparece la lista de interfaces de red con sus IPs. Elegís la
     que vas a compartir con los demás.
   - Connect: tipeás la IP del host con el teclado virtual onscreen.
4. Lobby: `SQR` para ready. El chat se escribe en una **ventana de consola
   separada** que se abre automáticamente (con tu teclado de PC, no con
   el controller). El host arranca con `START` cuando todos están ready.
5. Al terminar, el host puede mandar "volver al lobby" para jugar otra
   pista sin reconectar.

### CLI (opcional, para debug)

```
ctr_native.exe --host --name "Host" --players 4
ctr_native.exe --connect 192.168.1.100 --name "Player2"
ctr_native.exe --list-interfaces
```

## Architecture

```
main.c (entrypoint)
  |
  +-- platform/native_* (platform shell, audio, input, memcard, CD, renderer, PSX facade glue)
  |
  +-- game_includes.h
        |
        +-- game/ (all decompiled game source)
              |
              +-- include/ (headers: structs, globals, declarations)
```

- `CTR_NATIVE` is defined for native host/platform-specific code
- **64-bit Native Apple Silicon Port:** On macOS (`arm64`), the engine runs natively in 64-bit mode:
  - **Arena-Relative GPU Link Bridge:** Solves the PS1 24-bit Ordering Table (OT) pointer constraint by encoding links as offsets relative to the 8 MiB MEMPACK arena (`g_gpuArenaBase`), resolving full 64-bit host pointers without memory corruption or truncation.
  - **LP64 Relocation & Pointer Extension:** Restores upper 32-bit `base_hi` address bits across all binary assets loaded in DRAM (`ModelAnim` frame arrays, `AnimTex` texture cycling arrays, `IconGroup` icon collections, texture UV layouts, and level geometry lookups).
  - **Native Icon Table Cache (`DecalGlobal_GetIcons`):** Dynamically caches and expands packed 32-bit icon pointer offsets into native 64-bit arrays, enabling wheel sprite rendering, HUD wumpa/speedometer digits, and font decals.
  - **Thread & Object Lifecycles:** Eliminates 32-bit truncation of function pointers and thread ticks (`UI_ThTick_*`), ensuring stable 60 FPS execution throughout race physics and UI animation loops.
  - **Decoupled PS1 Scratchpad:** Avoids Mach-O 4 GiB `__PAGEZERO` collision by routing scratchpad accesses through engine-managed host memory via `CTR_SCRATCHPAD_PTR`.
  - **Modern macOS Frameworks:** Statically links SDL3 Cocoa windowing, OpenGL 3.3 Core Profile, CoreAudio sound streaming, and Apple GameController / HIDAPI gamepads (MFi, DualSense, DualShock 4, Xbox Series, Switch Pro).

## Roadmap

- Clean up `game/` copies strip byte budget hacks and route platform-specific code through `CTR_NATIVE`
- Expand the 64-bit arena-relative GPU link bridge to Linux and Windows x86_64 builds.
- Keep pruning inherited compatibility code now owned in `include/` and `platform/`.

## Credits

- [CTR-ModSDK](https://github.com/CTR-tools/CTR-ModSDK) — the decompilation project this is built on
- [PsyCross](https://github.com/OpenDriver2/PsyCross) — original PS1 compatibility code from which parts of CTR Native's owned platform layer and PsyQ facade headers are derived
- [SDL3](https://github.com/libsdl-org/SDL) — cross-platform multimedia
- Crash Team Racing is a trademark of Sony Computer Entertainment / Naughty Dog
- Faradise Team for the principal fork and more help
- Aalhendi Code Base 
- LoloTheSamurai for testing
- TDYU for testing
- Yostifu for testing
- CTR Community

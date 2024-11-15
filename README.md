# TinyCircuits-Tiny-Game-Engine
This is a 2D/3D game engine for embedded devices that can run Python/MicroPython.

# Building on Linux
These instructions assume that you are cloning MicroPython from the TinyCircuits Fork that has been pre-modified
1. Install SDL2: `sudo apt install libsdl2-dev`
2. Install FFI: `sudo apt install libffi-dev`
3. Install build tools: `sudo apt install build-essential`
4. Clone TinyCircuits MicroPython: `git clone https://github.com/TinyCircuits/micropython.git mp-thumby`
5. `cd` into MicroPython: `cd mp-thumby`
6. Checkout engine branch: `git checkout engine-1.23.0`
7. Clone latest engine module code with `lib` submodules: `git clone --recurse-submodules https://github.com/TinyCircuits/TinyCircuits-Tiny-Game-Engine.git`
8. Setup UNIX port:
   1. `cd ports/unix`
   2. `make submodules`
9. `cd` to engine file system to build and run MicroPython and the engine
   1. `cd`: `cd ../../TinyCircuits-Tiny-Game-Engine/filesystem`
   2. build: `(cd ../../ports/unix && make -j8 USER_C_MODULES=../../TinyCircuits-Tiny-Game-Engine DEBUG=1)`
   3. run: `../micropython_loop ../../ports/unix/build-standard/micropython -X heapsize=532480 main.py`

Use `(cd ../../ports/unix && make clean)` to make clean if needed

# Building on Linux, from Scratch
These instructions assume that you are cloning MicroPython from the official MicroPython repository and not the TinyCircuits Fork

1. Install SDL2: `sudo apt install libsdl2-dev`
2. Install FFI: `sudo apt install libffi-dev`
3. Install build tools: `sudo apt install build-essential`
4. Clone MicroPython: `git clone https://github.com/micropython/micropython.git`
5. `cd` into MicroPython: `cd micropython`
6. Reset to MicroPython version 1.23.0: `git reset --hard a61c446`
7. Clone latest engine module code with `lib` submodules: `git clone --recurse-submodules https://github.com/TinyCircuits/TinyCircuits-Tiny-Game-Engine.git`
8. Setup UNIX port:
   1. `cd ports/unix`
   2. `make submodules`
9. Inside `micropython/ports/unix/variants/mpconfigvariant_common.h` do:
   1. Change `#define MICROPY_FLOAT_IMPL (MICROPY_FLOAT_IMPL_DOUBLE)` -> `#define MICROPY_FLOAT_IMPL (MICROPY_FLOAT_IMPL_FLOAT)`
   2. Change `#define MICROPY_LONGINT_IMPL (MICROPY_LONGINT_IMPL_MPZ)` -> `#define MICROPY_LONGINT_IMPL (MICROPY_LONGINT_IMPL_LONGLONG)`
   3. Add `#define MICROPY_TRACKED_ALLOC (1)` anywhere
10. UGLY: inside `micropython/tools/mpy-tool.py`
   1. Change line 1784 to `default="longlong"` (don't know where this tool is used and how to pass it a different value, will just set it for now)
11. `cd` to filesystem root: `cd micropython/TinyCircuits-Tiny-Game-Engine/filesystem`
12. Build MicroPython UNIX port: `(cd ../../ports/unix && make -j8 USER_C_MODULES=../../TinyCircuits-Tiny-Game-Engine DEBUG=1)`
13. Run MicroPython port: `../micropython_loop ../../ports/unix/build-standard/micropython -X heapsize=532480 main.py`

Use `(cd ../../ports/unix && make clean)` to make clean if needed

# Building and Running On Linux Quickly 
If you followed the one of two methods to setup everythign above, you can run the following from `micropython/TinyCircuits-Tiny-Game-Engine/filesystem` to build and run a MicroPython script on Linux quickly:

`(cd ../../ports/unix && make -j8 USER_C_MODULES=../../TinyCircuits-Tiny-Game-Engine) && (../../ports/unix/build-standard/micropython main.py)`

# Building on Linux for WebAssembly
1. Follow instructions here https://emscripten.org/docs/getting_started/downloads.html and finish after executing `source ./emsdk_env.sh` (will need to execute this last command in `emsdk` in every new terminal/session)
2. `git clone https://github.com/TinyCircuits/micropython/tree/engine micropython`
3. `cd micropython/ports/webassembly`
4. `make -j8 USER_C_MODULES=../../TinyCircuits-Tiny-Game-Engine`

# Updating MicroPython version
Make sure to check that the copied structures in src/utility/engine_mp.h are still the same in the version of MicroPython you're updating to. Some structures are not exposed so they had to be copied to where the engine can use them.

# How UF2s are made
1. Download and compile https://github.com/raspberrypi/picotool (requires pico-sdk be installed in default known location or pass `-DPICO_SDK_PATH=` to `cmake`)
2. Upload a firmware and all files that should be included to Thumby Color
3. Connect Thumby Color to computer with picotool installed in BOOTSEL mode (turn off, press and hold down DPAD direction, turn back on)
4. Run: `sudo ./picotool save -r 0x10000000 0x11000000 thumby_color_dev_kit_full_image_08_20_2024.bin` (saves from `XIP_BASE` to `16MiB`s after to bin file)
5. Run: `sudo ./picotool uf2 convert thumby_color_dev_kit_full_image_08_20_2024.bin thumby_color_dev_kit_full_image_08_20_2024.uf2`
cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(craftos-pico)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(craftos
    fs_handle.cpp
    fs_standalone.cpp
    fs.cpp
    hid.c
    main.c
    os.c
    term.c
    vga.c
    lua-apis/colors.c
    lua-apis/keys.c
    lua-apis/parallel.c
    lua-apis/vector.c
    craftos2-lua/src/lauxlib.c
    craftos2-lua/src/lopcodes.c
    craftos2-lua/src/lstate.c
    craftos2-lua/src/lobject.c
    craftos2-lua/src/lmathlib.c
    craftos2-lua/src/lvm.c
    craftos2-lua/src/lfunc.c
    craftos2-lua/src/lstrlib.c
    craftos2-lua/src/linit.c
    craftos2-lua/src/lstring.c
    craftos2-lua/src/lundump.c
    craftos2-lua/src/ltable.c
    craftos2-lua/src/ldump.c
    craftos2-lua/src/lgc.c
    craftos2-lua/src/lzio.c
    craftos2-lua/src/ldblib.c
    craftos2-lua/src/lutf8lib.c
    craftos2-lua/src/lmem.c
    craftos2-lua/src/lcode.c
    craftos2-lua/src/ltablib.c
    craftos2-lua/src/lbitlib.c
    craftos2-lua/src/lapi.c
    craftos2-lua/src/lbaselib.c
    craftos2-lua/src/ldebug.c
    craftos2-lua/src/lparser.c
    craftos2-lua/src/llex.c
    craftos2-lua/src/ltm.c
    craftos2-lua/src/ldo.c
    craftos2-lua/src/lrotable.c
)

target_include_directories(craftos PRIVATE craftos2-lua/include ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(craftos PRIVATE -Werror-implicit-function-declaration -DLUA_OPTIMIZE_MEMORY=2)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(craftos pico_stdlib pico_stdio_uart pico_multicore pico_printf tinyusb_host tinyusb_board)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(craftos)
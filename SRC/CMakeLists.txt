cmake_minimum_required(VERSION 3.20)

project(duke3dx VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile_commands.json so editors can reuse the real flags.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The original game code passes string literals to non-const char*
# parameters, which MSVC treats as an error in C++17 mode.
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/Zc:strictStrings->)

# Binaries land at the repo root next to DUKE3D.CFG, intermediates in out/.
set(DUKE3DX_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(DUKE3DX_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../out")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${DUKE3DX_BIN_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${DUKE3DX_LIB_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${DUKE3DX_LIB_DIR}")
foreach(OUT_CONFIG DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUT_CONFIG} "${DUKE3DX_BIN_DIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUT_CONFIG} "${DUKE3DX_LIB_DIR}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUT_CONFIG} "${DUKE3DX_LIB_DIR}")
endforeach()

set(DUKE3DX_SOURCES
    ACTORS.CPP
    ANIMLIB.CPP
    BUILD.CPP
    CONFIG.CPP
    GAME.CPP
    GAMEDEF.CPP
    GLOBAL.CPP
    MENUES.CPP
    PLAYER.CPP
    PREMAP.CPP
    RTS.CPP
    SECTOR.CPP
    SOUNDS.CPP
    XAUDIO2.CPP
    KEYBOARD.CPP
    CONTROL.CPP
    SCRIPLIB.CPP
)

set(DUKE3DX_HEADERS
    _ANIMLIB.H
    _FUNCTIO.H
    _RTS.H
    ANIMLIB.H
    BUILD.H
    CONFIG.H
    CONTROL.H
    DEVELOP.H
    DUKE3D.H
    FILE_LIB.H
    FUNCT.H
    FUNCTION.H
    FX_MAN.H
    GAMEDEFS.H
    KEYBOARD.H
    MOUSE.H
    MUSIC.H
    NAMES.H
    PRAGMAS.H
    RTS.H
    SCRIPLIB.H
    SNDCARDS.H
    SOUNDEFS.H
    SOUNDS.H
    TYPES.H
    UTIL_LIB.H
)

add_executable(duke3dx
    ${DUKE3DX_SOURCES}
    ${DUKE3DX_HEADERS}
)

add_executable(editor WIN32
    BUILD.CPP
    EDITOR.CPP
)

# Shared settings for both targets: the old code needs relaxed CRT
# warnings, and a few level-3 diagnostics stay suppressed until the
# modern C++ port fixes them.
function(duke3dx_target_settings TARGET)
    target_include_directories(${TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

    target_compile_definitions(${TARGET} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )

    if(MSVC)
        target_compile_options(${TARGET} PRIVATE
            /W3
            /wd4996
            /wd4244
            /wd4305
            /wd4018
            /wd4101
            /wd4700
            /wd4761
        )
    endif()
endfunction()

duke3dx_target_settings(duke3dx)
duke3dx_target_settings(editor)

if(WIN32)
    target_link_libraries(duke3dx PRIVATE
        winmm
        user32
        gdi32
        ws2_32
    )

    target_link_libraries(editor PRIVATE
        winmm
        user32
        gdi32
        ws2_32
    )
endif()

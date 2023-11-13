cmake_minimum_required(VERSION 3.27)
project(sgp)

file(GLOB_RECURSE SOURCES
    "*.c"
    "*.h"
)

# Organize source files into virtual folders in Visual Studio
foreach(source IN LISTS SOURCES)
    get_filename_component(source_path "${source}" PATH)
    file(RELATIVE_PATH source_path_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${source_path}")
    string(REPLACE "/" "\\" source_group_path "${source_path_rel}")
    source_group("${source_group_path}" FILES "${source}")
endforeach()

add_library(sgp ${SOURCES})

target_link_libraries(sgp PUBLIC "ddraw.lib" "mss32.lib" "SMACKW32.lib")
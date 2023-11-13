cmake_minimum_required(VERSION 3.27)
project(ja2)

add_executable(ja2)

include_directories("Standard Gaming Platform")
include_directories("ja2/Build")
include_directories("ja2/Build/Editor")
include_directories("ja2/Build/Laptop")
include_directories("ja2/Build/Res")
include_directories("ja2/Build/Strategic")
include_directories("ja2/Build/Tactical")
include_directories("ja2/Build/TacticalAI")
include_directories("ja2/Build/TileEngine")
include_directories("ja2/Build/Utils")

add_compile_definitions(
	JA2
	UTIL
	JA2_PRECOMPILED_HEADERS
	PRECOMPILEDHEADERS
	_CRT_SECURE_NO_WARNINGS
	WINDOWS_IGNORE_PACKING_MISMATCH
	_CRT_NON_CONFORMING_SWPRINTFS
	_STL_COMPILER_PREPROCESSOR=0
	NO_ZLIB_COMPRESSION	
)

# Set the target architecture to 32-bit
set(CMAKE_GENERATOR_PLATFORM x86)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4091")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4091")

file(GLOB_RECURSE SOURCES
	"ja2/Build/*.c"
	"ja2/Build/*.h"
)

target_sources(ja2 PUBLIC ${SOURCES})


# Organize source files into virtual folders in Visual Studio
foreach(source IN LISTS SOURCES)
    get_filename_component(source_path "${source}" PATH)
    file(RELATIVE_PATH source_path_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${source_path}")
    string(REPLACE "/" "\\" source_group_path "${source_path_rel}")
    source_group("${source_group_path}" FILES "${source}")
endforeach()

target_link_options(ja2 PRIVATE /SAFESEH:NO)
target_link_directories(ja2 PUBLIC "Standard Gaming Platform")
target_link_libraries(ja2 PUBLIC sgp)

add_subdirectory("Standard Gaming Platform")
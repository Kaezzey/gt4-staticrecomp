cmake_minimum_required(VERSION 3.24)
project(GT4Recomp VERSION 0.1.0 LANGUAGES CXX)

# Compression is a utility dependency. Parsing and image construction are ours.
include(FetchContent)
set(GT4_ZLIB_ARCHIVE "https://zlib.net/fossils/zlib-1.3.1.tar.gz" CACHE STRING
    "Pinned zlib archive URL or local archive path for offline builds")
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(zlib
    URL "${GT4_ZLIB_ARCHIVE}"
    URL_HASH SHA256=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(zlib)

add_library(gt4recomp_core STATIC src/common/project.cpp)
target_include_directories(gt4recomp_core PUBLIC include)
target_compile_features(gt4recomp_core PUBLIC cxx_std_20)
set_target_properties(gt4recomp_core PROPERTIES CXX_EXTENSIONS OFF)

add_library(gt4recomp_decode STATIC src/ee/decode.cpp src/ee/disassemble.cpp)
target_include_directories(gt4recomp_decode PUBLIC include)
target_compile_features(gt4recomp_decode PUBLIC cxx_std_20)
set_target_properties(gt4recomp_decode PROPERTIES CXX_EXTENSIONS OFF)

add_library(gt4recomp_executable STATIC
    src/executable/core_image.cpp
    src/executable/analysis_elf.cpp
)
target_include_directories(gt4recomp_executable PUBLIC include)
target_include_directories(gt4recomp_executable PRIVATE "${zlib_SOURCE_DIR}" "${zlib_BINARY_DIR}")
target_compile_features(gt4recomp_executable PUBLIC cxx_std_20)
set_target_properties(gt4recomp_executable PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(gt4recomp_executable PRIVATE zlibstatic)

if(WIN32)
    add_library(gt4recomp_input STATIC tools/common/verified_core.cpp)
    target_include_directories(gt4recomp_input PUBLIC tools/common)
    target_compile_features(gt4recomp_input PUBLIC cxx_std_20)
    set_target_properties(gt4recomp_input PROPERTIES CXX_EXTENSIONS OFF)
    target_compile_definitions(gt4recomp_input PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(gt4recomp_input PRIVATE bcrypt)
    add_executable(gt4core tools/gt4core/main.cpp)
    target_link_libraries(gt4core PRIVATE gt4recomp_executable gt4recomp_input)
    target_compile_definitions(gt4core PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    add_executable(gt4disasm tools/gt4disasm/main.cpp)
    target_link_libraries(gt4disasm PRIVATE gt4recomp_decode gt4recomp_executable gt4recomp_input)
endif()

add_executable(gt4recomp tools/gt4recomp/main.cpp)
target_link_libraries(gt4recomp PRIVATE gt4recomp_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(ee_disassemble_tests tests/unit/ee_disassemble_test.cpp)
    target_link_libraries(ee_disassemble_tests PRIVATE gt4recomp_decode)
    add_test(NAME ee_disassemble COMMAND ee_disassemble_tests)
    add_executable(ee_decode_tests tests/unit/ee_decode_test.cpp)
    target_link_libraries(ee_decode_tests PRIVATE gt4recomp_decode)
    add_test(NAME ee_decode COMMAND ee_decode_tests)
    add_executable(gt4recomp_tests tests/unit/project_test.cpp)
    target_link_libraries(gt4recomp_tests PRIVATE gt4recomp_core)
    add_test(NAME project_identity COMMAND gt4recomp_tests)
    add_test(NAME cli_smoke COMMAND gt4recomp)
    set_tests_properties(cli_smoke PROPERTIES
        PASS_REGULAR_EXPRESSION "GT4Recomp: learning laboratory")
    add_executable(executable_image_tests tests/unit/executable_image_test.cpp)
    target_link_libraries(executable_image_tests PRIVATE gt4recomp_executable zlibstatic)
    target_include_directories(executable_image_tests PRIVATE "${zlib_SOURCE_DIR}" "${zlib_BINARY_DIR}")
    add_test(NAME executable_image COMMAND executable_image_tests)
endif()

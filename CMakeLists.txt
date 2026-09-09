cmake_minimum_required(VERSION 3.24)
project(GT4Recomp VERSION 0.1.0 LANGUAGES CXX)

add_library(gt4recomp_core STATIC src/common/project.cpp)
target_include_directories(gt4recomp_core PUBLIC include)
target_compile_features(gt4recomp_core PUBLIC cxx_std_20)
set_target_properties(gt4recomp_core PROPERTIES CXX_EXTENSIONS OFF)

add_executable(gt4recomp tools/gt4recomp/main.cpp)
target_link_libraries(gt4recomp PRIVATE gt4recomp_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(gt4recomp_tests tests/unit/project_test.cpp)
    target_link_libraries(gt4recomp_tests PRIVATE gt4recomp_core)
    add_test(NAME project_identity COMMAND gt4recomp_tests)
    add_test(NAME cli_smoke COMMAND gt4recomp)
    set_tests_properties(cli_smoke PROPERTIES
        PASS_REGULAR_EXPRESSION "GT4Recomp: learning laboratory")
endif()

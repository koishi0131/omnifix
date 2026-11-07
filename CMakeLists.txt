cmake_minimum_required(VERSION 3.31)

project(omnifix VERSION 0.8.0 LANGUAGES CXX)

# Match the existing Helesta MSVC build with x64-windows-static dependencies.
if (MSVC AND VCPKG_TARGET_TRIPLET STREQUAL "x64-windows-static")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

find_package(Zydis CONFIG REQUIRED)
find_package(safetyhook CONFIG REQUIRED)

find_path(PICOSHA2_INCLUDE_DIRS "picosha2.h")

add_library(omnifix SHARED
    src/omnifix.cc
    src/omni_folder.cc
    src/memory.cc
    src/modules.cc
)

target_compile_features(omnifix PRIVATE cxx_std_23)

target_include_directories(omnifix PRIVATE
    ${PICOSHA2_INCLUDE_DIRS}
)

target_link_directories(omnifix PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
)

target_link_libraries(omnifix PRIVATE
    avs2-core
    safetyhook::safetyhook
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_link_libraries(omnifix PRIVATE stdc++exp)
endif ()

if (MINGW)
    set_target_properties(omnifix PROPERTIES PREFIX "")
endif ()

include(cmake/version.cmake)

target_compile_definitions(omnifix PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    "META_PROJECT_VERSION=\"${PROJECT_VERSION}\""
    "META_GIT_BRANCH=\"${META_GIT_BRANCH}\""
    "META_GIT_COMMIT_SHORT=\"${META_GIT_COMMIT_SHORT}\""
    "META_COMPILER_ID=\"${CMAKE_CXX_COMPILER_ID}\""
    "META_COMPILER_VERSION=\"${CMAKE_CXX_COMPILER_VERSION}\""
)

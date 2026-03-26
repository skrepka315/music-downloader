cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(MusicDownloader VERSION 1.0.0)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/DownloadPopup.cpp
)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR
        "Define GEODE_SDK environment variable to point to Geode"
    )
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Явно указываем папку с ресурсами
target_sources(${PROJECT_NAME} PRIVATE
    src/main.cpp
    src/DownloadPopup.cpp
)

setup_geode_mod(${PROJECT_NAME} RESOURCES resources)
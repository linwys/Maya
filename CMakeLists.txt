# CMakeLists.txt
cmake_minimum_required(VERSION 3.25)
project(maya LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Svg SvgWidgets)
include(FetchContent)

if(WIN32)
    FetchContent_Declare(
        bass_sdk
        URL "https://www.un4seen.com/files/bass24.zip"
    )
    FetchContent_Declare(
        bassflac_sdk
        URL "https://www.un4seen.com/files/bassflac24.zip"
    )
else()
    FetchContent_Declare(
        bass_sdk
        URL "https://www.un4seen.com/files/bass24-linux.zip"
    )
    FetchContent_Declare(
        bassflac_sdk
        URL "https://www.un4seen.com/files/bassflac24-linux.zip"
    )
endif()

FetchContent_MakeAvailable(bass_sdk bassflac_sdk)
add_library(bass SHARED IMPORTED GLOBAL)
if(WIN32)
    set_target_properties(bass PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${bass_sdk_SOURCE_DIR}/c"
        IMPORTED_IMPLIB "${bass_sdk_SOURCE_DIR}/c/x64/bass.lib"
        IMPORTED_LOCATION "${bass_sdk_SOURCE_DIR}/x64/bass.dll"
    )
else()
    set_target_properties(bass PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${bass_sdk_SOURCE_DIR}"
        IMPORTED_LOCATION "${bass_sdk_SOURCE_DIR}/x64/libbass.so"
    )
endif()

file(GLOB_RECURSE SRC_FILES
    "src/*.cpp"
    "src/*.hpp"
)

#win32 gen
if(WIN32)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/resources.rc" "IDI_ICON1 ICON DISCARDABLE \"${CMAKE_CURRENT_SOURCE_DIR}/favicon.ico\"\n")
    set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/resources.rc")
endif()

add_executable(maya WIN32 ${SRC_FILES} ${RC_FILE})

target_link_libraries(maya PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Svg
    Qt6::SvgWidgets
    bass
)

target_include_directories(maya PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

#flags
if(MSVC)
    target_compile_options(maya PRIVATE /O2 /permissive- /W4 /utf-8 /wd4324)
    target_compile_definitions(maya PRIVATE NOMINMAX)
    target_link_options(maya PRIVATE /INCREMENTAL:NO /DEBUG:NONE)
else()
    target_compile_options(maya PRIVATE -O3 -Wall -Wextra -Wpedantic)
    if(NOT APPLE)
        target_link_options(maya PRIVATE -Wl,-rpath,'$ORIGIN')
    endif()
endif()

if(WIN32)
    file(GLOB RUNTIME_DEPS 
        "${bass_sdk_SOURCE_DIR}/x64/bass.dll"
        "${bassflac_sdk_SOURCE_DIR}/x64/bassflac.dll"
    )
    set(YTDLP_URL "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe")
    set(YTDLP_NAME "yt-dlp.exe")
else()
    file(GLOB RUNTIME_DEPS 
        "${bass_sdk_SOURCE_DIR}/x64/libbass.so"
        "${bassflac_sdk_SOURCE_DIR}/x64/libbassflac.so"
    )
    set(YTDLP_URL "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp")
    set(YTDLP_NAME "yt-dlp")
endif()

# post build file copying
add_custom_command(TARGET maya POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${RUNTIME_DEPS}
    $<TARGET_FILE_DIR:maya>
    COMMAND_EXPAND_LISTS
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/helper.cmake" "
set(DEST_DIR \$ENV{DEST_DIR})
set(URL \$ENV{URL})
set(DEST_FILE \$ENV{DEST_FILE})
if(NOT EXISTS \"\${DEST_DIR}/\${DEST_FILE}\")
    message(STATUS \"Downloading \${DEST_FILE}...\")
    file(DOWNLOAD \"\${URL}\" \"\${DEST_DIR}/\${DEST_FILE}\" SHOW_PROGRESS STATUS res)
    list(GET res 0 status_code)
    if(NOT status_code EQUAL 0)
        message(FATAL_ERROR \"Download failed: \${res}\")
    endif()
endif()
")

add_custom_command(TARGET maya POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E env "DEST_DIR=$<TARGET_FILE_DIR:maya>" "URL=${YTDLP_URL}" "DEST_FILE=${YTDLP_NAME}"
            ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/helper.cmake"
)

if(WIN32)
    get_target_property(QMAKE_EXE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QMAKE_EXE}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET maya POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE} --dir $<TARGET_FILE_DIR:maya> $<TARGET_FILE:maya>
        )
    endif()
endif()
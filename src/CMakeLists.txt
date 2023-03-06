cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

project(wx_webrequest_tutorial_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# hack for buggy CMake's FindwxWidgets script
if(DEFINED ENV_WX_CONFIG)
    set(ENV{WX_CONFIG} ${ENV_WX_CONFIG})
endif()

set(wxWidgets_USE_STATIC 1)

find_package(wxWidgets REQUIRED COMPONENTS net core base)

set(SRCS main.cpp)

include(${wxWidgets_USE_FILE})

# download JSON library release (actually just a header file)
include(FetchContent)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

if(APPLE)
    # create bundle on apple compiles
    add_executable(main MACOSX_BUNDLE ${SRCS})

    # Set a custom plist file for the app bundle - needed for Mac OS Retina display
    set_target_properties(main PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
else()
    # the WIN32 is needed for Windows in order for it to look for WinMain
    # instead of the main function. This is ignored on other systems,
    # so it works on all platforms
    add_executable(main WIN32 ${SRCS} main.exe.manifest)
endif()

if(UNIX AND NOT APPLE)
    # On Linux CURL is REQUIRED for wxWidgets' wxWebRequest
    # but it is not reported by wx-config when compiling wxWidgets
    # using CMake. So we need to search for it manually.
    # See bug: https://github.com/wxWidgets/wxWidgets/issues/22860
    find_package(CURL REQUIRED)
    target_link_libraries(main PRIVATE nlohmann_json::nlohmann_json ${wxWidgets_LIBRARIES} ${CURL_LIBRARIES})
else()
    target_link_libraries(main PRIVATE nlohmann_json::nlohmann_json ${wxWidgets_LIBRARIES})
endif()

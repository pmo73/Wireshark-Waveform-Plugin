cmake_minimum_required(VERSION 3.5)

project(VCDReader VERSION 0.0.1 DESCRIPTION "Wireshark VCD file reader" LANGUAGES C CXX)
option(INSTALL_PLUGIN_LOCAL "Install the vcd file reader plugin inside the local folder of wireshark" ON)
set(CMAKE_CXX_STANDARD 23)

find_package(Wireshark CONFIG REQUIRED)

if (NOT Wireshark_PLUGINS_ENABLED)
    message(FATAL_ERROR "Wireshark was compiled without support for plugins")
endif ()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Enable optimizations for Release
if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" AND CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fomit-frame-pointer -DNDEBUG -D_NDEBUG -pipe -march=native -mtune=native -flto=auto ${CMAKE_CXX_FLAGS}")
else ()
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -fdiagnostics-color=always -fdiagnostics-show-option -g -ggdb ${CMAKE_CXX_FLAGS}")
endif ()

add_definitions(-DVCD_VERSION=\"${PROJECT_VERSION}\")

add_library(${PROJECT_NAME} MODULE vcd_plugin.cpp
        vcd_parser.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" DEFINE_SYMBOL "")
target_link_libraries(${PROJECT_NAME} epan)

if (${INSTALL_PLUGIN_LOCAL})
    #install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "$ENV{HOME}/.local/lib/wireshark/plugins/${Wireshark_MAJOR_VERSION}.${Wireshark_MINOR_VERSION}/wiretap" NAMELINK_SKIP)
    install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "$ENV{HOME}/wireshark/cmake-build-debug/run/plugins/4.2/wiretap" NAMELINK_SKIP)
else ()
    install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${Wireshark_PLUGIN_INSTALL_DIR}/wiretap" NAMELINK_SKIP)
endif ()

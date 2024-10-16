cmake_minimum_required(VERSION 3.5)

project(VCDReader VERSION 0.0.1 DESCRIPTION "Wireshark VCD file reader" LANGUAGES C CXX)
option(INSTALL_PLUGIN_LOCAL "Install the vcd file reader plugin inside the local folder of wireshark" ON)
set(CMAKE_CXX_STANDARD 23)

find_package(Wireshark CONFIG REQUIRED)

if (NOT Wireshark_PLUGINS_ENABLED)
    message(FATAL_ERROR "Wireshark was compiled without support for plugins")
endif ()

set(CMAKE_C_VISIBILITY_PRESET hidden)
if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "-Wall -Wextra ${CMAKE_CXX_FLAGS}")
endif ()

add_definitions(-DVCD_VERSION=\"${PROJECT_VERSION}\")

add_library(${PROJECT_NAME} MODULE vcd.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" DEFINE_SYMBOL "")
target_link_libraries(${PROJECT_NAME} epan)

if (${INSTALL_PLUGIN_LOCAL})
    install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "$ENV{HOME}/.local/lib/wireshark/plugins/${Wireshark_MAJOR_VERSION}.${Wireshark_MINOR_VERSION}/wiretap" NAMELINK_SKIP)
else ()
    install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${Wireshark_PLUGIN_INSTALL_DIR}/wiretap" NAMELINK_SKIP)
endif ()

cmake_minimum_required(VERSION 3.5)

project(waveform VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCE_FILES
        mainwindow.cpp
)

set(PROJECT_UI_FILES
        mainwindow.ui
)

add_library(${PROJECT_NAME} ${PROJECT_SOURCE_FILES} ${PROJECT_UI_FILES})
target_link_libraries(${PROJECT_NAME} PUBLIC Qt${QT_VERSION_MAJOR}::Widgets)

add_executable(Main main.cpp)
target_link_libraries(Main PRIVATE ${PROJECT_NAME})

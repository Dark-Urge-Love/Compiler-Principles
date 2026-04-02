cmake_minimum_required(VERSION 3.10)
project(RustTokenParser)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(RustTokenParser WIN32
    main.cpp
    MainWindow.cpp
    MainWindow.h
    MainWindow.ui
    Lexer.cpp
    Lexer.h
)

target_link_libraries(RustTokenParser Qt6::Widgets)

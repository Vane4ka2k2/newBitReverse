cmake_minimum_required(VERSION 3.16)
project(SobelCam-Qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Multimedia)

find_package(OpenMP)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/Image.cpp
)

set(HEADERS
    include/MainWindow.h
    include/ProcessorWorker.h
    include/Image.h
    third_party/stb/stb_image.h
    third_party/stb/stb_image_write.h
)

add_executable(SobelCam-Qt ${SOURCES} ${HEADERS})

target_include_directories(SobelCam-Qt PRIVATE
    include
    third_party/stb
)

target_link_libraries(SobelCam-Qt PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(SobelCam-Qt PRIVATE OpenMP::OpenMP_CXX)
endif()

if(MSVC)
    target_compile_definitions(SobelCam-Qt PRIVATE
        _SILENCE_ALL_MSVC_STL_DEPRECATION_WARNINGS
        _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
    )
    target_compile_options(SobelCam-Qt PRIVATE /Zc:__cplusplus)
endif()
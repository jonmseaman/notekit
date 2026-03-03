cmake_minimum_required(VERSION 3.16...4.2)
project(notekit VERSION 0.3.0 LANGUAGES CXX C)

# C++17 minimum
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt 6.10 — ONLY build system; meson.build is DEPRECATED
# Zero GTK/gtkmm/gtksourceviewmm includes remain after this migration.
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)
qt_standard_project_setup()

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# zlib used directly — do NOT use QByteArray::qCompress (incompatible header format)
find_package(ZLIB REQUIRED)

# fontconfig: Linux only
if(UNIX AND NOT APPLE)
    pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
endif()

# Optional: cLaTeXMath
option(HAVE_CLATEXMATH "Use cLaTeXMath to render LaTeX math" OFF)
set(CLATEXMATH_PATH "${CMAKE_CURRENT_LIST_DIR}/cLaTeXMath/" CACHE STRING "Base path for cLaTeXMath")

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/notebook.cpp
    src/navigation.cpp
    src/drawing.cpp
    src/imagewidgets.cpp
    src/settings.cpp
    src/findinfiles.cpp
    src/about.cpp
    src/about.ui
)

qt_add_executable(notekit ${SOURCES})

# Explicitly run MOC on .hpp headers with Q_OBJECT that AUTOMOC won't find
qt_wrap_cpp(EXTRA_MOC_SRCS src/notebook_widgets.hpp TARGET notekit)
target_sources(notekit PRIVATE ${EXTRA_MOC_SRCS})

# Qt resource system — all assets via :/notekit/... — NO runtime filesystem paths
target_sources(notekit PRIVATE data/resources.qrc)

target_compile_features(notekit PUBLIC cxx_std_17)

target_include_directories(notekit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${JSONCPP_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
)

target_link_directories(notekit PRIVATE ${JSONCPP_LIBRARY_DIRS})

target_link_libraries(notekit PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    ${JSONCPP_LIBRARIES}
    ZLIB::ZLIB
)

if(UNIX AND NOT APPLE)
    target_include_directories(notekit PRIVATE ${FONTCONFIG_INCLUDE_DIRS})
    target_link_libraries(notekit PRIVATE ${FONTCONFIG_LIBRARIES})
endif()

if(HAVE_CLATEXMATH)
    target_include_directories(notekit PRIVATE "${CLATEXMATH_PATH}/src/")
    target_link_libraries(notekit PRIVATE "-L${CLATEXMATH_PATH}/" "-l:libLaTeX.a")
    pkg_check_modules(tinyxml2 tinyxml2)
    if(tinyxml2_FOUND)
        target_link_libraries(notekit PRIVATE ${tinyxml2_LIBRARIES})
    endif()
    target_compile_definitions(notekit PRIVATE HAVE_CLATEXMATH __OS_Linux__)
endif()

install(TARGETS notekit DESTINATION bin)

enable_testing()
add_subdirectory(tests)

find_package(Qt6 REQUIRED COMPONENTS Test Widgets Core Gui)

set(TEST_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_BINARY_DIR}
    ${JSONCPP_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
)

# ── notebook_test ───────────────────────────────────────────────────────────
qt_add_executable(notebook_test
    ../src/notebook_test.cpp
    ../src/notebook.cpp
    ../src/drawing.cpp
    ../src/imagewidgets.cpp
)

qt_wrap_cpp(NOTEBOOK_TEST_MOC ../src/notebook_widgets.hpp ../src/drawing.h TARGET notebook_test)
target_sources(notebook_test PRIVATE ${NOTEBOOK_TEST_MOC})
target_sources(notebook_test PRIVATE ../data/resources.qrc)

target_include_directories(notebook_test PRIVATE ${TEST_INCLUDES})
target_link_directories(notebook_test PRIVATE ${JSONCPP_LIBRARY_DIRS})
target_link_libraries(notebook_test PRIVATE
    Qt6::Test Qt6::Widgets Qt6::Gui Qt6::Core
    ${JSONCPP_LIBRARIES} ZLIB::ZLIB)

add_test(NAME notebook_test COMMAND notebook_test -platform offscreen)

# ── navigation_test ──────────────────────────────────────────────────────────
qt_add_executable(navigation_test
    ../src/navigation_test.cpp
    ../src/navigation.cpp
)

target_include_directories(navigation_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_BINARY_DIR}
)
target_link_libraries(navigation_test PRIVATE Qt6::Test Qt6::Widgets Qt6::Core Qt6::Gui)

add_test(NAME navigation_test COMMAND navigation_test -platform offscreen)

# ── settings_test ─────────────────────────────────────────────────────────────
qt_add_executable(settings_test
    ../src/settings_test.cpp
    ../src/settings.cpp
)

target_include_directories(settings_test PRIVATE ${TEST_INCLUDES})
target_link_directories(settings_test PRIVATE ${JSONCPP_LIBRARY_DIRS})
target_link_libraries(settings_test PRIVATE
    Qt6::Test Qt6::Core Qt6::Widgets
    ${JSONCPP_LIBRARIES})

add_test(NAME settings_test COMMAND settings_test -platform offscreen)

# ── findinfiles_test ──────────────────────────────────────────────────────────
qt_add_executable(findinfiles_test
    ../src/findinfiles_test.cpp
    ../src/findinfiles.cpp
)

target_include_directories(findinfiles_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_BINARY_DIR}
)
target_link_libraries(findinfiles_test PRIVATE Qt6::Test Qt6::Core Qt6::Widgets)

add_test(NAME findinfiles_test COMMAND findinfiles_test -platform offscreen)

cmake_minimum_required(VERSION 3.20)

# Qt GUI smoke test (requires Qt 6 + Qt6::Test)
#
# This directory is added from the top-level CMakeLists when BUILD_GUI_TESTS
# is ON and Qt6 was found. It is compiled with the same toolchain as the GUI.
if(NOT Qt6Test_FOUND)
    message(WARNING "Qt6::Test not found, skipping GUI tests")
    return()
endif()

set(CMAKE_AUTOMOC ON)

add_executable(gui_smoke_test
    gui_smoke_test.cpp
    # The screens under test
    ../../src/gui/SessionContext.cpp
    ../../src/gui/LoginDialog.cpp
    ../../src/gui/InterviewWidget.cpp
    ../../src/gui/DailyChallengeWidget.cpp
    ../../src/gui/ProfileWidget.cpp
    ../../src/gui/LeaderboardWidget.cpp
    ../../src/gui/AdminWidget.cpp
    ../../src/gui/MainWindow.cpp
    # headers listed explicitly so AUTOMOC can moc them (they live in include/gui)
    ../../include/gui/SessionContext.h
    ../../include/gui/LoginDialog.h
    ../../include/gui/InterviewWidget.h
    ../../include/gui/DailyChallengeWidget.h
    ../../include/gui/ProfileWidget.h
    ../../include/gui/LeaderboardWidget.h
    ../../include/gui/AdminWidget.h
    ../../include/gui/MainWindow.h
)

target_include_directories(gui_smoke_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party
)

target_link_libraries(gui_smoke_test PRIVATE
    ai_interviewer_core
    Qt6::Widgets
    Qt6::Test
)

enable_testing()
add_test(NAME gui_smoke COMMAND gui_smoke_test)

# Toolchain file for the Qt 6 GUI build.
#
# Qt 6.11.2 ships prebuilt with the MinGW-w64 GCC 13.1 toolchain, so the GUI
# target must be compiled with that compiler rather than the C:/MinGW GCC 6.3
# one used by the console build (different libstdc++ ABI).
#
# Usage:
#   cmake -G "MinGW Makefiles" ^
#         -DCMAKE_TOOLCHAIN_FILE=cmake/qt-mingw.cmake ^
#         -DCMAKE_PREFIX_PATH="C:/Qt/6.11.2/mingw_64" ^
#         -DBUILD_GUI=ON -S . -B build-gui

set(QT_PREFIX "C:/Qt/6.11.2/mingw_64" CACHE PATH "Qt installation prefix")
set(QT_TOOLCHAIN_PREFIX "C:/Qt/Tools/mingw1310_64" CACHE PATH "Qt bundled MinGW prefix")

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   "${QT_TOOLCHAIN_PREFIX}/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "${QT_TOOLCHAIN_PREFIX}/bin/c++.exe")
set(CMAKE_RC_COMPILER  "${QT_TOOLCHAIN_PREFIX}/bin/windres.exe")
# CMAKE_MAKE_PROGRAM is resolved by the generator from PATH; the Qt MinGW bin
# directory must be on PATH when configuring.

set(CMAKE_FIND_ROOT_PATH "${QT_PREFIX}" "${QT_TOOLCHAIN_PREFIX}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_PREFIX_PATH "${QT_PREFIX}")

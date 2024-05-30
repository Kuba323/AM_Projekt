REM Dopasuj poniższe ścieżki:
set MINGW_PATH=D:\winlibs-x86_64-posix-seh-gcc-11.3.0-llvm-14.0.3-mingw-w64ucrt-10.0.0-r3 (1)\mingw64
set CMAKE_PATH=C:\Program Files\CMake\bin


set PATH=%PATH%;%MINGW_PATH%;%CMAKE_PATH%;

cmake -G "MinGW Makefiles" -B build
mingw32-make.exe -C build
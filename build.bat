@echo off
REM Windows (MinGW g++) build script. Run from this folder:  build.bat
g++ -std=c++11 -O2 -Wall -Wextra -Iinclude src\main.cpp src\ConsoleUI.cpp src\Item.cpp src\ItemManager.cpp src\ItemRepository.cpp src\MatchingService.cpp src\Utils.cpp -o CampusFind.exe
if errorlevel 1 (echo BUILD FAILED) else (echo Built CampusFind.exe - run it with: .\CampusFind.exe)

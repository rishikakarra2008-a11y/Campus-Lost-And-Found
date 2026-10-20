#!/bin/sh
# Linux / macOS build script.  Run:  sh build.sh
g++ -std=c++11 -O2 -Wall -Wextra -Iinclude src/main.cpp src/ConsoleUI.cpp src/Item.cpp \
    src/ItemManager.cpp src/ItemRepository.cpp src/MatchingService.cpp src/Utils.cpp -o CampusFind \
  && echo "Built ./CampusFind"

CXX      ?= g++
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra -pedantic -Iinclude
APP_SRC   = src/main.cpp src/ConsoleUI.cpp src/Item.cpp src/ItemManager.cpp \
            src/ItemRepository.cpp src/MatchingService.cpp src/Utils.cpp
CORE_SRC  = src/Item.cpp src/ItemManager.cpp src/ItemRepository.cpp \
            src/MatchingService.cpp src/Utils.cpp

all: CampusFind

CampusFind: $(APP_SRC)
	$(CXX) $(CXXFLAGS) $(APP_SRC) -o CampusFind

test: tests/test_core.cpp $(CORE_SRC)
	$(CXX) $(CXXFLAGS) tests/test_core.cpp $(CORE_SRC) -o test_core
	./test_core

clean:
	rm -f CampusFind CampusFind.exe test_core test_core.exe

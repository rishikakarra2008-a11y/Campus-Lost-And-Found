#include "ConsoleUI.h"
#include "ItemManager.h"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    try {
        const std::string dataFile = (argc > 1) ? argv[1] : "items.txt";
        ItemManager manager(dataFile);
        ConsoleUI ui(manager);
        ui.run();
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}

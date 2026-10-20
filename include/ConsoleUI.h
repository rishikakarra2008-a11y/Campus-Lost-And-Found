#ifndef CONSOLE_UI_H
#define CONSOLE_UI_H

#include "ItemManager.h"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

// Thrown when input ends (Ctrl+Z / Ctrl+D / piped input finished) so the program exits cleanly.
class InputClosed : public std::runtime_error {
public:
    InputClosed() : std::runtime_error("input closed") {}
};

// Menu, prompts and printing only. All rules are enforced by ItemManager.
class ConsoleUI {
private:
    ItemManager& manager;

    static std::string readline(const std::string& prompt);
    static std::string readtext(const std::string& prompt, std::size_t maxLength, bool allowEmpty = false);
    static std::string readdate(const std::string& prompt, const std::string& currentIso = "");
    static int readint(const std::string& prompt, int minimum, int maximum);
    static bool confirm(const std::string& prompt);

    void showmenu() const;
    void reportitem(ItemType type);
    void browse();
    void searchitems();
    void viewitem();
    void showmatchesforitem();
    void showallmatches();
    void claimitem();
    void closereport();
    void editreport();
    void showstatistics() const;
    void exportdata();

    void showpaged(const std::vector<const Item*>& results, const std::string& title) const;
    void printmatches(const std::vector<MatchResult>& matches) const;
    SearchFilter choosefilter() const;

public:
    explicit ConsoleUI(ItemManager& itemManager);
    void run();
};

#endif

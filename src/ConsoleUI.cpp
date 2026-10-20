#include "ConsoleUI.h"
#include "Utils.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

namespace {
const std::size_t SHORT_FIELD = 100;
const std::size_t LONG_FIELD = 300;
const std::size_t PAGE_SIZE = 10;
}

ConsoleUI::ConsoleUI(ItemManager& itemManager) : manager(itemManager) {}

std::string ConsoleUI::readline(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) throw InputClosed();
    return line;
}

std::string ConsoleUI::readtext(const std::string& prompt, std::size_t maxLength, bool allowEmpty) {
    while (true) {
        const std::string value = Utils::sanitizeInput(readline(prompt));
        if (value.empty() && allowEmpty) return value;
        if (value.empty()) { std::cout << "  This field cannot be empty.\n"; continue; }
        if (value.size() > maxLength) {
            std::cout << "  Too long (" << value.size() << " characters). Maximum is " << maxLength << ".\n";
            continue;
        }
        return value;
    }
}

std::string ConsoleUI::readdate(const std::string& prompt, const std::string& currentIso) {
    while (true) {
        const std::string raw = Utils::trim(readline(prompt));
        if (raw.empty()) return currentIso.empty() ? Utils::today() : currentIso;
        std::string iso;
        if (!Utils::parseDate(raw, iso)) { std::cout << "  Invalid date. Example: 2026-10-20 or 20-10-2026.\n"; continue; }
        if (Utils::isFutureDate(iso)) { std::cout << "  The date cannot be in the future.\n"; continue; }
        return iso;
    }
}

int ConsoleUI::readint(const std::string& prompt, int minimum, int maximum) {
    while (true) {
        int value = 0;
        if (Utils::parseInt(readline(prompt), value) && value >= minimum && value <= maximum) return value;
        std::cout << "  Enter a whole number from " << minimum << " to " << maximum << ".\n";
    }
}

bool ConsoleUI::confirm(const std::string& prompt) {
    while (true) {
        const std::string answer = Utils::toLower(Utils::trim(readline(prompt + " (y/n): ")));
        if (answer == "y" || answer == "yes"){
            return true;
        } 
        if (answer == "n" || answer == "no") {
            return false;
        }
        std::cout << "  Please answer y or n.\n";
    }
}

void ConsoleUI::showmenu() const {
    std::cout << "\n------------COLLEGE LOST AND FOUND PORTAL -------------------\n"
              << " 1. Report a LOST item          7. Suggested matches (all)\n"
              << " 2. Report a FOUND item         8. Return item / confirm claim\n"
              << " 3. Browse reports              9. Close a report\n"
              << " 4. Search reports             10. Edit an open report\n"
              << " 5. View one report            11. Statistics\n"
              << " 6. Matches for one report     12. Export to CSV\n"
              << " 0. Exit\n"
              << "---------------------------------------------------------------\n";
}

SearchFilter ConsoleUI::choosefilter() const {
    SearchFilter filter;
    std::cout << "Show: 1) everything  2) lost only  3) found only  4) open lost  5) open found\n";
    const int choice = readint("Choice: ", 1, 5);
    if (choice == 2 || choice == 4) { 
        filter.filterType = true; filter.type = ItemType::Lost; 
    }
    if (choice == 3 || choice == 5) { 
        filter.filterType = true; filter.type = ItemType::Found; 
    }
    if (choice == 4 || choice == 5) {
        filter.openOnly = true;
    }
    return filter;
}

void ConsoleUI::showpaged(const std::vector<const Item*>& results, const std::string& title) const {
    if (results.empty()) { std::cout << "No reports found.\n"; return; }
    const std::size_t pages = (results.size() + PAGE_SIZE - 1) / PAGE_SIZE;
    std::size_t page = 0;
    while (true) {
        std::cout << "\n--- " << title << ": " << results.size() << " report(s), page " << page + 1 << "/" << pages << " ---\n";
        const std::size_t begin = page * PAGE_SIZE;
        const std::size_t end = std::min(begin + PAGE_SIZE, results.size());
        for (std::size_t i = begin; i < end; ++i) results[i]->display(false);
        if (pages == 1) {
            return;
        }
        const std::string cmd = Utils::toLower(Utils::trim(readline("[n] next  [p] previous  [number] open report  [q] back: ")));
        if (cmd == "q" || cmd.empty()) return;
        if (cmd == "n") { if (page + 1 < pages) ++page; else std::cout << "Already on the last page.\n"; }
        else if (cmd == "p") { if (page > 0) --page; else std::cout << "Already on the first page.\n"; }
        else {
            int id = 0;
            const Item* item = Utils::parseInt(cmd, id) ? manager.find(id) : NULL;
            if (item) item->display(true); else std::cout << "Unknown command.\n";
        }
    }
}

void ConsoleUI::printmatches(const std::vector<MatchResult>& matches) const {
    for (std::size_t i = 0; i < matches.size(); ++i) {
         const Item* lost = manager.find(matches[i].lostId);
        const Item* found = manager.find(matches[i].foundId);
        if (!lost || !found) continue;
        std::cout << "\n#" << i + 1 << "  SCORE " << std::fixed << std::setprecision(1) << matches[i].score << "/100\n";
        std::cout.unsetf(std::ios::floatfield);
        std::cout << "  LOST : "; lost->display(false);
        std::cout << "  FOUND: "; found->display(false);
        std::cout << "  Why  : ";
        for (std::size_t r = 0; r < matches[i].reasons.size(); ++r) std::cout << (r ? "; " : "") << matches[i].reasons[r];
        std::cout << "\n";
    }
    std::cout << "\nThese are suggestions only. Verify ownership before returning an item.\n";
}

void ConsoleUI::reportitem(ItemType type) {
      const bool lost = (type == ItemType::Lost);
    std::cout << "\n--- Report a " << Item::typeToString(type) << " item ---  (type the details; Enter on date = today)\n";
    const std::string name = readtext("Item name (e.g. Blue water bottle): ", SHORT_FIELD);
    const std::string category = readtext("Category (e.g. Bottle, ID card, Bag, Phone): ", SHORT_FIELD);
    const std::string description = readtext("Description (colour, brand, marks, contents): ", LONG_FIELD);
    const std::string location = readtext(lost ? "Where was it lost? " : "Where was it found? ", SHORT_FIELD);
    const std::string date = readdate(lost ? "Date lost [today]: " : "Date found [today]: ");
    const std::string reporter = readtext(lost ? "Your name / roll number / phone: " : "Finder's name / phone (and where the item is kept): ", SHORT_FIELD);

    const int d = manager.findDuplicate(type, name, category, location, date);
    
    if (d != 0) {
        std::cout << "\nAn identical open report already exists:\n  ";
        manager.find(d)->display(false);
        if (!confirm("Save this as a separate report anyway?")) { 
            std::cout << "Cancelled.\n"; return;
         }
    }

    std::string error;
    const int id = manager.report(type, name, category, description, location, date, reporter, error);
    if (id == 0) { std::cout << "Could not save: " << error << "\n"; return; }
    std::cout << "Saved. Your report ID is " << id << ".\n";

    const std::vector<MatchResult> matches = manager.matchesFor(id, MatchingService::DEFAULT_MIN_SCORE, 3);
    
    if (!matches.empty()) {
        std::cout << "\nPossible matches already on file:";
        printmatches(matches);
    } else {
        std::cout << "No matching reports yet. New reports are checked against this one automatically.\n";
    }
}

void ConsoleUI::browse() { 
    showpaged(manager.list(choosefilter()), "All reports"); 
}

void ConsoleUI::searchitems() {
      const std::string query = readtext("Search words (all must appear in name/category/description/location): ", SHORT_FIELD);
    showpaged(manager.search(query, choosefilter()), "Search results");
}

void ConsoleUI::viewitem() {
    const int ID = readint("Report ID: ", 1, 999999999);
    const Item* item = manager.find(ID);
    
    if (item) {
        item->display(true);
    } else {
        std::cout << "No report with ID " << ID << ".\n";
       }
}

void ConsoleUI::showmatchesforitem() {
    const int ID = readint("Report ID: ", 1, 999999999);
    const Item* item = manager.find(ID);
    
    if (!item) { std::cout << "No report with ID " << ID << ".\n"; return; }
    
    if (!item->isOpen()) { std::cout << "Report #" << ID << " is " << Item::statusToString(item->getStatus()) << ", so it is not matched.\n"; return; }
    const std::vector<MatchResult> matches = manager.matchesFor(ID);
    
    if (matches.empty()) std::cout << "No match scores " << MatchingService::DEFAULT_MIN_SCORE << " or higher right now.\n";
    else printmatches(matches);
}

void ConsoleUI::showallmatches() {
    std::cout << "Searching for the best 20 suggestions...\n";
    bool truncated = false;
    const std::vector<MatchResult> m = manager.allMatches(MatchingService::DEFAULT_MIN_SCORE, 20, &truncated);
   
    if (m.empty()) {
        std::cout << "No suggested matches right now.\n";
    }else{
          printmatches(m);
    }
    if (truncated){
         std::cout << "(Only the 1000 most recent open lost reports were checked. Use option 6 for any specific report.)\n";
    }
        }

void ConsoleUI::claimitem() {
    std::cout << "\n1) Owner has a LOST report on file   2) Owner never filed a lost report\n";
    const int m = readint("Choice: ", 1, 2);
    int lostId = 0;
    if (m == 1) lostId = readint("LOST report ID: ", 1, 999999999);
    const int foundId = readint("FOUND report ID: ", 1, 999999999);

    const Item* found = manager.find(foundId);
    if (!found) { std::cout << "No report with ID " << foundId << ".\n"; return; }
    found->display(true);
    if (lostId != 0) {
        const Item* lost = manager.find(lostId);
        if (!lost) { std::cout << "No report with ID " << lostId << ".\n"; return; }
        lost->display(true);
        MatchResult result;
        if (manager.comparePair(lostId, foundId, result)) {
            std::cout << "Similarity score: " << std::fixed << std::setprecision(1) << result.score << "/100\n";
            std::cout.unsetf(std::ios::floatfield);
            if (result.score < MatchingService::DEFAULT_MIN_SCORE &&
                !confirm("These reports look quite different. Continue anyway?")) { std::cout << "Cancelled.\n"; return; }
        }
    }
    const std::string claimant = readtext("Name / roll number / phone of the person collecting the item: ", SHORT_FIELD);
    const std::string verification = readtext("How was ownership verified (ID shown, describes hidden marks...): ", LONG_FIELD);
    if (!confirm("Record this handover?")) { std::cout << "Cancelled.\n"; return; }

    std::string error;
    if (manager.claim(lostId, foundId, claimant, verification, error))
        std::cout << "Recorded. Found report #" << foundId << " is CLAIMED"
                  << (lostId ? " and lost report #" + Utils::toString(lostId) + " is CLOSED" : std::string()) << ".\n";
    else std::cout << "Not recorded: " << error << "\n";
}

void ConsoleUI::closereport() {
    const int ID = readint("Report ID to close: ", 1, 999999999);
    const Item* item = manager.find(ID);
    if (!item) {
         std::cout << "No report with ID " << ID << ".\n"; return; 
        }
    item->display(true);
    const std::string reason = readtext("Reason (e.g. owner no longer needs it, donated, duplicate): ", SHORT_FIELD);
    if (!confirm("Close this report?")) {
         std::cout << "Cancelled.\n"; return;
         }
    std::string error;
    if (manager.close(ID, reason, error)) {
        std::cout << "Report #" << ID << " closed.\n";
    } else std::cout << "Not closed: " << error << "\n";
}

void ConsoleUI::editreport() {
    const int ID = readint("Report ID to edit: ", 1, 999999999);
    const Item* i = manager.find(ID);
    if (!i) {
         std::cout << "No report with ID " << ID << ".\n"; return; }
    if (!i->isOpen()) {
         std::cout << "Only OPEN reports can be edited.\n"; return; }
    i->display(true);
    std::cout << "Press Enter to keep the current value.\n";

    const Item current = *i; // copy: the manager's storage may change
    std::string T;
    T = readtext("Name [" + current.getName() + "]: ", SHORT_FIELD, true);        const std::string name = T.empty() ? current.getName() : T;
    T = readtext("Category [" + current.getCategory() + "]: ", SHORT_FIELD, true); const std::string category = T.empty() ? current.getCategory() : T;
    T = readtext("Description [" + current.getDescription() + "]: ", LONG_FIELD, true); const std::string description = T.empty() ? current.getDescription() : T;
    T = readtext("Location [" + current.getLocation() + "]: ", SHORT_FIELD, true); const std::string location = T.empty() ? current.getLocation() : T;
    const std::string date = readdate("Date [" + current.getDate() + "]: ", current.getDate());
    T = readtext("Contact [" + current.getReporter() + "]: ", SHORT_FIELD, true);  const std::string reporter = T.empty() ? current.getReporter() : T;

    std::string error;
    if (manager.edit(ID, name, category, description, location, date, reporter, error)) {
        std::cout << "Report #" << ID << " updated.\n";
    } else {
        std::cout << "Not updated: " << error << "\n";
    }
}

void ConsoleUI::showstatistics() const {
    const Statistics S = manager.statistics();
    std::cout << "\n--- Statistics ---\n"
              << "Total reports        : " << S.total << "\n"
              << "Lost reports         : " << S.lost << "  (open " << S.openLost << ", closed " << S.closedLost << ")\n"
              << "Found reports        : " << S.found << "  (open " << S.openFound << ", claimed " << S.claimedFound
              << ", closed without owner " << S.closedOther << ")\n"
              << "Recovery rate        : " << std::fixed << std::setprecision(1) << S.recoveryRatePercent << "% of found items returned\n";
    std::cout.unsetf(std::ios::floatfield);
    if (!S.topCategories.empty()) {
        std::cout << "Most reported categories:\n";
        for (std::size_t i = 0; i < S.topCategories.size(); ++i)
            std::cout << "  " << S.topCategories[i].first << " - " << S.topCategories[i].second << "\n";
    }
}

void ConsoleUI::exportdata() {
    std::string path = Utils::sanitizeInput(readline("File name [lost_found_export.csv]: "));
    if (path.empty()){
          path = "lost_found_export.csv";
    } 
    std::string error;
    if (manager.exportCsv(path, error)) std::cout << "Exported to " << path << " (opens in Excel).\n";
    else std::cout << "Export failed: " << error << "\n";
}

void ConsoleUI::run() {
    const ItemManager::StartupInfo& i = manager.getStartupInfo();
    std::cout << "College Lost and Found Portal - " << i.loaded<< " report(s) loaded.\n";
    if (i.imported) {
           std::cout << i.imported << " record(s) imported from the older file format.\n";
    }
    if (i.repaired) {
        std::cout << i.repaired << " half-finished claim(s) were repaired.\n";
    }
    if (i.rejected) {
          std::cout << "Warning: " << i.rejected<< " unreadable line(s) were skipped (saved in items.txt.rejected).\n";
    }
    if (!i.warning.empty()) {
        std::cout << "Warning: " << i.warning << "\n";
    }

    try {
        while (true) {
            showmenu();
            switch (readint("Enter your choice: ", 0, 12)) {
                case 1:  reportitem(ItemType::Lost); break;
                case 2:  reportitem(ItemType::Found); break;
                case 3:  browse(); break;
                case 4:  searchitems(); break;
                case 5:  viewitem(); break;
                case 6:  showmatchesforitem(); break;
                case 7:  showallmatches(); break;
                case 8:  claimitem(); break;
                case 9:  closereport(); break;
                case 10: editreport(); break;
                case 11: showstatistics(); break;
                case 12: exportdata(); break;
                case 0:  std::cout << "All changes are already saved. Goodbye!\n"; return;
            }
        }
    } catch (const InputClosed&) {
        std::cout << "\nInput closed. All changes are already saved. Goodbye!\n";
    }
}

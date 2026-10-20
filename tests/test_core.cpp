// Automated tests for the core modules (no keyboard input needed).
// Build & run:  make test      (or see README)
#include "ItemManager.h"
#include "Utils.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

static int passed = 0, failed = 0;

#define CHECK(cond) do { if (cond) { ++passed; } else { ++failed; \
    std::cout << "  FAIL line " << __LINE__ << ": " #cond "\n"; } } while (0)

static void section(const char* name) { std::cout << "- " << name << "\n"; }

static std::string daysAgo(int n) {
    // today minus n days, as ISO date
    const int target = Utils::dayNumber(Utils::today()) - n;
    for (int y = 2024; y <= 2030; ++y)
        for (int m = 1; m <= 12; ++m)
            for (int d = 1; d <= 31; ++d) {
                char b[16]; std::snprintf(b, sizeof(b), "%04d-%02d-%02d", y, m, d);
                std::string iso;
                if (Utils::parseDate(b, iso) && Utils::dayNumber(iso) == target) return iso;
            }
    return Utils::today();
}

static void cleanup(const std::string& f) {
    std::remove(f.c_str()); std::remove((f + ".bak").c_str());
    std::remove((f + ".tmp").c_str()); std::remove((f + ".rejected").c_str());
}

static void testUtils() {
    section("Utils: dates, escaping, tokens");
    std::string iso;
    CHECK(Utils::parseDate("2026-10-20", iso) && iso == "2026-10-20");
    CHECK(Utils::parseDate("20-10-2026", iso) && iso == "2026-10-20");
    CHECK(Utils::parseDate("5/3/2026", iso) && iso == "2026-03-05");
    CHECK(!Utils::parseDate("2026-02-30", iso));
    CHECK(!Utils::parseDate("2025-02-29", iso));
    CHECK(Utils::parseDate("2024-02-29", iso));
    CHECK(!Utils::parseDate("hello", iso));
    CHECK(!Utils::parseDate("2026-13-01", iso));
    CHECK(Utils::dayNumber("1970-01-02") - Utils::dayNumber("1970-01-01") == 1);
    CHECK(Utils::isFutureDate("2999-01-01") && !Utils::isFutureDate("2020-01-01"));
    const std::string nasty = "a|b\\c\nd";
    CHECK(Utils::unescapeField(Utils::escapeField(nasty)) == nasty);
    CHECK(Utils::escapeField(nasty).find('|') == std::string::npos);
    CHECK(Utils::tokenize("Blue, Water-Bottle!").size() == 3);
    CHECK(Utils::sanitizeInput("  a \t  b\n ") == "a b");
    CHECK(Utils::editDistance("bottle", "bootle", 2) == 1);
    int v = 0;
    CHECK(Utils::parseInt("42", v) && v == 42);
    CHECK(!Utils::parseInt("4x2", v) && !Utils::parseInt("", v) && !Utils::parseInt("99999999999", v));
}

static void testReportAndPersistence() {
    section("Reporting, validation, persistence, special characters");
    const std::string f = "t_persist.txt"; cleanup(f);
    std::string err;
    {
        ItemManager m(f);
        CHECK(m.size() == 0);
        const int a = m.report(ItemType::Lost, "Blue | Bottle", "Bottle", "Steel, \"dent\" on side\\", "Library", daysAgo(2), "Ravi 9999", err);
        CHECK(a == 1);
        CHECK(m.report(ItemType::Found, "Flask", "Bottle", "blue steel flask", "Library", daysAgo(1), "Guard", err) == 2);
        CHECK(m.report(ItemType::Lost, "", "x", "x", "x", daysAgo(1), "x", err) == 0);
        CHECK(m.report(ItemType::Lost, "x", "x", "x", "x", "2999-01-01", "x", err) == 0);
        CHECK(m.report(ItemType::Lost, "x", "x", "x", "x", "31-02-2026", "x", err) == 0);
        CHECK(m.size() == 2);
    }
    {
        ItemManager m(f);   // restart
        CHECK(m.size() == 2);
        const Item* it = m.find(1);
        CHECK(it && it->getName() == "Blue | Bottle");
        CHECK(it && it->getDescription() == "Steel, \"dent\" on side\\");
        std::string e2;
        CHECK(m.report(ItemType::Lost, "Umbrella", "Umbrella", "black", "Gate", daysAgo(0), "me", e2) == 3);  // ids continue
    }
    cleanup(f);
}

static void testMatching() {
    section("Matching quality");
    const std::string f = "t_match.txt"; cleanup(f);
    std::string e;
    ItemManager m(f);
    const int lostBottle = m.report(ItemType::Lost, "Blue water bottle", "Bottle", "Steel bottle with a dent", "Library", daysAgo(3), "A", e);
    const int foundBottle = m.report(ItemType::Found, "Blue steel flask", "Bottle", "dented steel bottle", "Library", daysAgo(2), "B", e);
    const int foundWallet = m.report(ItemType::Found, "Black wallet", "Wallet", "leather wallet with cards", "Canteen", daysAgo(2), "B", e);
    const int lostPhone = m.report(ItemType::Lost, "Samsung mobile", "Electronics", "black Samsung phone cracked screen", "Block C", daysAgo(4), "C", e);
    const int foundPhone = m.report(ItemType::Found, "Smartphone", "Electronics", "Samsung phone with cracked screen", "Block C corridor", daysAgo(3), "D", e);
    const int foundPhoneTypo = m.report(ItemType::Found, "Moblie phne", "Electronics", "samsung cracked", "Block C", daysAgo(3), "D", e);
    const int foundRed = m.report(ItemType::Found, "Red water bottle", "Bottle", "plastic red bottle", "Library", daysAgo(2), "E", e);

    std::vector<MatchResult> r = m.matchesFor(lostBottle);
    CHECK(!r.empty() && r[0].foundId == foundBottle);
    CHECK(r.size() >= 1 && !r[0].reasons.empty());
    bool walletSuggested = false;
    for (size_t i = 0; i < r.size(); ++i) if (r[i].foundId == foundWallet) walletSuggested = true;
    CHECK(!walletSuggested);                                    // different category: not suggested
    // blue bottle should outrank red bottle (colour clash)
    double blueScore = 0, redScore = 0;
    for (size_t i = 0; i < r.size(); ++i) { if (r[i].foundId == foundBottle) blueScore = r[i].score; if (r[i].foundId == foundRed) redScore = r[i].score; }
    CHECK(blueScore > redScore);

    r = m.matchesFor(lostPhone);                                // synonyms: mobile = phone = smartphone
    CHECK(!r.empty() && r[0].foundId == foundPhone);
    bool typoFound = false;
    for (size_t i = 0; i < r.size(); ++i) if (r[i].foundId == foundPhoneTypo) typoFound = true;
    CHECK(typoFound);                                           // misspelled report still suggested

    r = m.matchesFor(foundPhone);                               // works in both directions
    CHECK(!r.empty() && r[0].lostId == lostPhone);

    // found BEFORE it was lost must not match
    const int lostLate = m.report(ItemType::Lost, "Grey umbrella", "Umbrella", "grey umbrella", "Gate", daysAgo(0), "F", e);
    const int foundEarly = m.report(ItemType::Found, "Grey umbrella", "Umbrella", "grey umbrella", "Gate", daysAgo(10), "G", e);
    r = m.matchesFor(lostLate);
    bool early = false;
    for (size_t i = 0; i < r.size(); ++i) if (r[i].foundId == foundEarly) early = true;
    CHECK(!early);

    std::vector<MatchResult> all = m.allMatches();
    CHECK(!all.empty());
    for (size_t i = 1; i < all.size(); ++i) CHECK(all[i - 1].score >= all[i].score);
    cleanup(f);
}

static void testClaims() {
    section("Claims, double-claim protection, close, edit");
    const std::string f = "t_claim.txt"; cleanup(f);
    std::string e;
    {
        ItemManager m(f);
        const int lost = m.report(ItemType::Lost, "ID card", "ID card", "student id Rahul", "Canteen", daysAgo(2), "Rahul", e);
        const int found = m.report(ItemType::Found, "Student ID", "ID card", "id card Rahul", "Canteen", daysAgo(1), "Guard", e);
        const int found2 = m.report(ItemType::Found, "Calculator", "Calculator", "casio", "Lab", daysAgo(1), "Guard", e);
        CHECK(!m.claim(lost, found, "", "", e));                      // verification required
        CHECK(!m.claim(found, lost, "R", "ID", e));                   // swapped types rejected
        CHECK(!m.claim(lost, 999, "R", "ID", e));                     // unknown id
        CHECK(m.claim(lost, found, "Rahul 123", "Showed college ID", e));
        CHECK(m.find(found)->getStatus() == ItemStatus::Claimed);
        CHECK(m.find(lost)->getStatus() == ItemStatus::Closed);
        CHECK(m.find(found)->getLinkedId() == lost && m.find(lost)->getLinkedId() == found);
        CHECK(!m.claim(lost, found, "X", "Y", e));                    // cannot claim twice
        CHECK(!m.claim(0, found, "X", "Y", e));
        CHECK(m.claim(0, found2, "Walk-in owner", "Described serial number", e));   // no lost report filed
        CHECK(m.matchesFor(found).empty());                           // closed reports are not matched
        const int toClose = m.report(ItemType::Lost, "Pen", "Stationery", "blue pen", "Hall", daysAgo(1), "me", e);
        CHECK(!m.close(toClose, "", e));
        CHECK(m.close(toClose, "no longer needed", e));
        CHECK(!m.close(toClose, "again", e));
        const int ed = m.report(ItemType::Lost, "Bag", "Bag", "black bag", "Hall", daysAgo(1), "me", e);
        CHECK(m.edit(ed, "Backpack", "Bag", "black backpack", "Hall", daysAgo(1), "me", e));
        CHECK(m.find(ed)->getName() == "Backpack");
        CHECK(!m.edit(found, "x", "x", "x", "x", daysAgo(1), "x", e)); // closed items cannot be edited
    }
    {
        ItemManager m(f);                                             // everything persisted
        CHECK(m.find(1)->getStatus() == ItemStatus::Closed);
        CHECK(m.find(2)->getStatus() == ItemStatus::Claimed);
        CHECK(m.find(5)->getName() == "Backpack");
        Statistics s = m.statistics();
        CHECK(s.claimedFound == 2 && s.total == 5);
    }
    cleanup(f);
}

static void testSearchAndDuplicates() {
    section("Search, filters, duplicates");
    const std::string f = "t_search.txt"; cleanup(f);
    std::string e;
    ItemManager m(f);
    m.report(ItemType::Lost, "Blue water bottle", "Bottle", "steel", "Library", daysAgo(2), "A", e);
    m.report(ItemType::Found, "Red notebook", "Book", "physics notes", "Lab 2", daysAgo(1), "B", e);
    SearchFilter any;
    CHECK(m.search("BOTTLE", any).size() == 1);                       // case-insensitive
    CHECK(m.search("water library", any).size() == 1);                // all words, any field
    CHECK(m.search("water lab", any).size() == 0);
    CHECK(m.search("", any).empty());
    SearchFilter foundOnly; foundOnly.filterType = true; foundOnly.type = ItemType::Found;
    CHECK(m.search("red", foundOnly).size() == 1 && m.search("bottle", foundOnly).empty());
    CHECK(m.list(any).size() == 2 && m.list(any)[0]->getId() == 2);   // newest first
    CHECK(m.findDuplicate(ItemType::Lost, "blue WATER bottle", "bottle", "library", daysAgo(2)) == 1);
    CHECK(m.findDuplicate(ItemType::Found, "blue water bottle", "bottle", "library", daysAgo(2)) == 0);
    cleanup(f);
}

static void testFileRobustness() {
    section("Corrupt lines, legacy import, compaction, crash repair");
    const std::string f = "t_robust.txt"; cleanup(f);
    {
        std::ofstream out(f.c_str());
        out << "#CampusFind data v2\n"
            << "garbage line\n"
            << "R2|1|LOST|OPEN|Phone|Electronics|black|Block A|2026-01-10|me|2026-01-10 10:00|0|\n"
            << "R2|2|FOUND|OPEN|Phone|Electronics|black|Block A|2026-13-40|me|2026-01-10 10:00|0|\n"  // bad date
            << "5|Old bottle|Bottle|blue|Gate|12-01-2026|Found|Open\n"                                // legacy format
            << "R2|9|FOUND|CLAIMED|Keys|Keys|silver|Gate|2026-01-11|g|2026-01-11 09:00|10|Handed to X\n"
            << "R2|10|LOST|OPEN|Keys|Keys|silver|Gate|2026-01-10|me|2026-01-10 09:00|0|\n";           // half-finished claim
    }
    {
        ItemManager m(f);
        const ItemManager::StartupInfo& s = m.getStartupInfo();
        CHECK(s.rejected == 2);
        CHECK(s.imported == 1);
        CHECK(s.repaired == 1);
        CHECK(m.find(5) && m.find(5)->getDate() == "2026-01-12");
        CHECK(m.find(10)->getStatus() == ItemStatus::Closed);       // repaired
        std::string e;
        CHECK(m.report(ItemType::Lost, "x", "x", "x", "x", "2026-01-01", "x", e) == 11);  // ids continue after highest
    }
    {   // many appended updates -> compaction on next start, data intact
        std::string e;
        { ItemManager m(f);
          const int id = m.report(ItemType::Lost, "Pen", "Stationery", "blue", "Hall", daysAgo(1), "me", e);
          for (int i = 0; i < 1100; ++i) m.edit(id, "Pen", "Stationery", i % 2 ? "blue pen" : "blue ink pen", "Hall", daysAgo(1), "me", e);
        }
        ItemManager m(f);
        CHECK(m.getStartupInfo().compacted);
        std::ifstream in(f.c_str()); std::string line; int lines = 0; while (std::getline(in, line)) ++lines;
        CHECK(lines < 20);
        CHECK(m.find(12) && m.find(12)->getName() == "Pen");
    }
    cleanup(f);
    std::remove("t_robust.txt.rejected");
}

static void testExport() {
    section("CSV export");
    const std::string f = "t_export.txt", out = "t_export.csv"; cleanup(f);
    std::string e;
    ItemManager m(f);
    m.report(ItemType::Lost, "=cmd()", "Bag", "has \"quotes\", commas", "Hall", daysAgo(1), "me", e);
    CHECK(m.exportCsv(out, e));
    std::ifstream in(out.c_str()); std::stringstream ss; ss << in.rdbuf();
    CHECK(ss.str().find("\"'=cmd()\"") != std::string::npos);        // formula injection neutralised
    CHECK(ss.str().find("\"has \"\"quotes\"\", commas\"") != std::string::npos);
    CHECK(!m.exportCsv("/nonexistent_dir/x.csv", e));
    cleanup(f); std::remove(out.c_str());
}

static void testScale(int total) {
    section("Scale test");
    const std::string f = "t_scale.txt"; cleanup(f);
    typedef std::chrono::steady_clock Clock;
    const char* names[] = {"bottle", "wallet", "phone", "bag", "umbrella", "laptop", "charger", "keys", "watch", "book"};
    const char* colours[] = {"blue", "black", "red", "green", "grey"};
    const char* places[] = {"Library", "Canteen", "Block A", "Block B", "Hostel", "Ground", "Lab 1", "Gate"};
    std::string e;
    {
        std::ofstream out(f.c_str());                                  // generate file directly (fast)
        out << "#CampusFind data v2\n";
        for (int i = 1; i <= total; ++i) {
            const bool lost = (i % 2 == 0);
            const int base = Utils::dayNumber("2026-01-01") + (i % 200);
            // ISO date from day number
            std::string date = "2026-01-01";
            for (int d = 0; d < 200; ++d) { /* cheap lookup avoided below */ break; }
            char buf[16]; std::snprintf(buf, sizeof(buf), "2026-%02d-%02d", 1 + (i % 200) / 28, 1 + (i % 200) % 28);
            date = buf; (void)base;
            out << "R2|" << i << '|' << (lost ? "LOST" : "FOUND") << "|OPEN|" << colours[i % 5] << ' ' << names[(i / 3) % 10]
                << '|' << names[(i / 3) % 10] << '|' << colours[i % 5] << " item number " << i << '|' << places[i % 8]
                << '|' << date << "|reporter|2026-01-01 10:00|0|\n";
        }
    }
    Clock::time_point t0 = Clock::now();
    ItemManager m(f);
    double loadMs = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
    CHECK(m.size() == static_cast<std::size_t>(total));

    t0 = Clock::now();
    for (int i = 0; i < 200; ++i) m.matchesFor(2 + 2 * i);
    double perQuery = std::chrono::duration<double, std::milli>(Clock::now() - t0).count() / 200.0;

    t0 = Clock::now();
    SearchFilter any;
    for (int i = 0; i < 20; ++i) m.search("blue bottle library", any);
    double perSearch = std::chrono::duration<double, std::milli>(Clock::now() - t0).count() / 20.0;

    t0 = Clock::now();
    for (int i = 0; i < 1000; ++i) m.report(ItemType::Found, "Blue bottle", "bottle", "blue steel", "Library", "2026-01-05", "x", e);
    double perReport = std::chrono::duration<double, std::milli>(Clock::now() - t0).count() / 1000.0;

    t0 = Clock::now();
    bool cut = false;
    std::vector<MatchResult> all = m.allMatches(40.0, 20, &cut);
    CHECK(total <= 2000 || cut);
    double allMs = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();

    std::cout << "    " << total << " records: load+index " << loadMs << " ms | matches for 1 report " << perQuery
              << " ms | search " << perSearch << " ms | add report " << perReport
              << " ms | all-matches " << allMs << " ms (" << all.size() << " shown)\n";
    CHECK(perQuery < 50.0 && perReport < 5.0);
    cleanup(f);
}

int main(int argc, char** argv) {
    testUtils();
    testReportAndPersistence();
    testMatching();
    testClaims();
    testSearchAndDuplicates();
    testFileRobustness();
    testExport();
    testScale(argc > 1 ? std::atoi(argv[1]) : 20000);
    std::cout << "\nPassed: " << passed << "   Failed: " << failed << "\n";
    return failed == 0 ? 0 : 1;
}

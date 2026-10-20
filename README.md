# College Lost and Found Portal (C++)

A menu-driven console application for recording lost and found belongings on a campus and
suggesting which reports belong together. Text-based matching only (no image recognition),
no database server: records live in one plain text file.

## Build and run

Windows (MinGW g++), in PowerShell or CMD, from this folder:

    g++ -std=c++11 -O2 -Iinclude src/main.cpp src/ConsoleUI.cpp src/Item.cpp src/ItemManager.cpp src/ItemRepository.cpp src/MatchingService.cpp src/Utils.cpp -o CampusFind.exe
    .\CampusFind.exe

(or just run `build.bat`). Linux/macOS: `sh build.sh` then `./CampusFind`, or `make`.
Optional argument: a different data file, e.g. `.\CampusFind.exe other.txt`.
To try it with demo data, copy `sample_items.txt` to `items.txt` first.

Automated tests: `make test` (or compile `tests/test_core.cpp` with the five core `.cpp` files).

## Structure

| File | Responsibility |
|------|----------------|
| `Item.h/.cpp` | The `Item` class: one report; private data, constructors, getters, controlled setters, display |
| `Utils.h/.cpp` | Dates, text clean-up, tokenising, file escaping, edit distance |
| `ItemRepository.h/.cpp` | File handling: append-only log, load, safe compaction |
| `MatchingService.h/.cpp` | Indexed, explainable scoring of lost vs found reports |
| `ItemManager.h/.cpp` | All business rules: report, search, claim, close, edit, statistics, CSV export |
| `ConsoleUI.h/.cpp` | Menus, prompts, printing (no rules, no file access) |
| `main.cpp` | Starts the program |
| `tests/test_core.cpp` | 85 automated checks incl. a scale test |
| `tests/test_plan.md` | Manual test table for your report |

## Features

Report lost/found items (date validated, no future dates) - automatic match suggestions the
moment a report is saved - browse with paging and filters - multi-word search - view one report -
matches for one report / best matches overall - claim with claimant and verification notes
(with or without a prior lost report) - close a report with a reason - edit open reports -
duplicate warning - statistics and recovery rate - CSV export - data survives restarts.

## How matching works (max 100)

| Part | Points | How |
|------|--------|-----|
| Name | 35 | word overlap; synonyms (mobile = phone = smartphone, flask = bottle ...), plurals and small spelling mistakes count |
| Category | 20 | same overlap rule |
| Description | 20 | same overlap rule |
| Location | 10 | same place = 10, similar place = partial |
| Date | 15 | found 0-3 days after lost = 15, falling to 0 after 30 days; found before the lost date = never suggested |
| Colour clash | -10 | both mention colours and none agree |

Suggestions need a score of at least 40 and always show the reasons. They are suggestions only:
staff must verify ownership before a handover. The weights are design choices, not calibrated
probabilities.

## Why it scales

* Saving appends one line (O(1)); the file is compacted only when it has grown to more than
  twice the live data. Compaction writes a temporary file, keeps a `.bak`, and restores the
  original on failure.
* An inverted index (word -> reports) means a report is only scored against reports that share
  a word, never against everything. A date window (found up to 1 day before / 90 days after
  lost) trims candidates further.
* A claim updates both reports in a single write; if the program dies halfway, the next
  start repairs the pair.
* Malformed lines are skipped and copied to `items.txt.rejected`; they never crash the program.

Measured here (-O2, synthetic records, one machine, so treat as indicative):

| Records | Load + index | Matches for one report | Search | Add a report | Best matches overall |
|--------:|-------------:|-----------------------:|-------:|-------------:|---------------------:|
| 20,000 | 0.15 s | 1.2 ms | 3 ms | 0.02 ms | 0.26 s |
| 100,000 | 0.78 s | 2.4 ms | 17 ms | 0.03 ms | 0.82 s |

"Best matches overall" checks only the 1000 newest open lost reports and says so.

## Honest limits

* One person at a time. There is no file locking, so do not run two copies on the same file.
  A real multi-user deployment needs a database server and a web or app front end; storage is
  isolated in `ItemRepository`, so that part can be replaced without touching the rest.
* Matching compares text only. Two different items described identically can score high.
* The reporter's phone/name is stored in plain text in the data file.

## OOP concepts you can point to

Classes and objects (`Item`, `ItemManager`, ...), constructors (several overloads in `Item`),
encapsulation (private members, getters, validated mutation), member functions, `std::vector`,
file handling (`ifstream`/`ofstream`). Beyond the basic list, the code also uses
`enum class`, `std::unordered_map`, a `namespace`, and one exception class (`InputClosed`).
Be ready to explain those, or tell your evaluator they are deliberate extras.
There is no inheritance or virtual functions, because `Item` has a `type` field instead.

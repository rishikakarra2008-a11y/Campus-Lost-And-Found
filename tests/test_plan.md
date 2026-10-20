# Manual test plan (run the program, record the ACTUAL result in the last column)

Start from a clean folder (delete items.txt) or copy sample_items.txt to items.txt for tests 3-4.

| # | Action | Expected | Actual |
|---|--------|----------|--------|
| 1 | Menu 1: report a lost "Blue water bottle", category Bottle | Saved, ID shown, "No matching reports yet" | |
| 2 | Menu 2: report found "Blue steel flask", category Bottle, same place, 2 days later | Saved; automatic match shown with score and reasons | |
| 3 | Menu 6 on the lost report ID | Found report listed with score > 40 | |
| 4 | Menu 7 | Highest scores listed first, with reasons | |
| 5 | Menu 4: search "blue library" | Both reports found (all words must appear) | |
| 6 | Enter date 30-02-2026 | "Invalid date" and prompt repeats | |
| 7 | Enter a future date | "Cannot be in the future" | |
| 8 | Type "abc" or 99 at the menu | Error message, menu shown again | |
| 9 | Report the same lost item twice | "Identical open report exists" warning | |
| 10 | Menu 8 (option 1): claim found with its lost report | Found = CLAIMED, lost = CLOSED, both linked | |
| 11 | Menu 8 again with the same IDs | Rejected: already claimed | |
| 12 | Menu 8 (option 2) on another open found item | Claimed without a lost report | |
| 13 | Menu 9: close an open report with a reason | Status CLOSED | |
| 14 | Menu 10: edit an open report | Changes saved; closed reports cannot be edited | |
| 15 | Exit, restart | All reports and statuses still present | |
| 16 | Menu 11 | Counts and recovery rate correct | |
| 17 | Menu 12, open the CSV in Excel | All records, one per row | |
| 18 | Type a name containing the | character | Saved and reloaded unchanged | |
| 19 | Press Ctrl+Z (Windows) / Ctrl+D (Linux) at any prompt | Program exits cleanly, data intact | |
| 20 | Add a garbage line to items.txt, restart | Warning shown; bad line copied to items.txt.rejected | |

# WRepeatfind 1.0.1

WRepeatfind finds repeated phrases within a single document, or within a book made of several chapter files. It is meant for writers checking their own work for accidental repetition — for example, a paragraph that was moved while editing but whose original was never deleted.

It uses the same fast comparison approach as WCopyfind and shares its document-reading code (`..\Common`), so it reads the same file types: `.docx`, `.doc`, `.txt`, `.pdf` (with `pdftotext.exe` beside `WRepeatfind.exe`), and `.htm`/`.html`. Text can also be pasted in directly.

Web site: https://WRepeatfind.WFindApps.org · Problems and suggestions: https://github.com/lab3e/WFindApps/issues · Author: Lou Bloomfield, lab3e@virginia.edu

## What's new in 1.0.1
- `.doc` files are read with Windows' IFilter reader as intended; in 1.0.0 it could not load, so they fell back to the rougher byte scanner
- An About box (in the window's system menu) with a link to https://WRepeatfind.WFindApps.org
- Licensed under the GNU General Public License, version 3 or later
- A clearer message when a file can't be opened because it may be damaged

## Using it
1. **Choose what to check.** Add documents (or drag them, or a folder of them, into the window), or click **Paste Text…**. For a book, list the chapters in order and leave **Check the documents together as one work** ticked, so that a passage repeated between chapter 2 and chapter 9 is found. Untick it to check each document on its own.
2. **Decide what counts as a repeat.** The shortest phrase to report (default 6 words), how many differing words in a row to tolerate (default 2), and whether to ignore case, punctuation, and numbers. **More Options…** holds the rest.
3. **Find Repeats**, then review. Each repeated passage is listed with its length, number of copies, and locations (document and paragraph). Double-click one to jump to it in the report.

The report is a single web page showing the whole text with each repeated passage highlighted in its own color. Clicking a highlighted passage jumps to its next copy. Each repeat can be **dismissed** if it is intentional (a refrain, a deliberate callback); dismissals are remembered by the browser and survive re-running WRepeatfind after edits, as long as the passage's words haven't changed.

Reports are saved in `Documents\WRepeatfind Reports` by default. Pasted text is saved in `%LOCALAPPDATA%\WRepeatfind\Pasted Text`. Settings and the document list are remembered in the registry under `HKEY_CURRENT_USER\Software\WRepeatfind`.

## Command line
```
WRepeatfind.exe /report <report.html> [/summary <summary.txt>] [/separate] [/phrase N] [/tolerance N] <documents...>
```
Runs without the window, using the saved settings for anything not given. `/summary` writes a tab-separated list of the repeats. Documents given without `/report` are simply loaded into the window (for example, when files are dropped onto the program's icon).

## How it works
Every word is hash-coded and the hash codes are sorted, so identical words sit together. Each pair of identical words (an earlier and a later occurrence) seeds a match that is grown backward and forward, bridging small differences, exactly as in WCopyfind. Because both copies are in the same text, two rules are added:
- the earlier copy must end before the later copy begins, so a phrase can't match an overlapping version of itself;
- a word can be the *later* copy of only one earlier passage, but a passage can be the *earlier* copy of any number of later ones — so a passage that appears three times is found three times, not twice.

For each later word, every earlier occurrence is tried and the longest match wins. Pairs of copies that share words are then gathered into one repeat group. Copies never cross a document boundary.

## Source files
- `RepeatFinder.h/.cpp` — the engine (`CRepeatFinder`): reading, seeding, growing, and grouping
- `RepeatReport.cpp` — writes the HTML report
- `MainDlg`, `PasteDlg`, `OptionsDlg` — the window and its two dialogs
- `WRepeatfind.cpp` — application start-up, settings, command line, opening the report
- `..\Common\DialogLayout.h` — keeps controls anchored as the window is resized (shared with WCopyfind)

## License
Copyright (C) 2026 Louis A. Bloomfield. WRepeatfind is free software under the GNU General Public License, version 3 or later; see [LICENSE](../LICENSE).

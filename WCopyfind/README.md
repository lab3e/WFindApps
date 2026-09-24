# WCopyfind 6.0.1

WCopyfind is a C++/MFC program for Windows that compares collections of documents to discover if they share phrases. It is used for detecting plagiarism and for finding common passages across large sets of text files.

Web site: https://WCopyfind.WFindApps.org

## What's new in 6.0.1
- Licensed under the GNU General Public License, version 3 or later (6.0.0 and earlier: version 2 or later), with the license text in `LICENSE` and a link to the source code in the About box

## What's new in 6.0.0

### A new window
- Three numbered steps: choose the documents, decide what counts as a match, compare
- Resizable, with the lists and the results growing to fill the window
- New and old document lists show each document's name and folder, with a count
- Documents and whole folders (with their subfolders) can be dropped onto either list; shortcuts are followed
- **Add Documents** has a drop-down menu (also on right-click) to add a folder, load or save the list, sort it by name (so "paper2" comes before "paper10"), keep it sorted, move documents between the lists, remove them, or clear the list
- The document lists are remembered between sessions
- The comparison runs in the background; the **Compare Documents** button becomes **Stop**, and the results appear as they are found
- The results can be sorted by clicking a column heading; when two documents have the same file name, the results show their folders too
- Less-used settings (minimum % matching, outer punctuation, non-words, long words, basic characters, language, report folder, brief report, opening the report automatically) and the vocabulary tool are in **More Options**
- Settings from WCopyfind 5.0.0 and earlier are carried over the first time 6.0.0 runs
- A document that can't be read is left out and listed, rather than stopping the whole comparison
- The report folder is created automatically; it defaults to `Documents\WCopyfind Reports`

### New reports
- `matches.html` lists the matching pairs with how much of each document matches, and can be filtered by name and sorted by any column
- Each pair has one page (`pairs\pair-00001.html`, …) showing the two documents side by side, each scrolling on its own, replacing the three files per pair and the HTML frames of earlier versions
- Matching passages are highlighted in the same color in both documents; clicking one brings its twin into view; **Next match** / **Previous match** (or the N and P keys) step through them
- **Show only paragraphs with matches** hides the rest of the text; the pages also work in dark mode and when printed
- Words skipped by the filters (non-words, long words) are now shown in the report instead of disappearing from it
- `matches.txt` keeps its tab-separated format (now written in UTF-8, so file names in any language survive)

### Fixes
- The first-run defaults for the report folder and language were cut to their first character (`C` and `E`), so a first comparison on a new computer failed
- Settings and saved lists lost accented and non-English characters
- **Load from File** cut the last character from a list's final line when the file didn't end with a line break
- The comparison thread updated the window directly, which Windows does not support
- Two documents with the same file name in different folders overwrote each other's report pages
- The vocabulary tool froze the window, ignored the Basic Characters setting, and wrote words in the ANSI code page; it now writes UTF-8, most frequent words first
- `.doc` files were read by the fallback byte scanner because COM was never initialized on the comparison thread, so the more accurate IFilter reader could not load
- Format strings passed a `CString` object and a 64-bit count where a pointer and an `int` were expected

## What was new in 5.0.0

5.0.0 was a modernization of WCopyfind 4.1.5, a program that had not been significantly edited for over a decade.

### Bug fixes (15 total)
- Replaced undefined-behaviour `HeapSort` (1-before-array pointer) with `std::sort`
- Fixed bitwise-AND used instead of logical-AND in word-skip logic (2 locations)
- Fixed stale extra argument passed to `fwprintf` in log output
- Fixed `wcsncat_s` called with wrong buffer-remaining count in HTML report builder
- Replaced raw `new` / `== NULL` checks (modern `new` never returns NULL) with `std::vector`
- Fixed `wcstombs_s` truncating DOCX paths longer than 256 characters
- Fixed broken `pdftotext` command-line quoting (trailing extra `"`)
- Fixed uninitialised `z_stream` in PDF inflate path
- Fixed `FindStringInBufferPdf` using `(size_t)-1` sentinel conflated with position 0
- Fixed non-atomic `g_abort` global read from two threads (now `std::atomic<bool>`)
- Fixed `SHBrowseForFolder` LPITEMIDLIST never freed (memory leak)
- Fixed `RegCreateKeyEx` HKEY handles never closed (resource leak, 4 functions)
- Fixed bitwise-AND used instead of logical-AND in vocabulary builder (2 locations)
- Fixed 20 MB stack/heap allocation with no OOM check (now `std::vector`)
- Fixed `.doc` byte-scan silently dropping extended characters (including curly apostrophes in possessives) due to if/else-chain structure bug

### IFilter support for .doc files
- Added Windows IFilter COM interface as the primary reader for legacy `.doc` files
- Falls back to the existing byte-scan approach when IFilter is unavailable
- Fixed IFilter path dropping the first character of every word (lookahead not put back)
- Fixed IFilter path truncating the last text segment (`FILTER_S_LAST_TEXT` not handled)

### zlib replaced with miniz
- Removed 31 bundled zlib source files
- Replaced with miniz (5 files, MIT licence), which also provides the ZIP reader used for DOCX extraction

### Toolchain
- Updated to Visual Studio 2025 (toolset v145), C++20

### Later fix
- Fixed imperfect-match extension stopping one flaw early when growing a phrase forward (`Flaws == m_MismatchTolerance` instead of `>`), so that "Most Imperfections to Allow" now applies equally in both directions; with a setting of 1, phrases could previously absorb a flaw only at their start

## Supported file types
- `.txt` — plain text
- `.docx` — Word 2007 and later (ZIP/XML, read via miniz)
- `.doc` — Word 97–2003 (read via IFilter COM, with byte-scan fallback)
- `.pdf` — PDF (read via pdftotext external tool, placed beside `WCopyfind.exe`)
- `.htm` / `.html` — web pages
- `.url` — internet shortcuts, fetched via WinINet

## Source files
- `WCopyfindDlg` — the main window; `OptionsDlg` — More Options and the vocabulary tool; `WCopyfind.cpp` — start-up and settings
- `clib\CompareDocuments` — the comparison engine; `clib\CompareReports.cpp` — the reports; `clib\ReportAssets.h` — their styles and scripts
- `..\Common` — document reading, word filters and hashing, sorting, miniz, and the dialog layout helper shared with WRepeatfind

## License
Copyright (C) 2026 Louis A. Bloomfield. WCopyfind is free software under the GNU General Public License, version 3 or later; see [LICENSE](../LICENSE). Versions through 6.0.0 were published under version 2 or later.

## Building
Open `WCopyfind.sln` (in the repository root) in Visual Studio 2022 or later with the **Desktop development with C++** workload and the **MFC** optional component installed. Build the Release x64 configuration. Each program builds into its own `x64\Release` folder.

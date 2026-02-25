# WCopyfind 5.0.0

WCopyfind is a C++/MFC program for Windows that compares collections of documents to discover if they share phrases. It is used for detecting plagiarism and for finding common passages across large sets of text files.

This version (5.0.0) is a modernization of WCopyfind 4.1.5, a program that had not been significantly edited for over a decade.

## What's new in 5.0.0

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

## Supported file types
- `.txt` — plain text
- `.docx` — Word 2007 and later (ZIP/XML, read via miniz)
- `.doc` — Word 97–2003 (read via IFilter COM, with byte-scan fallback)
- `.pdf` — PDF (read via pdftotext external tool)
- `.htm` / `.html` — web pages
- URLs — fetched via WinINet

## Building
Open `WCopyfind.sln` in Visual Studio 2022 or later with the **Desktop development with C++** workload and the **MFC** optional component installed. Build the Release x64 configuration.

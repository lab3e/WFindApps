# WFindApps

Two Windows programs that find shared and repeated phrases in documents, built on the same fast comparison engine.

- **[WCopyfind](WCopyfind/README.md)** compares a collection of documents with one another to find the phrases they share. It was written to detect plagiarism in student papers and is used for many other kinds of comparisons. Web site: https://WCopyfind.WFindApps.org
- **[WRepeatfind](WRepeatfind/README.md)** looks within one document, or a book made of chapter files, for phrases it repeats, so writers can catch accidental repetition. Web site: https://WRepeatfind.WFindApps.org

Both read `.docx`, `.doc`, `.txt`, `.pdf` (with `pdftotext.exe`), and `.htm`/`.html` files, and write their results as web pages.

## Repository layout
- `WCopyfind\` — WCopyfind
- `WRepeatfind\` — WRepeatfind
- `Common\` — code both programs compile: document reading (`InputDocument`), word filters and hashing (`Words`), sorting (`HeapSort`), the dialog layout helper, and miniz

## Versions
Each release is marked with a git tag: `WCopyfind-4.1.5` (the original source, before modernization), `WCopyfind-5.0.0`, `WRepeatfind-1.0.0`, and so on.

## Building
Open `WCopyfind.sln` in Visual Studio 2022 or later with the **Desktop development with C++** workload and the **MFC** optional component installed, and build the Release x64 configuration. Each program builds into its own `x64\Release` folder.

Copyright (C) 2026 Louis A. Bloomfield. WCopyfind is free software under the GNU General Public License, version 2 or later.

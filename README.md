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

## License
Copyright (C) 2026 Louis A. Bloomfield

WCopyfind and WRepeatfind are free software: you can redistribute them and/or modify them under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. They are distributed in the hope that they will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the [LICENSE](LICENSE) file for the full text.

In short: anyone may use, study, share, and change these programs. Anyone who distributes a changed version must make its source code available under the same license. Each source file carries the tag `SPDX-License-Identifier: GPL-3.0-or-later`.

The bundled [miniz](Common/miniz) library, by Rich Geldreich, RAD Game Tools, Valve Software, and contributors, keeps its own MIT license, which is stated in its source files.

WCopyfind releases through 6.0.0 (tags `WCopyfind-4.1.5`, `WCopyfind-5.0.0`, `WCopyfind-6.0.0`) were published under GPL version 2 or later. WRepeatfind 1.0.0 carried no license notice; version 3 or later applies from WRepeatfind 1.0.1 and WCopyfind 6.0.1 on.

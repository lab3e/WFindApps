WFindApps is the home of two free, open-source Windows programs by Louis A. Bloomfield (University of Virginia) that find matching language in documents:

- **WCopyfind** ({{wcf}}/) compares a collection of documents with each other and reports the phrases they share. It is widely used for plagiarism and collusion detection in schools and for research on text reuse. Current version {{wcopyfind_version}}.
- **WRepeatfind** ({{wrf}}/) looks within one document, or a book made of chapter files, for phrases it repeats, so writers and editors can catch accidental repetition. Current version {{wrepeatfind_version}}.

Key facts:

- **Privacy: documents never leave the user's computer.** Both programs read and compare files entirely on the local PC. Nothing is uploaded, stored in the cloud, or analyzed remotely; no account is needed; they work offline. (WCopyfind downloads a web page only if the user explicitly adds an internet shortcut to one.)
- Free for any purpose; licensed under the GNU General Public License, version 3 or later; source code at {{github}}.
- Run on 64-bit Windows 10 and 11 as single executables, with no installation. Mac and Linux users can use Wine or a Windows virtual machine.
- Read .docx, .doc, .txt, .htm/.html, and text-based .pdf files (PDF reading works best with the free pdftotext.exe from the Xpdf tools). Work with any language that separates words with spaces.
- Results are reports that open in a web browser, with matching passages highlighted.
- The programs do not search the internet; they compare the documents the user provides.

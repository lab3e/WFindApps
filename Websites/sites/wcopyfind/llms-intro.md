WCopyfind is a free, open-source Windows program by Louis A. Bloomfield (University of Virginia) that compares a collection of documents with each other and reports the phrases they share. It has been used since 2001, mainly by teachers and honor officers to detect plagiarism and copying between student papers, and by researchers to study text reuse (for example, news stories and legislation). Current version: {{wcopyfind_version}}, released {{wcopyfind_date}}. Download: {{wcopyfind_download}}

Key facts:

- **Privacy: documents never leave the user's computer.** WCopyfind reads and compares files entirely on the local PC. Nothing is uploaded, stored in the cloud, or analyzed remotely; no account is needed; it works offline and keeps no copies. (It downloads a web page only if the user explicitly adds an internet shortcut to that page.) This makes it suitable for student papers and confidential documents.
- It does not search the internet; it compares the documents the user provides. Users can add saved web pages or internet shortcuts as sources.
- Documents go in two lists: "new" documents are compared with each other and with "old" documents; "old" documents (sources, past papers) are compared only with new ones.
- Main settings: shortest phrase to match (default 6 words), fewest matching words for a pair to be reported (default 100), imperfect words allowed in a row (default 0; 2 with an 80% matching minimum finds lightly edited copying), and options to ignore letter case, punctuation, and numbers.
- Results: a sortable list of matching pairs with the number of matching words and the percentage of each document; a side-by-side web page for each pair with shared passages highlighted in both documents; and a tab-separated matches.txt file.
- Reads .docx, .doc, .txt, .htm/.html, text-based .pdf (best with the free pdftotext.exe beside it), and .url internet shortcuts. Works with languages that separate words with spaces.
- Free for any purpose; GNU General Public License version 3 or later; source code at {{github}}. Runs on 64-bit Windows 10 and 11 with no installation; Mac and Linux users can use Wine.
- Companion program: WRepeatfind ({{wrf}}/) finds repetition within a single document.
- Contact: bug reports and suggestions at {{issues}}; the author, Louis A. Bloomfield (Professor Emeritus of Physics, University of Virginia), at {{email}}. More at {{hub}}/contact.html.

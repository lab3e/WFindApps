This repository contains WCopyfind 5.0.0, a C++/MFC program that compares collections of documents to discover if they share phrases.

It is based on WCopyfind 4.1.5, a program that had not been edited significantly for a decade or so. Version 5.0.0 modernizes the codebase: 15 bugs were fixed, zlib was replaced with miniz, and IFilter COM is used for reading .doc files. The program runs on Windows and requires the Visual C++ runtime.

# WFindApps websites

The source for three static websites:

| Site | Folder | Purpose |
|---|---|---|
| https://wfindapps.org | `sites/wfindapps` | Home of both apps: overview, downloads, news, about, contact, privacy |
| https://wcopyfind.wfindapps.org | `sites/wcopyfind` | WCopyfind: overview, download, user guide, settings, reports, FAQ, troubleshooting, how it works, uses, version history, essays |
| https://wrepeatfind.wfindapps.org | `sites/wrepeatfind` | WRepeatfind: overview, download, user guide, tips for writers, FAQ, troubleshooting, how it works, version history |

The sites are plain HTML and CSS (no database, no scripts, no cookies), so they're fast, secure, and easy to host.

## Building

```
python build.py
python tools/check_site.py
```

`build.py` writes each site into `dist/<host name>/`. `check_site.py` checks for broken links and anchors, missing descriptions, duplicate titles, unclosed tags, and unfilled placeholders.

## Updating for a new release

Change the version number (and, if needed, the zip size and date) near the top of `build.py`, add the release to the version-history pages (`sites/*/pages/changelog.html`) and the news page (`sites/wfindapps/pages/news.html`), then rebuild and upload. Download links point to the release files on GitHub, so each release must be published there with the same file names.

## How the pages are made

Each page is an HTML fragment in `sites/<site>/pages/`, starting with a header:

```
<!--
title: Page title
description: One or two sentences for search results and link previews
nav: which main-menu item to highlight
help: yes    (optional: shows the help-topics menu beside the page)
schema: faq | article | howto    (optional: structured data for search engines)
-->
```

Fragments can use `{{placeholders}}` for values defined in `build.py`, such as `{{wcopyfind_download}}` and `{{privacy_panel}}` (the privacy promise, worded once and shown on all three sites). `build.py` wraps each fragment in the shared header, navigation, and footer, and for each site also writes:

- `sitemap.xml` and `robots.txt` for search engines
- `llms.txt`, a plain-text summary for AI assistants (from `sites/<site>/llms-intro.md` plus a list of pages)
- structured data (JSON-LD) describing the software, FAQs, articles, and breadcrumbs
- `.htaccess` with https and www redirects, the 404 page, compression, and caching (Apache only)
- `downloads/<App>-Samples.zip` from the sample documents in `samples/`

Images: `tools/make_images.py <folder of raw screenshots>` makes the logo, icons, favicons, optimized screenshots, and the 1200×630 link-preview images.

## Publishing

Upload the contents of each `dist/<host name>/` folder to that site's document root, including the hidden `.htaccess` file. Then:

1. Make sure each site has an HTTPS certificate (for example, Let's Encrypt in Plesk), and that `www.` addresses point to the same sites.
2. Register the three sites with [Google Search Console](https://search.google.com/search-console) and [Bing Webmaster Tools](https://www.bing.com/webmasters), and submit each site's `sitemap.xml`. Bing's index also feeds several AI assistants.
3. On the old Plagiarism Resource Site, add a notice pointing to the new sites, and redirect the old software pages to their new homes (see below), so existing links and search rankings carry over.

## Redirects from the old site

Suggested permanent (301) redirects for `plagiarism.bloomfieldmedia.com`:

| Old address | New address |
|---|---|
| `/software/` | `https://wcopyfind.wfindapps.org/` |
| `/software/wcopyfind/` | `https://wcopyfind.wfindapps.org/download.html` |
| `/software/wcopyfind-instructions/` | `https://wcopyfind.wfindapps.org/guide.html` |
| `/software/faq/` | `https://wcopyfind.wfindapps.org/faq.html` |
| `/software/interesting-things-wcopyfind-can-do/` | `https://wcopyfind.wfindapps.org/uses.html` |
| `/software/copyfind/` | `https://wcopyfind.wfindapps.org/download.html#copyfind` |
| `/2011/06/14/the-importance-of-writing/` | `https://wcopyfind.wfindapps.org/essays/the-importance-of-writing.html` |
| `/2011/06/14/on-stating-the-goals-for-assigned-work/` | `https://wcopyfind.wfindapps.org/essays/on-stating-the-goals-for-assigned-work.html` |
| `/2011/06/14/on-the-motivations-for-grade-inflation/` | `https://wcopyfind.wfindapps.org/essays/on-the-motivations-for-grade-inflation.html` |
| `/2011/06/14/on-the-motivations-for-cheating/` | `https://wcopyfind.wfindapps.org/essays/on-the-motivations-for-cheating.html` |

Keep the old download files (`/WCopyfind.4.1.5.exe.zip` and so on) where they are: the new WCopyfind download page links to them for older versions. In WordPress, the free *Redirection* plugin can set these up without editing server files.

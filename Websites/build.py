# build.py : builds the three WFindApps websites into Websites/dist
#
#   python build.py            build all three sites
#
# Each page is an HTML fragment in sites/<site>/pages, starting with a header block:
#
#   <!--
#   title: Page title (shown in the browser tab and search results)
#   description: One or two sentences for search results and link previews
#   nav: the navigation item this page belongs to
#   schema: faq | article | howto  (optional structured data for search engines)
#   -->
#
# The fragment may use {{name}} placeholders for the values in VALUES below (version numbers, download links,
# site addresses), so a new release means changing this file only. The script wraps each fragment in the
# shared layout, copies the site's images, makes the sample-document downloads, and writes sitemap.xml,
# robots.txt, llms.txt, and .htaccess for each site.

import html, json, os, re, shutil, zipfile, datetime

ROOT = os.path.dirname(os.path.abspath(__file__))
DIST = os.path.join(ROOT, 'dist')
TODAY = datetime.date.today().isoformat()

AUTHOR = 'Louis A. Bloomfield'
PERSON = {'@type': 'Person', 'name': AUTHOR, 'jobTitle': 'Professor Emeritus of Physics',
          'affiliation': {'@type': 'CollegeOrUniversity', 'name': 'University of Virginia'}}
EMAIL = 'lab3e@virginia.edu'
POSTAL = ['Louis Bloomfield', 'Department of Physics', 'Box 400714', 'Charlottesville, VA 22904-4714']
PERSON['email'] = EMAIL
PERSON['address'] = {'@type': 'PostalAddress', 'streetAddress': 'Department of Physics, Box 400714', 'addressLocality': 'Charlottesville',
                     'addressRegion': 'VA', 'postalCode': '22904-4714', 'addressCountry': 'US'}
GITHUB = 'https://github.com/lab3e/WFindApps'
ISSUES = GITHUB + '/issues'

SITES = {
    'wfindapps': {
        'host': 'wfindapps.org', 'name': 'WFindApps', 'display': 'WFindApps.org',
        'tagline': 'Free tools that find shared and repeated phrases in documents',
        'nav': [('Home', 'index.html'), ('Download', 'download.html'), ('News', 'news.html'), ('About', 'about.html'), ('Contact', 'contact.html')],
    },
    'wcopyfind': {
        'host': 'wcopyfind.wfindapps.org', 'name': 'WCopyfind', 'display': 'WCopyfind.WFindApps.org',
        'tagline': 'Free plagiarism detection software that finds the phrases documents share',
        'nav': [('Overview', 'index.html'), ('Download', 'download.html'), ('User Guide', 'guide.html'), ('Settings', 'settings.html'),
                ('Reports', 'reports.html'), ('FAQ', 'faq.html'), ('How It Works', 'how-it-works.html')],
        'help': [('Getting started', 'guide.html'), ('Settings explained', 'settings.html'), ('Reading the results', 'reports.html'),
                 ('Frequently asked questions', 'faq.html'), ('Troubleshooting', 'troubleshooting.html'), ('How WCopyfind works', 'how-it-works.html'),
                 ('Interesting things it can do', 'uses.html'), ('Version history', 'changelog.html')],
    },
    'wrepeatfind': {
        'host': 'wrepeatfind.wfindapps.org', 'name': 'WRepeatfind', 'display': 'WRepeatfind.WFindApps.org',
        'tagline': 'Free software that finds the phrases your writing repeats',
        'nav': [('Overview', 'index.html'), ('Download', 'download.html'), ('User Guide', 'guide.html'), ('For Writers', 'tips.html'),
                ('FAQ', 'faq.html'), ('How It Works', 'how-it-works.html')],
        'help': [('Getting started', 'guide.html'), ('Tips for writers and editors', 'tips.html'), ('Frequently asked questions', 'faq.html'),
                 ('Troubleshooting', 'troubleshooting.html'), ('How WRepeatfind works', 'how-it-works.html'), ('Version history', 'changelog.html')],
    },
}

WCOPYFIND_VERSION = '6.0.2'
WREPEATFIND_VERSION = '1.0.1'
WCOPYFIND_ZIP = f'WCopyfind-{WCOPYFIND_VERSION}-win-x64.zip'
WREPEATFIND_ZIP = f'WRepeatfind-{WREPEATFIND_VERSION}-win-x64.zip'

VALUES = {
    'hub': 'https://wfindapps.org', 'wcf': 'https://wcopyfind.wfindapps.org', 'wrf': 'https://wrepeatfind.wfindapps.org',
    'github': GITHUB, 'issues': ISSUES, 'email': EMAIL, 'email_link': f'<a href="mailto:{EMAIL}">{EMAIL}</a>',
    'postal': '<address>' + '<br>'.join(POSTAL) + '</address>', 'postal_text': ', '.join(POSTAL),
    'license': GITHUB + '/blob/main/LICENSE', 'author': AUTHOR, 'year': str(datetime.date.today().year),
    'wcopyfind_version': WCOPYFIND_VERSION, 'wcopyfind_zip': WCOPYFIND_ZIP, 'wcopyfind_size': '1.4 MB',
    'wcopyfind_download': f'{GITHUB}/releases/download/WCopyfind-{WCOPYFIND_VERSION}/{WCOPYFIND_ZIP}',
    'wcopyfind_release': f'{GITHUB}/releases/tag/WCopyfind-{WCOPYFIND_VERSION}',
    'wcopyfind_date': 'September 24, 2026',
    'wrepeatfind_version': WREPEATFIND_VERSION, 'wrepeatfind_zip': WREPEATFIND_ZIP, 'wrepeatfind_size': '1.4 MB',
    'wrepeatfind_download': f'{GITHUB}/releases/download/WRepeatfind-{WREPEATFIND_VERSION}/{WREPEATFIND_ZIP}',
    'wrepeatfind_release': f'{GITHUB}/releases/tag/WRepeatfind-{WREPEATFIND_VERSION}',
    'wrepeatfind_date': 'September 24, 2026',
    'old_site': 'https://plagiarism.bloomfieldmedia.com',
    'xpdf': 'https://www.xpdfreader.com/download.html',
}

# The privacy promise, worded once and used on every site. Both programs read and compare files entirely on the
# user's own computer; the only network use is WCopyfind reading a web page the user explicitly points it to.
VALUES['privacy_short'] = 'Your documents never leave your computer.'
VALUES['privacy_panel'] = '''<section class="privacy" aria-labelledby="privacy-heading">
<div class="privacy-icon" aria-hidden="true"><svg viewBox="0 0 24 24" width="40" height="40"><path fill="currentColor" d="M12 1 3 5v6c0 5.5 3.8 10.7 9 12 5.2-1.3 9-6.5 9-12V5l-9-4Zm0 5a3 3 0 0 1 3 3v2h1v6H8v-6h1V9a3 3 0 0 1 3-3Zm0 2a1 1 0 0 0-1 1v2h2V9a1 1 0 0 0-1-1Z"/></svg></div>
<div>
<h2 id="privacy-heading">Your documents never leave your computer</h2>
<p>WCopyfind and WRepeatfind read and compare your files right where they are, on your own PC. Nothing is uploaded, nothing is stored in the cloud, and no one else's computer ever sees your text. There's no account to create and nothing to sign in to, and the programs work without an internet connection.</p>
<p>That makes them safe for the documents that matter most: student papers, unpublished manuscripts, theses, legal and business files, and anything confidential. Many online plagiarism checkers keep a copy of every paper submitted to them. These programs keep nothing, because they never receive anything.</p>
<p class="fine">The one exception is by your choice: if you add an internet shortcut to WCopyfind, it downloads that web page to compare it. It never sends your documents anywhere. <a href="{hub}/privacy.html">Read the full privacy statement</a>.</p>
</div>
</section>'''.format(hub='https://wfindapps.org')

# ---------------------------------------------------------------------------------------------------------

def fill(text):
    return re.sub(r'\{\{(\w+)\}\}', lambda m: VALUES[m.group(1)], text)

def parse_page(path):
    source = open(path, encoding='utf-8').read()
    m = re.match(r'\s*<!--(.*?)-->\s*', source, re.S)
    meta = {}
    for line in m.group(1).strip().splitlines():
        key, _, value = line.partition(':')
        meta[key.strip()] = fill(value.strip())
    return meta, fill(source[m.end():])

def breadcrumb_json(site, crumbs):
    return {'@type': 'BreadcrumbList', 'itemListElement': [
        {'@type': 'ListItem', 'position': i + 1, 'name': name, 'item': url} for i, (name, url) in enumerate(crumbs)]}

def software_json(site_key):
    name = SITES[site_key]['name']
    version = WCOPYFIND_VERSION if site_key == 'wcopyfind' else WREPEATFIND_VERSION
    url = 'https://' + SITES[site_key]['host'] + '/'
    return {
        '@type': 'SoftwareApplication', 'name': name, 'url': url, 'softwareVersion': version,
        'operatingSystem': 'Windows 10, Windows 11', 'applicationCategory': 'UtilitiesApplication',
        'applicationSubCategory': 'Plagiarism detection' if site_key == 'wcopyfind' else 'Writing and editing',
        'description': SITES[site_key]['tagline'] + '.',
        'downloadUrl': VALUES[site_key + '_download'], 'fileSize': VALUES[site_key + '_size'],
        'installUrl': url + 'download.html', 'screenshot': url + 'img/' + site_key + '-window.png',
        'license': 'https://www.gnu.org/licenses/gpl-3.0.html', 'isAccessibleForFree': True,
        'offers': {'@type': 'Offer', 'price': '0', 'priceCurrency': 'USD'},
        'author': PERSON, 'publisher': {'@type': 'Organization', 'name': 'WFindApps', 'url': VALUES['hub'] + '/'},
        'codeRepository': GITHUB,
    }

def faq_json(body):
    items = []
    for q, a in re.findall(r'<details[^>]*>\s*<summary>(.*?)</summary>(.*?)</details>', body, re.S):
        answer = re.sub(r'\s+', ' ', re.sub(r'<[^>]+>', ' ', a)).strip()
        items.append({'@type': 'Question', 'name': html.unescape(re.sub(r'<[^>]+>', '', q)).strip(),
                      'acceptedAnswer': {'@type': 'Answer', 'text': html.unescape(answer)}})
    return {'@type': 'FAQPage', 'mainEntity': items}

LOGOS = {
    'wfindapps': '<img src="/img/logo.svg" width="30" height="30" alt="">',
    'wcopyfind': '<img src="/img/wcopyfind-icon.png" width="30" height="30" alt="">',
    'wrepeatfind': '<img src="/img/wrepeatfind-icon.png" width="30" height="30" alt="">',
}

def page_html(site_key, page, meta, body):
    site = SITES[site_key]
    base = 'https://' + site['host']
    url = base + '/' + (page[:-len('index.html')] if page.endswith('index.html') else page)
    title = meta['title']
    full_title = title if page == 'index.html' else f"{title} | {site['name']}"
    description = meta['description']
    nav_current = meta.get('nav', '')
    image = base + '/img/og.png'

    graph = []
    if page == 'index.html':
        if site_key == 'wfindapps':
            graph.append({'@type': 'Organization', 'name': 'WFindApps', 'url': base + '/', 'logo': base + '/img/logo.png',
                          'founder': PERSON, 'sameAs': [GITHUB],
                          'contactPoint': {'@type': 'ContactPoint', 'contactType': 'customer support', 'email': EMAIL, 'url': base + '/contact.html'}})
            graph.append({'@type': 'WebSite', 'name': 'WFindApps', 'url': base + '/', 'description': description})
            graph.append(software_json('wcopyfind'))
            graph.append(software_json('wrepeatfind'))
        else:
            graph.append(software_json(site_key))
    else:
        crumbs = [(site['name'], base + '/')]
        if page.startswith('essays/') and page != 'essays/index.html':
            crumbs.append(('Essays', base + '/essays/'))
        crumbs.append((title, url))
        graph.append(breadcrumb_json(site, crumbs))
    schema = meta.get('schema', '')
    if schema == 'faq':
        graph.append(faq_json(body))
    elif schema in ('article', 'howto'):
        article = {'@type': 'TechArticle' if schema == 'howto' else 'Article', 'headline': title, 'description': description,
                   'author': PERSON if meta.get('author', AUTHOR) == AUTHOR else {'@type': 'Person', 'name': meta['author']}, 'dateModified': TODAY, 'url': url,
                   'publisher': {'@type': 'Organization', 'name': 'WFindApps', 'url': VALUES['hub'] + '/'}}
        if meta.get('published'):
            article['datePublished'] = meta['published']
        graph.append(article)
    ld = json.dumps({'@context': 'https://schema.org', '@graph': graph}, indent=1, ensure_ascii=False)

    nav = '\n'.join(
        f'<a href="/{"" if href == "index.html" else href}"{" aria-current=\"page\"" if label == nav_current else ""}>{label}</a>'
        for label, href in site['nav'])
    download = ''
    if site_key != 'wfindapps':
        download = f'<a class="btn btn-small" href="{VALUES[site_key + "_download"]}">Download</a>'
    brand = f'<a class="brand" href="/">{LOGOS[site_key]}<span>{site["name"]}</span></a>'
    if site_key != 'wfindapps':
        brand = f'<a class="family" href="{VALUES["hub"]}/" title="WFindApps: all the apps">WFindApps</a><span class="slash">/</span>' + brand

    help_nav = ''
    if meta.get('help') == 'yes' and 'help' in site:
        items = '\n'.join(f'<li><a href="/{href}"{" aria-current=\"page\"" if href == page else ""}>{label}</a></li>' for label, href in site['help'])
        help_nav = f'<aside class="help-nav" aria-label="Help topics"><h2>Help</h2><ul>{items}</ul></aside>'
        body = f'<div class="with-help">{help_nav}<div class="help-body">{body}</div></div>'

    depth = page.count('/')
    return f'''<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(full_title)}</title>
<meta name="description" content="{html.escape(description)}">
<link rel="canonical" href="{url}">
<meta name="author" content="{AUTHOR}">
<meta name="robots" content="index, follow, max-image-preview:large">
<meta property="og:type" content="{"website" if page == "index.html" else "article"}">
<meta property="og:site_name" content="{site["name"]}">
<meta property="og:title" content="{html.escape(title)}">
<meta property="og:description" content="{html.escape(description)}">
<meta property="og:url" content="{url}">
<meta property="og:image" content="{image}">
<meta property="og:image:width" content="1200">
<meta property="og:image:height" content="630">
<meta name="twitter:card" content="summary_large_image">
<meta name="theme-color" content="#2458b3">
<link rel="icon" href="/favicon.ico" sizes="any">
<link rel="icon" href="/img/icon-192.png" type="image/png">
<link rel="apple-touch-icon" href="/img/icon-180.png">
<link rel="stylesheet" href="/site.css">
<link rel="alternate" type="text/plain" title="Summary for AI assistants" href="/llms.txt">
<script type="application/ld+json">
{ld}
</script>
</head>
<body class="site-{site_key}">
<a class="skip" href="#main">Skip to the content</a>
<header class="site-header">
<div class="bar">
<div class="brand-wrap">{brand}</div>
<nav class="main-nav" aria-label="Main">{nav}</nav>
{download}
</div>
</header>
<main id="main">
{body}
</main>
<footer class="site-footer">
<div class="cols">
<div>
<p class="foot-title"><a href="{VALUES["hub"]}/">WFindApps</a></p>
<p>Free, open-source Windows tools that find shared and repeated phrases in documents. Your documents never leave your computer.</p>
</div>
<div>
<p class="foot-title">The apps</p>
<ul>
<li><a href="{VALUES["wcf"]}/">WCopyfind</a>: compare documents with each other</li>
<li><a href="{VALUES["wrf"]}/">WRepeatfind</a>: find repetition within a document</li>
<li><a href="{VALUES["hub"]}/download.html">All downloads</a></li>
</ul>
</div>
<div>
<p class="foot-title">Help</p>
<ul>
<li><a href="{VALUES["wcf"]}/guide.html">WCopyfind user guide</a></li>
<li><a href="{VALUES["wrf"]}/guide.html">WRepeatfind user guide</a></li>
<li><a href="{VALUES["wcf"]}/faq.html">WCopyfind FAQ</a> · <a href="{VALUES["wrf"]}/faq.html">WRepeatfind FAQ</a></li>
<li><a href="{VALUES["hub"]}/contact.html">Report a problem</a></li>
</ul>
</div>
<div>
<p class="foot-title">About</p>
<ul>
<li><a href="{VALUES["hub"]}/about.html">About WFindApps</a></li>
<li><a href="{GITHUB}">Source code on GitHub</a></li>
<li><a href="{VALUES["hub"]}/privacy.html">Privacy</a></li>
<li><a href="{VALUES["license"]}">License: GNU GPL v3</a></li>
</ul>
</div>
</div>
<p class="copyright">© {VALUES["year"]} {AUTHOR}. WCopyfind and WRepeatfind are free software under the GNU General Public License, version 3 or later.</p>
</footer>
</body>
</html>
'''

def llms_txt(site_key, pages):
    site = SITES[site_key]
    base = 'https://' + site['host']
    lines = [f'# {site["name"]}', '', f'> {site["tagline"]}.', '']
    intro = open(os.path.join(ROOT, 'sites', site_key, 'llms-intro.md'), encoding='utf-8').read()
    lines.append(fill(intro).strip())
    lines += ['', '## Pages', '']
    for page, meta in pages:
        if page == '404.html':
            continue
        lines.append(f'- [{meta["title"]}]({base}/{"" if page == "index.html" else page}): {meta["description"]}')
    lines += ['', '## Related', '', f'- [WFindApps]({VALUES["hub"]}/): home of both apps',
              f'- [WCopyfind]({VALUES["wcf"]}/): compares documents with each other to find shared phrases (plagiarism detection)',
              f'- [WRepeatfind]({VALUES["wrf"]}/): finds phrases repeated within one document or book',
              f'- [Source code]({GITHUB}) (GNU GPL v3 or later)', '']
    return '\n'.join(lines)

def build_site(site_key):
    site = SITES[site_key]
    src = os.path.join(ROOT, 'sites', site_key)
    out = os.path.join(DIST, site['host'])
    if os.path.isdir(out):
        shutil.rmtree(out)
    os.makedirs(out)

    pages = []
    pages_dir = os.path.join(src, 'pages')
    for folder, _, files in os.walk(pages_dir):
        for name in sorted(files):
            if not name.endswith('.html'):
                continue
            path = os.path.join(folder, name)
            page = os.path.relpath(path, pages_dir).replace(os.sep, '/')
            meta, body = parse_page(path)
            target = os.path.join(out, page)
            os.makedirs(os.path.dirname(target), exist_ok=True)
            open(target, 'w', encoding='utf-8', newline='\n').write(page_html(site_key, page, meta, body))
            pages.append((page, meta))

    shutil.copy(os.path.join(ROOT, 'shared', 'site.css'), os.path.join(out, 'site.css'))
    shutil.copytree(os.path.join(ROOT, 'shared', 'img'), os.path.join(out, 'img'))
    site_img = os.path.join(src, 'img')
    if os.path.isdir(site_img):
        shutil.copytree(site_img, os.path.join(out, 'img'), dirs_exist_ok=True)
    shutil.copy(os.path.join(src, 'favicon.ico'), os.path.join(out, 'favicon.ico'))

    # the site's own downloads (such as older versions of the program), then the sample documents
    site_downloads = os.path.join(src, 'downloads')
    if os.path.isdir(site_downloads):
        shutil.copytree(site_downloads, os.path.join(out, 'downloads'), dirs_exist_ok=True)

    # sample documents to download
    samples = os.path.join(ROOT, 'samples', site_key)
    if os.path.isdir(samples):
        os.makedirs(os.path.join(out, 'downloads'), exist_ok=True)
        zip_path = os.path.join(out, 'downloads', f'{site["name"]}-Samples.zip')
        with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as z:
            for folder, _, files in os.walk(samples):
                for name in sorted(files):
                    full = os.path.join(folder, name)
                    z.write(full, os.path.join(f'{site["name"]} Samples', os.path.relpath(full, samples)))

    base = 'https://' + site['host']
    urls = [f'<url><loc>{base}/{"" if p == "index.html" else p.replace("index.html", "")}</loc><lastmod>{TODAY}</lastmod></url>'
            for p, m in pages if p != '404.html']
    open(os.path.join(out, 'sitemap.xml'), 'w', encoding='utf-8', newline='\n').write(
        '<?xml version="1.0" encoding="UTF-8"?>\n<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">\n' + '\n'.join(urls) + '\n</urlset>\n')
    open(os.path.join(out, 'robots.txt'), 'w', encoding='utf-8', newline='\n').write(
        f'# Everyone, including search engines and AI assistants, is welcome to read this site.\nUser-agent: *\nAllow: /\n\nSitemap: {base}/sitemap.xml\n')
    open(os.path.join(out, 'llms.txt'), 'w', encoding='utf-8', newline='\n').write(llms_txt(site_key, pages))
    shutil.copy(os.path.join(ROOT, 'shared', 'htaccess'), os.path.join(out, '.htaccess'))
    return len(pages)

if __name__ == '__main__':
    for key in SITES:
        count = build_site(key)
        print(f'{SITES[key]["host"]}: {count} pages')
    print('Built into', DIST)

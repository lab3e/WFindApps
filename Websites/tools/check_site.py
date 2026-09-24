# check_site.py : checks the built sites for broken internal links, missing images, duplicate or missing titles
# and descriptions, unfilled {{placeholders}}, and HTML tags left unclosed.
#
#   python tools/check_site.py

import os, re, sys
from html.parser import HTMLParser

DIST = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'dist')
HOSTS = {'wfindapps.org', 'wcopyfind.wfindapps.org', 'wrepeatfind.wfindapps.org'}
VOID = {'meta', 'link', 'img', 'br', 'hr', 'input', 'source', 'path', 'rect', 'circle'}

class Page(HTMLParser):
    def __init__(self):
        super().__init__()
        self.links, self.ids, self.stack, self.errors, self.title, self.desc = [], set(), [], [], '', ''
        self._in_title = False
    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        if 'id' in a: self.ids.add(a['id'])
        for key in ('href', 'src'):
            if key in a and tag in ('a', 'img', 'link', 'script'): self.links.append(a[key])
        if tag == 'meta' and a.get('name') == 'description': self.desc = a.get('content', '')
        if tag == 'title': self._in_title = True
        if tag not in VOID: self.stack.append((tag, self.getpos()))
    def handle_startendtag(self, tag, attrs):
        self.handle_starttag(tag, attrs)
        if tag not in VOID and self.stack: self.stack.pop()
    def handle_endtag(self, tag):
        if tag == 'title': self._in_title = False
        if tag in VOID: return
        if self.stack and self.stack[-1][0] == tag: self.stack.pop()
        else:
            self.errors.append(f'unexpected </{tag}> at line {self.getpos()[0]} (open: {[t for t, _ in self.stack[-3:]]})')
            for i in range(len(self.stack) - 1, -1, -1):
                if self.stack[i][0] == tag:
                    del self.stack[i:]
                    break
    def handle_data(self, data):
        if self._in_title: self.title += data

problems = 0
def report(msg):
    global problems
    problems += 1
    print(msg)

pages = {}
for host in sorted(HOSTS):
    root = os.path.join(DIST, host)
    for folder, _, files in os.walk(root):
        for name in files:
            if name.endswith('.html'):
                path = os.path.join(folder, name)
                rel = '/' + os.path.relpath(path, root).replace(os.sep, '/')
                text = open(path, encoding='utf-8').read()
                p = Page(); p.feed(text); p.close()
                pages[(host, rel)] = p
                if '{{' in text: report(f'{host}{rel}: unfilled placeholder')
                for e in p.errors: report(f'{host}{rel}: {e}')
                if p.stack: report(f'{host}{rel}: unclosed {[t for t, _ in p.stack]}')
                if not p.desc: report(f'{host}{rel}: no description')
    for name in ('llms.txt', 'sitemap.xml', 'robots.txt'):
        text = open(os.path.join(root, name), encoding='utf-8').read()
        if '{{' in text: report(f'{host}/{name}: unfilled placeholder')

def exists(host, path):
    root = os.path.join(DIST, host)
    target = os.path.join(root, path.lstrip('/'))
    if path.endswith('/'): target = os.path.join(target, 'index.html')
    return os.path.isfile(target)

titles = {}
for (host, rel), p in pages.items():
    titles.setdefault((host, p.title), []).append(rel)
    for link in p.links:
        m = re.match(r'https?://([^/]+)(/[^#?]*)?(#.*)?$', link)
        if m and m.group(1) in HOSTS:
            h, path, frag = m.group(1), m.group(2) or '/', m.group(3)
        elif link.startswith(('http:', 'https:', 'mailto:')):
            continue
        else:
            h = host
            path, _, frag = link.partition('#')
            frag = '#' + frag if frag else None
            if not path: path = rel
            elif not path.startswith('/'): path = os.path.normpath(os.path.join(os.path.dirname(rel), path)).replace(os.sep, '/')
        if not exists(h, path):
            report(f'{host}{rel}: broken link {link}')
        elif frag and len(frag) > 1:
            target = pages.get((h, path if not path.endswith('/') else path + 'index.html'))
            if target is not None and frag[1:] not in target.ids:
                report(f'{host}{rel}: missing anchor {link}')
for (host, title), rels in titles.items():
    if len(rels) > 1 and '404' not in ''.join(rels): report(f'{host}: duplicate title "{title}" on {rels}')

print(f'{len(pages)} pages checked, {problems} problems')
sys.exit(1 if problems else 0)

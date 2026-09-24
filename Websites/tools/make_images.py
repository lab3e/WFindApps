# make_images.py : makes the sites' logo, icons, screenshots, and link-preview images
#
#   python tools/make_images.py <folder with the raw screenshots>
#
# Raw screenshots (PNG, from the programs and their reports) are expected under these names:
#   shot-wcopyfind.png, shot-wcopyfind-options.png, shot-wcf-index.png, shot-wcf-pair.png,
#   shot-wrepeatfind.png, shot-wrf-report.png

import os, sys
from PIL import Image, ImageDraw, ImageFont, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
WEB = os.path.dirname(HERE)
REPO = os.path.dirname(WEB)
SHARED = os.path.join(WEB, 'shared', 'img')
SITES = os.path.join(WEB, 'sites')
FONTS = r'C:\Windows\Fonts'
BLUE = (36, 88, 179)
AMBER = (255, 209, 102)
PAPER = (253, 252, 249)
INK = (29, 29, 31)

def font(name, size):
    return ImageFont.truetype(os.path.join(FONTS, name), size)

def logo(size):
    """The WFindApps mark: a page of text with one highlighted line, under a magnifying glass."""
    s = 8
    n = size * s
    im = Image.new('RGBA', (n, n), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    u = n / 64
    d.rounded_rectangle([0, 0, n - 1, n - 1], radius=14 * u, fill=BLUE)
    white = (255, 255, 255, 225)
    d.rounded_rectangle([12 * u, 15 * u, 44 * u, 20 * u], radius=2.5 * u, fill=white)
    d.rounded_rectangle([12 * u, 26 * u, 36 * u, 31 * u], radius=2.5 * u, fill=AMBER)
    d.rounded_rectangle([12 * u, 37 * u, 30 * u, 42 * u], radius=2.5 * u, fill=white)
    d.rounded_rectangle([12 * u, 48 * u, 26 * u, 53 * u], radius=2.5 * u, fill=white)
    d.ellipse([29 * u, 23 * u, 51 * u, 45 * u], outline=(255, 255, 255, 255), width=int(4.5 * u))
    d.line([47 * u, 42 * u, 55 * u, 50 * u], fill=(255, 255, 255, 255), width=int(5.5 * u))
    d.ellipse([52.3 * u, 47.3 * u, 57.7 * u, 52.7 * u], fill=(255, 255, 255, 255))
    return im.resize((size, size), Image.LANCZOS)

LOGO_SVG = '''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" role="img" aria-label="WFindApps">
<rect width="64" height="64" rx="14" fill="#2458b3"/>
<g fill="#fff" fill-opacity=".88"><rect x="12" y="15" width="32" height="5" rx="2.5"/><rect x="12" y="37" width="18" height="5" rx="2.5"/><rect x="12" y="48" width="14" height="5" rx="2.5"/></g>
<rect x="12" y="26" width="24" height="5" rx="2.5" fill="#ffd166"/>
<circle cx="40" cy="34" r="11" fill="none" stroke="#fff" stroke-width="4.5"/>
<path d="M47 42 L55 50" stroke="#fff" stroke-width="5.5" stroke-linecap="round"/>
</svg>
'''

def app_icon(ico, size):
    im = Image.open(ico)
    im.size = (256, 256)
    return im.convert('RGBA').resize((size, size), Image.LANCZOS)

def screenshot(raw, out, width):
    im = Image.open(raw).convert('RGB')
    if im.width > width:
        im = im.resize((width, round(im.height * width / im.width)), Image.LANCZOS)
    im.save(out, optimize=True)
    return im

def rounded(im, radius):
    mask = Image.new('L', im.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, im.width - 1, im.height - 1], radius=radius, fill=255)
    out = Image.new('RGBA', im.size)
    out.paste(im, (0, 0), mask)
    return out

def wrap(draw, text, fnt, width):
    words, lines, line = text.split(), [], ''
    for word in words:
        trial = (line + ' ' + word).strip()
        if draw.textlength(trial, font=fnt) <= width or not line:
            line = trial
        else:
            lines.append(line)
            line = word
    return lines + [line]

def og_image(out, mark, name, lines, shot):
    """A 1200 x 630 link-preview image: name and tagline on the left, a screenshot on the right."""
    W, H, TEXT_W = 1200, 630, 540
    im = Image.new('RGB', (W, H), PAPER)
    d = ImageDraw.Draw(im)
    d.rectangle([0, 0, W, 10], fill=BLUE)
    mark = mark.resize((96, 96), Image.LANCZOS)
    im.paste(mark, (64, 84), mark)
    d.text((64 + 96 + 22, 92), name, font=font('segoeuib.ttf', 60), fill=INK)
    y = 232
    for text, size, colour in lines:
        fnt = font('segoeui.ttf', size)
        for line in wrap(d, text, fnt, TEXT_W):
            d.text((66, y), line, font=fnt, fill=colour)
            y += int(size * 1.35)
        y += int(size * 0.35)
    d.text((66, H - 80), 'Free for Windows  ·  Open source  ·  Private', font=font('segoeuisl.ttf', 26), fill=(93, 96, 104))
    if shot is not None:
        s = Image.open(shot).convert('RGB')
        box_w, box_h = 520, 500
        scale = min(box_w / s.width, box_h / s.height)
        s = rounded(s.resize((round(s.width * scale), round(s.height * scale)), Image.LANCZOS), 12)
        x, y = W - 60 - s.width, (H - s.height) // 2 + 6
        shadow = Image.new('RGBA', (s.width + 60, s.height + 60), (0, 0, 0, 0))
        ImageDraw.Draw(shadow).rounded_rectangle([30, 34, s.width + 30, s.height + 34], radius=12, fill=(0, 0, 0, 70))
        shadow = shadow.filter(ImageFilter.GaussianBlur(14))
        im.paste(shadow, (x - 30, y - 30), shadow)
        im.paste(s, (x, y), s)
    im.save(out, optimize=True)

def main(raw):
    os.makedirs(SHARED, exist_ok=True)
    for key in ('wfindapps', 'wcopyfind', 'wrepeatfind'):
        os.makedirs(os.path.join(SITES, key, 'img'), exist_ok=True)

    open(os.path.join(SHARED, 'logo.svg'), 'w', encoding='utf-8').write(LOGO_SVG)
    logo(512).save(os.path.join(SHARED, 'logo.png'))
    wcf_ico = os.path.join(REPO, 'WCopyfind', 'res', 'WCopyfind.ico')
    wrf_ico = os.path.join(REPO, 'WRepeatfind', 'res', 'WRepeatfind.ico')
    app_icon(wcf_ico, 128).save(os.path.join(SHARED, 'wcopyfind-icon.png'))
    app_icon(wrf_ico, 128).save(os.path.join(SHARED, 'wrepeatfind-icon.png'))

    # icons for each site: the hub uses the WFindApps mark, each app site its program's icon
    for key, source in (('wfindapps', None), ('wcopyfind', wcf_ico), ('wrepeatfind', wrf_ico)):
        folder = os.path.join(SITES, key)
        for size in (180, 192):
            (logo(size) if source is None else app_icon(source, size)).save(os.path.join(folder, 'img', f'icon-{size}.png'))
        ico = os.path.join(folder, 'favicon.ico')
        if source is None:
            logo(256).save(ico, sizes=[(16, 16), (32, 32), (48, 48), (64, 64)])
        else:
            app_icon(source, 256).save(ico, sizes=[(16, 16), (32, 32), (48, 48), (64, 64)])

    shots = {
        'wcopyfind-window.png': ('shot-wcopyfind.png', 1144), 'wcopyfind-options.png': ('shot-wcopyfind-options.png', 602),
        'wcopyfind-report-index.png': ('shot-wcf-index.png', 1400), 'wcopyfind-report-pair.png': ('shot-wcf-pair.png', 1400),
        'wrepeatfind-window.png': ('shot-wrepeatfind.png', 1064), 'wrepeatfind-report.png': ('shot-wrf-report.png', 1100),
    }
    for out, (name, width) in shots.items():
        screenshot(os.path.join(raw, name), os.path.join(SHARED, out), width)

    grey = (93, 96, 104)
    og_image(os.path.join(SITES, 'wfindapps', 'img', 'og.png'), logo(256), 'WFindApps',
             [('Free tools that find shared and repeated phrases in documents', 38, INK),
              ('WCopyfind  ·  WRepeatfind', 28, BLUE)], os.path.join(SHARED, 'wcopyfind-report-pair.png'))
    og_image(os.path.join(SITES, 'wcopyfind', 'img', 'og.png'), app_icon(wcf_ico, 256), 'WCopyfind',
             [('Free plagiarism detection: find the phrases documents share', 38, INK),
              ('Compare hundreds of documents in seconds', 27, grey)], os.path.join(SHARED, 'wcopyfind-window.png'))
    og_image(os.path.join(SITES, 'wrepeatfind', 'img', 'og.png'), app_icon(wrf_ico, 256), 'WRepeatfind',
             [('Find the phrases your writing repeats', 38, INK),
              ('For articles, essays, and books', 27, grey)], os.path.join(SHARED, 'wrepeatfind-report.png'))
    print('images written')

if __name__ == '__main__':
    main(sys.argv[1])

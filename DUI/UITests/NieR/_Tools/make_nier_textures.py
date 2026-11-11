"""Procedural textures for the NieR-style UI test screens (measured from 1920x1080 reference screenshots).

    python nier_make_textures.py <out dir>

Everything is drawn here; nothing is taken from the game. Writes:
  NieR_T_MenuBackground.png  1920x1080  cream base, 7.2 px grid, faint diagonals and arcs, vignette
  NieR_T_BandTop.png         1920x40    the top border: 2 px line, a dash and a dot triangle every 77 px
  NieR_T_BandBottom.png      1920x40    the bottom border (mirrored)
  NieR_T_Cursor.png          64x32      the pointer beside the selected row
  NieR_T_Icon_<Name>.png     48x48      white tab icons, tinted in the UI
  NieR_T_TitleBackground.png 1920x1080  dark blue night with a pale glow and drifting motes
  NieR_T_Square.png          16x16      a white square (markers, key caps)
"""
import math
import os
import random
import sys

from PIL import Image, ImageDraw, ImageFilter

OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), 'Textures')
os.makedirs(OUT, exist_ok=True)
W, H = 1920, 1080
SS = 4  # supersampling for anti-aliased shapes


def save(img, name):
    img.save(os.path.join(OUT, name))
    print('wrote', name, img.size)


def menu_background():
    base = (238, 233, 194)
    img = Image.new('RGB', (W, H), base)
    px = img.load()
    # 7.2 px grid, 2 px lines, about 6 luminance units darker.
    pitch = 7.2
    grid = Image.new('L', (W, H), 0)
    gd = ImageDraw.Draw(grid)
    x = 3.0
    while x < W:
        gd.rectangle([int(x), 0, int(x) + 1, H], fill=255)
        x += pitch
    y = 0.5
    while y < H:
        gd.rectangle([0, int(y), W, int(y) + 1], fill=255)
        y += pitch
    # Faint diagonals and large arcs, drawn supersampled.
    deco = Image.new('L', (W * SS // 2, H * SS // 2), 0)
    dd = ImageDraw.Draw(deco)
    s = SS // 2
    for x0, w in ((380, 3), (430, 2), (900, 2), (1010, 3), (1500, 2), (1560, 2)):
        dd.line([(x0 * s, 0), ((x0 + 1100) * s, 1100 * s)], fill=200, width=w * s // 2 + 1)
    for cx, cy, r, w in ((1650, 1150, 560, 3), (1650, 1150, 610, 2), (240, 120, 330, 2), (1180, 380, 700, 2)):
        dd.ellipse([(cx - r) * s, (cy - r) * s, (cx + r) * s, (cy + r) * s], outline=170, width=w * s // 2 + 1)
    deco = deco.resize((W, H), Image.LANCZOS)
    g = grid.load()
    d = deco.load()
    for yy in range(H):
        v = (yy - H / 2) / (H / 2)
        for xx in range(W):
            u = (xx - W / 2) / (W / 2)
            r = math.sqrt(u * u + v * v)
            f = 1.0 - 0.45 * max(0.0, r - 0.6)
            dark = 6.0 * g[xx, yy] / 255.0 + 14.0 * d[xx, yy] / 255.0
            px[xx, yy] = tuple(int(max(0, min(255, (c - dark * c / 238.0) * f))) for c in base)
    return img


def band(bottom):
    h = 40
    img = Image.new('RGBA', (W * SS, h * SS), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    ink = (60, 55, 42, 255)
    line = (78, 74, 61, 255)

    def Y(y):
        return (h - 1 - y) if bottom else y

    def rect(x0, y0, x1, y1, c):
        ya, yb = sorted((Y(y0), Y(y1)))
        d.rectangle([x0 * SS, ya * SS, (x1 + 1) * SS - 1, (yb + 1) * SS - 1], fill=c)

    def dot(cx, cy, r, c):
        cy = Y(cy)
        d.ellipse([(cx - r) * SS, (cy - r) * SS, (cx + r) * SS, (cy + r) * SS], fill=c)

    # The 2 px line at the band's edge (y 36-37 of the screen; here 2-3), then per 77 px: a 13x4 dash hanging from it
    # and a triangle of 6 px dots (two above, one between and below).
    rect(0, 2, W, 3, line)
    x = 24.0
    while x < W + 77:
        rect(int(x) + 42, 4, int(x) + 54, 7, ink)
        dot(x + 2.5, 13.0, 3.0, ink)
        dot(x + 17.5, 13.0, 3.0, ink)
        dot(x + 10.0, 25.0, 3.0, ink)
        x += 77.0
    return img.resize((W, h), Image.LANCZOS)


def cursor():
    w, h = 64, 32
    img = Image.new('RGBA', (w * SS, h * SS), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    c = (255, 255, 255, 255)
    # A pointed lozenge with a slot, and two dots behind it.
    d.polygon([(4 * SS, 16 * SS), (16 * SS, 9 * SS), (38 * SS, 13 * SS), (38 * SS, 19 * SS), (16 * SS, 23 * SS)], fill=c)
    d.ellipse([17 * SS, 13 * SS, 23 * SS, 19 * SS], fill=(0, 0, 0, 0))
    d.ellipse([44 * SS, 9 * SS, 50 * SS, 15 * SS], fill=c)
    d.ellipse([44 * SS, 18 * SS, 50 * SS, 24 * SS], fill=c)
    return img.resize((w, h), Image.LANCZOS)


def icon(kind):
    n = 48
    img = Image.new('RGBA', (n * SS, n * SS), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    c = (255, 255, 255, 255)
    k = (0, 0, 0, 0)
    S = SS

    def R(x0, y0, x1, y1, col=c):
        d.rectangle([x0 * S, y0 * S, x1 * S, y1 * S], fill=col)

    def P(pts, col=c):
        d.polygon([(x * S, y * S) for x, y in pts], fill=col)

    def E(x0, y0, x1, y1, col=c, width=0):
        if width:
            d.ellipse([x0 * S, y0 * S, x1 * S, y1 * S], outline=col, width=width * S)
        else:
            d.ellipse([x0 * S, y0 * S, x1 * S, y1 * S], fill=col)

    # Every icon sits in a square frame, as the tabs' icons do.
    R(4, 4, 44, 44)
    R(8, 8, 40, 40, k)
    if kind == 'Map':
        P([(24, 10), (38, 24), (24, 38), (10, 24)])
        P([(24, 16), (32, 24), (24, 32), (16, 24)], k)
        R(21, 21, 27, 27)
    elif kind == 'Quests':
        P([(24, 10), (38, 24), (24, 38), (10, 24)])
        P([(24, 15), (33, 24), (24, 33), (15, 24)], k)
        R(22, 17, 26, 27)
        R(22, 29, 26, 32)
    elif kind == 'Items':
        R(14, 11, 34, 15)
        R(16, 17, 32, 21)
        R(19, 23, 29, 37)
        R(21, 25, 27, 35, k)
    elif kind == 'Weapons':
        P([(24, 9), (28, 30), (24, 34), (20, 30)])
        R(15, 30, 33, 33)
        R(22, 33, 26, 40)
    elif kind == 'Skills':
        R(12, 12, 22, 22)
        R(26, 12, 36, 22)
        R(12, 26, 22, 36)
        R(26, 26, 36, 36)
        R(15, 15, 19, 19, k)
        R(29, 29, 33, 33, k)
    elif kind == 'Intel':
        R(13, 11, 35, 37)
        R(16, 14, 32, 34, k)
        R(18, 17, 30, 19)
        R(18, 22, 30, 24)
        R(18, 27, 26, 29)
    elif kind == 'System':
        E(12, 12, 36, 36)
        E(16, 16, 32, 32, k)
        E(20, 20, 28, 28)
        for a in range(0, 360, 45):
            x = 24 + 13 * math.cos(math.radians(a))
            y = 24 + 13 * math.sin(math.radians(a))
            R(x - 2.5, y - 2.5, x + 2.5, y + 2.5)
    return img.resize((n, n), Image.LANCZOS)


def title_background():
    rnd = random.Random(7)
    img = Image.new('RGB', (W, H), (0, 0, 0))
    px = img.load()
    top = (14, 20, 40)
    mid = (34, 44, 70)
    for y in range(H):
        t = y / (H - 1)
        # Darker sky, a hazy band two thirds down, darker again at the bottom.
        k = math.exp(-((t - 0.62) / 0.22) ** 2)
        col = tuple(int(a + (b - a) * k) for a, b in zip(top, mid))
        for x in range(W):
            u = (x - W / 2) / (W / 2)
            f = 1.0 - 0.35 * u * u
            px[x, y] = tuple(int(c * f) for c in col)
    glow = Image.new('L', (W, H), 0)
    gd = ImageDraw.Draw(glow)
    # A pale field of light low on the screen, as the white flowers make it.
    for _ in range(2600):
        x = rnd.uniform(-50, W + 50)
        y = rnd.gauss(H * 0.66, H * 0.07)
        r = rnd.uniform(1.0, 4.5)
        gd.ellipse([x - r, y - r * 0.6, x + r, y + r * 0.6], fill=int(rnd.uniform(60, 200)))
    glow = glow.filter(ImageFilter.GaussianBlur(2.2))
    motes = Image.new('L', (W, H), 0)
    md = ImageDraw.Draw(motes)
    for _ in range(90):
        x = rnd.uniform(0, W)
        y = rnd.uniform(0, H * 0.6)
        r = rnd.uniform(1.5, 3.5)
        md.ellipse([x - r, y - r, x + r, y + r], fill=int(rnd.uniform(120, 255)))
    motes = motes.filter(ImageFilter.GaussianBlur(1.6))
    tint = (205, 225, 220)
    g = glow.load()
    m = motes.load()
    for y in range(H):
        for x in range(W):
            a = min(1.0, (g[x, y] * 0.55 + m[x, y]) / 255.0)
            c = px[x, y]
            px[x, y] = tuple(int(cc + (tc - cc) * a) for cc, tc in zip(c, tint))
    return img


def hexagon():
    n = 32
    img = Image.new('RGBA', (n * SS, n * SS), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    pts = [((16 + 14 * math.cos(math.radians(a))) * SS, (16 + 14 * math.sin(math.radians(a))) * SS) for a in range(30, 390, 60)]
    d.polygon(pts, fill=(255, 255, 255, 255))
    return img.resize((n, n), Image.LANCZOS)


def square():
    return Image.new('RGBA', (16, 16), (255, 255, 255, 255))


save(menu_background(), 'NieR_T_MenuBackground.png')
save(band(False), 'NieR_T_BandTop.png')
save(band(True), 'NieR_T_BandBottom.png')
save(cursor(), 'NieR_T_Cursor.png')
for name in ('Map', 'Quests', 'Items', 'Weapons', 'Skills', 'Intel', 'System'):
    save(icon(name), 'NieR_T_Icon_%s.png' % name)
save(title_background(), 'NieR_T_TitleBackground.png')
save(square(), 'NieR_T_Square.png')
save(hexagon(), 'NieR_T_Hex.png')

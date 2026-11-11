"""Reference screenshot beside the study's still, one row per screen, into Saved/Takes/compare.png.
    python compare.py <take name> < /dev/null
"""
import os
import sys
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
REF = os.path.join(ROOT, "Plan", "P3R", "ref", "article")
TAKES = os.path.join(ROOT, "Saved", "Takes")
take = sys.argv[1] if len(sys.argv) > 1 else "p3r-study-tour-v13"

ROWS = [  # (label, reference, still time)
    ("主菜单", "a04.jpg", "4.0"),
    ("STATUS", "a02.jpg", "7.2"),
    ("COMMU", "a03.jpg", "12.5"),
    ("SYSTEM", "a05.jpg", "17.0"),
    ("早期稿 A", "a08.jpg", "20.3"),
    ("早期稿 B", "a07.jpg", "22.5"),
]
W, H = 800, 450
sheet = Image.new("RGB", (W * 2, H * len(ROWS)), "black")
draw = ImageDraw.Draw(sheet)
for row, (label, ref, t) in enumerate(ROWS):
    for col, path in enumerate((os.path.join(REF, ref), os.path.join(TAKES, f"{take}-{t}s.png"))):
        if os.path.exists(path):
            sheet.paste(Image.open(path).convert("RGB").resize((W, H), Image.LANCZOS), (col * W, row * H))
    draw.rectangle([0, row * H, 150, row * H + 22], fill="black")
    draw.text((6, row * H + 5), f"{ref} / {t}s", fill="yellow")
out = os.path.join(TAKES, "compare.png")
sheet.save(out)
print(out)

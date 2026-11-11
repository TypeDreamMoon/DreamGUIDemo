"""Tile up to four stills 2x2 (960x540 each) into one PNG for a quick look.
    python sheet.py out.png a.png b.png c.png d.png < /dev/null
"""
import sys
from PIL import Image, ImageDraw

out, files = sys.argv[1], sys.argv[2:6]
sheet = Image.new("RGB", (1920, 1080), "black")
draw = ImageDraw.Draw(sheet)
for k, f in enumerate(files):
    im = Image.open(f).convert("RGB").resize((960, 540), Image.LANCZOS)
    x, y = (k % 2) * 960, (k // 2) * 540
    sheet.paste(im, (x, y))
    draw.rectangle([x, y, x + 200, y + 18], fill="black")
    draw.text((x + 4, y + 3), f.replace("\\", "/").split("/")[-1], fill="yellow")
sheet.save(out)

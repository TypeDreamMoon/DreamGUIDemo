"""Reference above, our still below, one image per screen: Saved/fr/pair_<ref>.png (1280 wide)."""
import os
import sys
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
REF = os.path.join(ROOT, "Plan", "P3R", "ref", "article")
TAKES = os.path.join(ROOT, "Saved", "Takes")
OUT = os.path.join(ROOT, "Saved", "fr")
os.makedirs(OUT, exist_ok=True)
take = sys.argv[1] if len(sys.argv) > 1 else "p3r-study-tour-v2"
ROWS = [("a04.jpg", "4.0"), ("a02.jpg", "7.2"), ("a03.jpg", "12.5"), ("a05.jpg", "17.0"), ("a08.jpg", "20.3"), ("a07.jpg", "22.5")]
W, H = 1280, 720
for ref, t in ROWS:
    sheet = Image.new("RGB", (W, H * 2), "black")
    for i, path in enumerate((os.path.join(REF, ref), os.path.join(TAKES, f"{take}-{t}s.png"))):
        if os.path.exists(path):
            sheet.paste(Image.open(path).convert("RGB").resize((W, H), Image.LANCZOS), (0, i * H))
    sheet.save(os.path.join(OUT, f"pair_{ref[:3]}.png"))
print("ok")

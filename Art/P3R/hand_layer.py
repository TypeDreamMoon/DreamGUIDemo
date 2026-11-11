"""The hand in front of the STATUS close-up, cut out as its own layer (imported by prepare_art.py; run alone to preview).

    python hand_layer.py < /dev/null      -> Saved/fr/hand_layer.png

In the game the chosen member's eyes show in a shard of glass framed by the protagonist's hand: fingertips above, palm
below. So the picture is drawn in two layers with the shard between them -- the whole close-up behind, and over the
shard only the hand. The hand is the picture's large pure-white pieces inside a loose outline of it (the hair's light
strands are lavender, not white, so they stay behind), closed over its shading and dilated over its ink lines.
"""
import os
import numpy as np
import cv2
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))

# A loose outline of the hand, in the 1448 x 1086 picture's pixels: wrist at the left, fingertips over the hairline,
# thumb along the brow. It only bounds the search; the white pieces inside it are the hand.
OUTLINE = [(250, 560), (300, 330), (420, 150), (500, 70), (640, 40), (760, 40), (920, 60), (930, 230), (1000, 330),
           (990, 400), (720, 470), (560, 540), (420, 620), (300, 640)]


def hand_layer(im):
    rgba = np.asarray(im.convert("RGBA")).astype(np.float32) / 255.0
    rgb, alpha = rgba[..., :3], rgba[..., 3]
    h, w = alpha.shape
    sx, sy = w / 1448.0, h / 1086.0
    region = np.zeros((h, w), np.uint8)
    cv2.fillPoly(region, [np.array([(x * sx, y * sy) for x, y in OUTLINE], np.int32)], 1)
    mx, mn = rgb.max(-1), rgb.min(-1)
    white = ((mn > 0.86) & (mx - mn < 0.12) & (alpha > 0.5) & (region > 0)).astype(np.uint8)
    count, labels, stats, _ = cv2.connectedComponentsWithStats(white, 8)
    keep = np.zeros_like(white)
    for index in range(1, count):
        if stats[index, cv2.CC_STAT_AREA] >= 1200 * sx * sy:
            keep[labels == index] = 1
    radius = max(3, round(9 * sx))
    mask = cv2.morphologyEx(keep, cv2.MORPH_CLOSE, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (2 * radius + 1, 2 * radius + 1)))
    mask = cv2.dilate(mask, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (7, 7))) & (region | cv2.dilate(region, np.ones((15, 15), np.uint8)))
    soft = cv2.GaussianBlur(mask.astype(np.float32), (3, 3), 0.8)
    out = np.dstack([rgb, alpha * soft])
    return Image.fromarray((np.clip(out, 0, 1) * 255 + 0.5).astype(np.uint8), "RGBA")


def preview():
    src = Image.open(os.path.join(HERE, "Final", "T_P3R_HeroHand.png")).convert("RGBA")
    layer = hand_layer(src)
    ground = Image.new("RGBA", src.size, (200, 40, 60, 255))
    ground.alpha_composite(layer)
    both = Image.new("RGB", (src.width, src.height * 2))
    back = Image.new("RGBA", src.size, (24, 80, 220, 255))
    back.alpha_composite(src)
    both.paste(back.convert("RGB"), (0, 0))
    both.paste(ground.convert("RGB"), (0, src.height))
    both.resize((src.width // 2, src.height)).save(os.path.join(HERE, "..", "..", "Saved", "fr", "hand_layer.png"))
    print("ok")


if __name__ == "__main__":
    preview()

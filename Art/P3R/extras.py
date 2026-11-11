"""Smaller pieces cut or drawn for the menus (imported by prepare_art.py; run alone to preview in Saved/fr):

- T_P3R_Tarot_0..5 and T_P3R_TarotBack: the bonds' cards, cut from one generated sheet (our own deck design), each
  resized to the card the protagonist holds on COMMU (0.62 wide to tall).
- T_P3R_HeroCardFront: the fingers holding that card, cut out of the COMMU picture as a layer in front of the UI card,
  so the card sits in his hand rather than on it.
- T_P3R_BoardMask / T_P3R_SideMask: the two flat polygons of the first early design, as alpha masks over the whole
  1920 x 1080 frame -- the street is blurred inside them (a texture clip mask on a BackgroundBlur).
"""
import os
import numpy as np
import cv2
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
GEN = os.path.join(HERE, "Gen")

TAROT_SHEET = "V3/gpt-image-1794366502253-1.png"
CARD_SIZE = (256, 412)

# The early design's two polygons, in the 1920 x 1080 frame (measured off the article's screenshot).
BOARD = [(248, 528), (1530, 165), (1740, 525), (960, 1102), (458, 975)]
SIDE = [(1665, 480), (1762, 318), (1935, 300), (1935, 1110), (1260, 1110)]

# The COMMU picture (1106 x 1422): a loose outline of the hand holding the card, and the card itself.
CARD_HAND = [(262, 318), (390, 322), (470, 380), (560, 470), (600, 600), (610, 720), (430, 720), (300, 570), (240, 420)]
CARD_RECT = (368, 222, 524, 476)


def tarot_cards():
    """The seven cards of the sheet, in reading order: 0, I, IV, V, VII, IX, then the back."""
    sheet = np.asarray(Image.open(os.path.join(GEN, TAROT_SHEET)).convert("RGB"))
    lit = (sheet.max(-1) > 40).astype(np.uint8)
    lit = cv2.dilate(lit, np.ones((5, 5), np.uint8))
    count, labels, stats, _ = cv2.connectedComponentsWithStats(lit, 8)
    boxes = sorted((stats[i] for i in range(1, count)), key=lambda s: -s[cv2.CC_STAT_AREA])[:7]
    boxes = sorted(boxes, key=lambda s: (round(s[1] / (sheet.shape[0] / 3)), s[0]))
    cards = []
    for s in boxes:
        x, y, w, h = s[0], s[1], s[2], s[3]
        inset = 2
        crop = Image.fromarray(sheet[y + inset:y + h - inset, x + inset:x + w - inset])
        cards.append(crop.resize(CARD_SIZE, Image.LANCZOS))
    return cards


def cut_hand(im, outline, base, exclude=None, min_area=900):
    """The large pure-white pieces of a flat picture inside a loose outline, closed over their shading and dilated over
    their ink lines; a piece whose bounds match `exclude` (x0, y0, x1, y1) is left out."""
    rgba = np.asarray(im.convert("RGBA")).astype(np.float32) / 255.0
    rgb, alpha = rgba[..., :3], rgba[..., 3]
    h, w = alpha.shape
    sx, sy = w / base[0], h / base[1]
    region = np.zeros((h, w), np.uint8)
    cv2.fillPoly(region, [np.array([(x * sx, y * sy) for x, y in outline], np.int32)], 1)
    mx, mn = rgb.max(-1), rgb.min(-1)
    white = ((mn > 0.86) & (mx - mn < 0.12) & (alpha > 0.5) & (region > 0)).astype(np.uint8)
    count, labels, stats, _ = cv2.connectedComponentsWithStats(white, 8)
    keep = np.zeros_like(white)
    for i in range(1, count):
        x, y, bw, bh, area = stats[i]
        if area < min_area * sx * sy:
            continue
        if exclude is not None:
            ex0, ey0, ex1, ey1 = exclude[0] * sx, exclude[1] * sy, exclude[2] * sx, exclude[3] * sy
            ix = max(0, min(x + bw, ex1) - max(x, ex0))
            iy = max(0, min(y + bh, ey1) - max(y, ey0))
            inter = ix * iy
            union = bw * bh + (ex1 - ex0) * (ey1 - ey0) - inter
            if inter / union > 0.5:
                continue
        keep[labels == i] = 1
    radius = max(3, round(7 * sx))
    mask = cv2.morphologyEx(keep, cv2.MORPH_CLOSE, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (2 * radius + 1, 2 * radius + 1)))
    mask = cv2.dilate(mask, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (7, 7))) & cv2.dilate(region, np.ones((15, 15), np.uint8))
    soft = cv2.GaussianBlur(mask.astype(np.float32), (3, 3), 0.8)
    out = np.dstack([rgb, alpha * soft])
    return Image.fromarray((np.clip(out, 0, 1) * 255 + 0.5).astype(np.uint8), "RGBA")


def card_hand(im):
    return cut_hand(im, CARD_HAND, (1106, 1422), exclude=CARD_RECT)


def poly_mask(points, size=(960, 540)):
    """An anti-aliased polygon (points in the 1920 x 1080 frame) as the alpha of a white texture."""
    big = Image.new("L", (size[0] * 4, size[1] * 4), 0)
    k = size[0] * 4 / 1920.0
    ImageDraw.Draw(big).polygon([(x * k, y * k) for x, y in points], fill=255)
    a = big.resize(size, Image.LANCZOS)
    white = Image.new("RGBA", size, (255, 255, 255, 0))
    white.putalpha(a)
    return white


def write(out):
    names = ["T_P3R_Tarot_0", "T_P3R_Tarot_1", "T_P3R_Tarot_2", "T_P3R_Tarot_3", "T_P3R_Tarot_4", "T_P3R_Tarot_5", "T_P3R_TarotBack"]
    for name, card in zip(names, tarot_cards()):
        card.save(os.path.join(out, name + ".png"))
    card_hand(Image.open(os.path.join(out, "T_P3R_HeroCard.png"))).save(os.path.join(out, "T_P3R_HeroCardFront.png"))
    poly_mask(BOARD).save(os.path.join(out, "T_P3R_BoardMask.png"))
    poly_mask(SIDE).save(os.path.join(out, "T_P3R_SideMask.png"))
    print("T_P3R_Tarot_0..5, T_P3R_TarotBack, T_P3R_HeroCardFront, T_P3R_BoardMask, T_P3R_SideMask")


def preview():
    cards = tarot_cards()
    strip = Image.new("RGB", (CARD_SIZE[0] * 7, CARD_SIZE[1]))
    for i, c in enumerate(cards):
        strip.paste(c, (i * CARD_SIZE[0], 0))
    strip.save(os.path.join(HERE, "..", "..", "Saved", "fr", "tarot.png"))
    src = Image.open(os.path.join(HERE, "Final", "T_P3R_HeroCard.png")).convert("RGBA")
    hand = card_hand(src)
    ground = Image.new("RGBA", src.size, (200, 40, 60, 255))
    ground.alpha_composite(hand)
    ground.convert("RGB").crop((150, 150, 750, 800)).save(os.path.join(HERE, "..", "..", "Saved", "fr", "card_hand.png"))
    print(len(cards), "cards")


if __name__ == "__main__":
    preview()

"""The stills of the second early design (P3R_ProtoMosaic): T_P3R_Mosaic_0..7, one picture per tile, cut from the night
street and the protagonist's pictures (imported by prepare_art.py; run alone for a contact sheet in Saved/fr).

The design's stills are frames of the game's own scenes, shot close with a shallow depth of field: lamps melted into
bokeh, a facade gone soft, the paving, a grey blur of something near the lens, a warm close-up -- and the protagonist,
once as an ink drawing in profile against a warm blur, and twice washed out in cyan light. So each tile is a crop,
blurred (strongly for the near things), graded towards the design's muted warm greys or its cyan.
"""
import os
import numpy as np
import cv2
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
STREET = os.path.join(HERE, "Gen", "V3", "gpt-image-1794358724759-1.png")

# name index -> (width, height) of the tile, as laid out in P3R_ProtoMosaic.dui
SIZES = {0: (217, 216), 1: (517, 284), 2: (225, 300), 3: (563, 262), 4: (177, 317), 5: (232, 135), 6: (288, 353), 7: (458, 183)}


def load(path, mode="RGB"):
    return np.asarray(Image.open(path).convert(mode)).astype(np.float32) / 255.0


def crop_cover(img, box, size):
    """Crop box (x0, y0, x1, y1, in fractions) and fill size (w, h), keeping the aspect (centre crop)."""
    h, w = img.shape[:2]
    x0, y0, x1, y1 = int(box[0] * w), int(box[1] * h), int(box[2] * w), int(box[3] * h)
    part = img[y0:y1, x0:x1]
    tw, th = size
    scale = max(tw / part.shape[1], th / part.shape[0])
    part = cv2.resize(part, (max(tw, round(part.shape[1] * scale)), max(th, round(part.shape[0] * scale))), interpolation=cv2.INTER_AREA)
    oy, ox = (part.shape[0] - th) // 2, (part.shape[1] - tw) // 2
    return part[oy:oy + th, ox:ox + tw]


def blur(img, sigma):
    return cv2.GaussianBlur(img, (0, 0), sigma) if sigma > 0 else img


def grade(img, sat=0.5, gain=1.0, lift=0.0, warm=0.0):
    grey = img @ np.array([0.299, 0.587, 0.114], np.float32)
    out = grey[..., None] + (img - grey[..., None]) * sat
    out = out * gain + lift
    out = out + np.array([warm, warm * 0.4, -warm * 0.6], np.float32)
    return np.clip(out, 0, 1)


def cyan_wash(rgba):
    """A figure washed out in cyan light: white where it was light, cyan elsewhere, over a pale cyan ground."""
    rgb, a = rgba[..., :3], rgba[..., 3:4]
    lum = rgb @ np.array([0.299, 0.587, 0.114], np.float32)
    ground = np.array([0.42, 0.86, 0.98], np.float32)
    figure = np.clip(lum[..., None] * 1.5, 0, 1) * np.array([1.0, 1.0, 1.0]) + (1 - np.clip(lum[..., None] * 1.5, 0, 1)) * np.array([0.2, 0.7, 0.95])
    return np.clip(ground * (1 - a) + figure * a, 0, 1)


def tiles():
    street = load(STREET)
    ink = load(os.path.join(HERE, "Final", "T_P3R_HeroInk.png"))
    card = load(os.path.join(HERE, "Final", "T_P3R_HeroCard.png"), "RGBA")
    floating = load(os.path.join(HERE, "Final", "T_P3R_HeroFloat.png"), "RGBA")
    out = {}
    # 0: the street lamps, melted into bokeh
    out[0] = grade(blur(crop_cover(street, (0.15, 0.03, 0.3, 0.35), SIZES[0]), 9), sat=0.8, gain=1.1)
    # 1: the protagonist in ink against a warm blur, fading into it on the right
    w, h = SIZES[1]
    warm = grade(blur(crop_cover(street, (0.58, 0.5, 0.86, 0.8), SIZES[1]), 14), sat=0.7, gain=0.9, warm=0.05)
    face = crop_cover(ink, (0.25, 0.3, 0.85, 0.8), SIZES[1])
    fade = np.clip((np.linspace(1.0, 0.0, w)[None, :] - 0.25) / 0.3, 0, 1)[..., None] * np.ones((h, 1, 1), np.float32)
    lum = face @ np.array([0.299, 0.587, 0.114], np.float32)
    drawn = np.where(lum[..., None] < 0.5, face * 0.15, np.ones_like(face) * 0.96)
    out[1] = np.clip(drawn * fade + warm * (1 - fade), 0, 1)
    # 2: a facade, soft
    out[2] = grade(blur(crop_cover(street, (0.66, 0.0, 0.84, 0.42), SIZES[2]), 8), sat=0.3, gain=1.0, lift=0.06)
    # 3: a warm close-up -- the shop's curtain, near the lens
    out[3] = grade(blur(crop_cover(street, (0.58, 0.5, 0.74, 0.62), SIZES[3]), 12), sat=0.3, gain=1.15, lift=0.1, warm=0.05)
    # 4 and 7: the protagonist washed out in cyan light
    out[4] = blur(cyan_wash(crop_cover(card, (0.3, 0.2, 0.56, 0.62), SIZES[4])), 1.5)
    out[7] = blur(cyan_wash(crop_cover(floating, (0.18, 0.48, 0.92, 0.78), SIZES[7])), 2.5)
    # 5: the paving
    out[5] = grade(blur(crop_cover(street, (0.13, 0.72, 0.33, 0.98), SIZES[5]), 4), sat=0.35, gain=1.1, lift=0.05)
    # 6: something grey right in front of the lens (the pole)
    out[6] = grade(blur(crop_cover(street, (0.0, 0.3, 0.14, 0.8), SIZES[6]), 11), sat=0.1, gain=1.35, lift=0.12)
    return out


def write(folder):
    for index, img in tiles().items():
        Image.fromarray((img * 255 + 0.5).astype(np.uint8), "RGB").save(os.path.join(folder, f"T_P3R_Mosaic_{index}.png"))
    print("T_P3R_Mosaic_0..7")


def sheet():
    out = tiles()
    canvas = np.zeros((1080, 1920, 3), np.float32)
    at = {0: (424, 150), 1: (806, 300), 2: (1195, 150), 3: (443, 581), 4: (823, 611), 5: (116, 795), 6: (504, 904), 7: (889, 868)}
    tri = np.zeros((1080, 1920), np.float32)
    yy, xx = np.mgrid[0:1080, 0:1920]
    edge = 795 + (1080 - yy) / 1080 * (1695 - 795)
    canvas[xx > edge] = np.array([0.18, 0.42, 0.92])
    for i, img in out.items():
        cx, cy = at[i]
        hh, ww = img.shape[:2]
        x0, y0 = cx - ww // 2, cy - hh // 2
        xs, ys = max(0, x0), max(0, y0)
        canvas[ys:y0 + hh, xs:x0 + ww] = img[ys - y0:, xs - x0:][: 1080 - ys, : 1920 - xs]
    Image.fromarray((canvas * 255).astype(np.uint8)).resize((1280, 720)).save(os.path.join(HERE, "..", "..", "Saved", "fr", "mosaic.png"))
    print("sheet")


if __name__ == "__main__":
    sheet()

"""The protagonist's pictures redrawn in the pause menus' palette (imported by prepare_art.py; run alone for a preview).

    python toon.py < /dev/null        -> Saved/fr/toon.png, before and after on the sea's blue

The game's menu model is a toon render washed with the sea's light: skin and shirt pure white with pale lavender shade,
hair in two or three flat cyans and blues, the dark blazer the same blue as the water (and half see-through), thin
dark-blue lines, and red only as a black-and-red cord. The generated pictures are anime paintings -- many hair strands,
gradients, a large bright red ribbon -- so each pixel is sorted into one of those families and given its flat colour:

    line    very dark, and thin (dark only along a narrow band)           -> navy, opaque
    red     the ribbon                                                   -> black, its brightest folds red
    pale    low saturation, light (skin, shirt, card)                    -> white / lavender, two levels
    blue    everything else (hair, blazer, rim light)                     -> three flat blues by brightness,
                                                                            the darkest see-through
The picture is smoothed first (edge-preserving), so strands merge into the flat masses of a cel render.
"""
import os
import numpy as np
import cv2
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))


def srgb(*c):
    return np.array(c, np.float32)


NAVY = srgb(0.05, 0.08, 0.36)
BLACK = srgb(0.04, 0.03, 0.08)
RED = srgb(0.86, 0.09, 0.2)
WHITE = srgb(1.0, 1.0, 1.0)
LAVENDER = srgb(0.77, 0.79, 0.93)
SHADE = srgb(0.6, 0.64, 0.88)
CYAN_LIGHT = srgb(0.56, 0.9, 1.0)
CYAN = srgb(0.24, 0.7, 0.98)
BLUE = srgb(0.15, 0.42, 0.93)
DEEP = srgb(0.09, 0.24, 0.84)


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def toon(im, deep=DEEP, deep_alpha=0.8):
    """deep: the darkest blue (blazer, deepest hair), deep_alpha: how solid it is (the material lets more of the sea
    through it, see M_P3R_Figure)."""
    rgba = np.asarray(im.convert("RGBA")).astype(np.float32) / 255.0
    rgb, alpha = rgba[..., :3], rgba[..., 3]
    # Edge-preserving smoothing merges the strands; lines are taken from the unsmoothed picture.
    u8 = (rgb * 255).astype(np.uint8)
    flat = cv2.bilateralFilter(u8, 9, 40, 9)
    flat = cv2.bilateralFilter(flat, 9, 40, 9).astype(np.float32) / 255.0
    r, g, b = flat[..., 0], flat[..., 1], flat[..., 2]
    mx = flat.max(-1)
    mn = flat.min(-1)
    sat = (mx - mn) / np.maximum(mx, 1e-4)
    lum = 0.2126 * r + 0.7152 * g + 0.0722 * b

    # Lines: dark here, much lighter a few pixels around (a line, not a mass).
    raw_lum = 0.2126 * rgb[..., 0] + 0.7152 * rgb[..., 1] + 0.0722 * rgb[..., 2]
    around = cv2.dilate(raw_lum, np.ones((7, 7), np.uint8))
    line = smoothstep(0.32, 0.18, raw_lum) * smoothstep(0.12, 0.3, around - raw_lum)

    redness = smoothstep(0.15, 0.35, r - np.maximum(g, b))
    pale = smoothstep(0.32, 0.18, sat) * smoothstep(0.45, 0.6, lum)

    # Each family posterized to a few levels, then the level map median-filtered so specks merge into their mass.
    def levels(value, cuts, size):
        index = np.zeros(value.shape, np.uint8)
        for cut in cuts:
            index += (value > cut).astype(np.uint8)
        return cv2.medianBlur(index, size)

    # Pale: shade, lavender, white.
    pale_index = levels(lum, (0.6, 0.8), 5)
    pale_col = np.stack([SHADE, LAVENDER, WHITE])[pale_index]
    # Blue: by brightness (the blue channel is high on every blue, so the luminance tells them apart).
    blue_index = levels(lum, (0.2, 0.5, 0.74), 7)
    blue_col = np.stack([deep, BLUE, CYAN, CYAN_LIGHT])[blue_index]
    blue_alpha = np.where(blue_index > 0, 1.0, deep_alpha)
    # The eyes: a vivid cyan, kept as drawn.
    eye = smoothstep(0.55, 0.75, sat) * smoothstep(0.55, 0.7, g) * smoothstep(0.5, 0.3, r)
    eye = cv2.erode(eye, np.ones((3, 3), np.uint8))
    blue_col = blue_col * (1 - eye[..., None]) + rgb * eye[..., None]
    # The ribbon: black, red only along its brightest quarter (the game's cord is black with a red thread).
    ribbon = redness > 0.5
    red_cut = np.percentile(r[ribbon], 75) if ribbon.any() else 1.0
    red_col = np.where((r > red_cut)[..., None], RED, BLACK)

    col = blue_col
    a = blue_alpha
    col = col * (1 - pale[..., None]) + pale_col * pale[..., None]
    a = a * (1 - pale) + pale
    col = col * (1 - redness[..., None]) + red_col * redness[..., None]
    a = a * (1 - redness) + redness
    # Ink where the pale (skin, shirt) meets the dark blues or the ribbon, and round the silhouette, two pixels wide --
    # not where it meets the light cyans of the hair, which a cel render leaves unlined.
    kernel = np.ones((3, 3), np.uint8)
    is_pale = (pale > 0.5).astype(np.uint8)
    is_dark = (((pale <= 0.5) & (blue_index <= 1)) | (redness > 0.5)).astype(np.uint8)
    edge = ((is_pale > 0) & (cv2.dilate(is_dark, kernel) > 0)) | ((is_dark > 0) & (cv2.dilate(is_pale, kernel) > 0))
    solid = (alpha > 0.5).astype(np.uint8)
    edge |= (cv2.dilate(solid, kernel) != cv2.erode(solid, kernel)) & (alpha > 0.2)
    ink = cv2.GaussianBlur(edge.astype(np.float32), (3, 3), 0.7)
    line = np.maximum(line, np.clip(ink * 1.4, 0, 1))
    col = col * (1 - line[..., None]) + NAVY * line[..., None]
    a = a * (1 - line) + line
    out = np.dstack([col, alpha * a])
    return Image.fromarray((np.clip(out, 0, 1) * 255 + 0.5).astype(np.uint8), "RGBA")


def ribbon_only(im):
    """For pictures already drawn flat in the menus' palette: only the red ribbon changes, to black edged in red (the
    game's black-and-red cord). A flat ribbon has no brightness to split, so the red is a rim a few pixels wide."""
    rgba = np.asarray(im.convert("RGBA")).astype(np.float32) / 255.0
    rgb = rgba[..., :3]
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    redness = smoothstep(0.15, 0.35, r - np.maximum(g, b))
    ribbon = (redness > 0.5).astype(np.uint8)
    if ribbon.any():
        rim = max(2, round(min(rgb.shape[:2]) / 260))
        core = cv2.erode(ribbon, np.ones((2 * rim + 1, 2 * rim + 1), np.uint8))
        core = cv2.GaussianBlur(core.astype(np.float32), (3, 3), 0.8)[..., None]
        red_col = RED * (1 - core) + BLACK * core
        rgb = rgb * (1 - redness[..., None]) + red_col * redness[..., None]
    out = np.dstack([rgb, rgba[..., 3]])
    return Image.fromarray((np.clip(out, 0, 1) * 255 + 0.5).astype(np.uint8), "RGBA")


def preview():
    names = ["T_P3R_HeroFloat", "T_P3R_HeroHand", "T_P3R_HeroCard", "T_P3R_HeroFall"]
    rows = []
    for n in names:
        src = Image.open(os.path.join(HERE, "Final", n + ".png")).convert("RGBA")
        h = 520
        w = int(src.width * h / src.height)
        src = src.resize((w, h), Image.LANCZOS)
        pair = Image.new("RGBA", (w * 2, h), (24, 80, 220, 255))
        before = Image.new("RGBA", (w, h), (24, 80, 220, 255))
        before.alpha_composite(src)
        after = Image.new("RGBA", (w, h), (24, 80, 220, 255))
        after.alpha_composite(toon(src))
        pair.paste(before, (0, 0))
        pair.paste(after, (w, 0))
        rows.append(pair)
    width = sum(p.width for p in rows)
    sheet = Image.new("RGB", (width, 520))
    x = 0
    for p in rows:
        sheet.paste(p.convert("RGB"), (x, 0))
        x += p.width
    sheet.save(os.path.join(HERE, "..", "..", "Saved", "fr", "toon.png"))
    print(sheet.size)


if __name__ == "__main__":
    preview()

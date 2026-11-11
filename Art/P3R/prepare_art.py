"""Turns the generated pictures in Art/P3R/Gen into the textures the UI uses, in Art/P3R/Final.

    python prepare_art.py < /dev/null

Pictures delivered on a flat green ground are keyed to alpha; the rest are copied (and, for backgrounds, scaled to a
16:9 1920x1080 frame). Re-runnable: the outputs are rebuilt every time.
"""
import os
from PIL import Image, ImageFilter
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
GEN = os.path.join(HERE, "Gen")
OUT = os.path.join(HERE, "Final")

# source file (under Gen) -> (output name, treatment)
PICTURES = {
    "gpt-image-1794325665461-1.png": ("T_P3R_SeaClassroom", "frame"),
    # The protagonist, second pass, redrawn in the menus' flat palette (toon.py; the material adds the water, see M_P3R_Figure).
    "V2/gpt-image-1794333968533-1.png": ("T_P3R_HeroFall", "hero-solid"),
    # Third pass: drawn after the game's own framing of each menu (the screenshots as pose references, our character),
    # already flat -- only the ribbon is recoloured.
    "V3/gpt-image-1794362432602-1.png": ("T_P3R_HeroFloat", "ribbon"),
    # STATUS, second try: fingertips over the hairline and the palm along the brow, a gap between them for the shard.
    # ...and edited once more so his own eye is hidden under a lock of hair: only the party member's eyes, in the shard.
    "V3/gpt-image-1794365665060-1.png": ("T_P3R_HeroHand", "ribbon"),
    # COMMU, second try: a slim three-quarter view, as the game's.
    "V3/gpt-image-1794363686236-1.png": ("T_P3R_HeroCard", "ribbon"),
    "gpt-image-1794325905497-1.png": ("T_P3R_Portrait_6", "copy"),
    "gpt-image-1794325941031-1.png": ("T_P3R_CityBokeh", "frame"),
    "gpt-image-1794325971821-1.png": ("T_P3R_HeroInk", "copy"),
    "Party/gpt-image-1794326054879-1.png": ("T_P3R_Face_0", "copy"),
    "Party/gpt-image-1794326109527-1.png": ("T_P3R_Face_1", "copy"),
    "Party/gpt-image-1794326165338-1.png": ("T_P3R_Face_2", "copy"),
    "Party/gpt-image-1794326220774-1.png": ("T_P3R_Face_3", "copy"),
    "Party/gpt-image-1794326275163-1.png": ("T_P3R_Face_4", "copy"),
    "Party/gpt-image-1794326328419-1.png": ("T_P3R_Face_5", "copy"),
    "Party/gpt-image-1794326385731-1.png": ("T_P3R_Face_6", "copy"),
    "More/gpt-image-1794326897711-1.png": ("T_P3R_FieldClassroom", "frame"),
    "More/gpt-image-1794326964943-1.png": ("T_P3R_Portrait_0", "copy"),
    "More/gpt-image-1794327034659-1.png": ("T_P3R_Portrait_1", "copy"),
    "More/gpt-image-1794327101338-1.png": ("T_P3R_Portrait_2", "copy"),
    "More/gpt-image-1794327174488-1.png": ("T_P3R_Portrait_3", "copy"),
    "More/gpt-image-1794327251933-1.png": ("T_P3R_Portrait_4", "copy"),
    "More/gpt-image-1794327324677-1.png": ("T_P3R_Portrait_5", "copy"),
    "More/gpt-image-1794327363287-1.png": ("T_P3R_HeroCinema", "copy"),
    # Third pass: the street of the first early design.
    "V3/gpt-image-1794358724759-1.png": ("T_P3R_NightStreet", "frame"),
}


def bubble(size=128):
    """A soap-bubble sprite: a thin bright rim, a faint body and a small highlight, white on transparent."""
    yy, xx = np.mgrid[0:size, 0:size].astype(np.float32)
    c = (size - 1) / 2.0
    r = np.hypot(xx - c, yy - c) / c
    rim = np.clip(1.0 - np.abs(r - 0.86) / 0.1, 0.0, 1.0) ** 1.5
    body = np.clip(1.0 - r, 0.0, 1.0) * 0.12
    hx, hy = c - size * 0.2, c - size * 0.2
    spot = np.clip(1.0 - np.hypot(xx - hx, yy - hy) / (size * 0.11), 0.0, 1.0) ** 2
    alpha = np.clip(rim * 0.9 + body + spot, 0.0, 1.0) * (r <= 1.0)
    rgba = np.dstack([np.ones_like(alpha), np.ones_like(alpha), np.ones_like(alpha), alpha])
    return Image.fromarray((rgba * 255.0 + 0.5).astype(np.uint8), "RGBA")


def shard(size=128):
    """A confetti shard: a sharp white triangle (tinted per particle)."""
    im = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    from PIL import ImageDraw
    ImageDraw.Draw(im).polygon([(size * 0.08, size * 0.85), (size * 0.92, size * 0.62), (size * 0.38, size * 0.1)], fill=(255, 255, 255, 255))
    return im


def key_green(im):
    """Flat #00FF00 ground to alpha, with the green spill taken back out of the soft edge."""
    a = np.asarray(im.convert("RGB")).astype(np.float32) / 255.0
    r, g, b = a[..., 0], a[..., 1], a[..., 2]
    greenness = g - np.maximum(r, b)              # 0 on white/cyan/blue/black/red, ~1 on the ground
    alpha = 1.0 - np.clip((greenness - 0.18) / (0.55 - 0.18), 0.0, 1.0)
    g = np.where(alpha < 0.999, np.minimum(g, np.maximum(r, b)), g)   # despill the edge
    rgba = np.dstack([r, g, b, alpha])
    out = Image.fromarray((rgba * 255.0 + 0.5).astype(np.uint8), "RGBA")
    # one pixel of erosion on the alpha: the generator's antialiasing leaves a green-grey fringe
    a_ch = out.getchannel("A").filter(ImageFilter.MinFilter(3))
    out.putalpha(a_ch)
    return out


def clean_alpha(im, min_area=900):
    """A transparent-ground picture with the generator's specks taken off: islands of alpha smaller than min_area pixels
    (stray flecks around the hair) are cleared, and alpha under 6% is zeroed."""
    from scipy import ndimage
    rgba = np.asarray(im.convert("RGBA")).astype(np.float32) / 255.0
    alpha = rgba[..., 3]
    alpha = np.where(alpha < 0.06, 0.0, alpha)
    labels, count = ndimage.label(alpha > 0.0)
    if count:
        sizes = ndimage.sum(np.ones_like(alpha), labels, index=np.arange(1, count + 1))
        keep = np.concatenate([[False], sizes >= min_area])
        alpha = np.where(keep[labels], alpha, 0.0)
    rgba[..., 3] = alpha
    return Image.fromarray((rgba * 255.0 + 0.5).astype(np.uint8), "RGBA")


# The party's eyes drawn flat and looking up to the upper left, the angle of the protagonist's face on STATUS.
EYES_SHEET = "V3/gpt-image-1794364518257-1.png"


def eye_strips():
    """The party's eyes, cut from a 2 x 4 sheet of 768 x 256 panels (the eighth is empty): T_P3R_Eyes_0..6, in party
    order. A few pixels come off every side, past the sheet's black gutters."""
    sheet = Image.open(os.path.join(GEN, EYES_SHEET)).convert("RGB")
    w, h = sheet.width // 2, sheet.height // 4
    inset = 8
    for index in range(7):
        col, row = index % 2, index // 2
        box = (col * w + inset, row * h + inset, (col + 1) * w - inset, (row + 1) * h - inset)
        sheet.crop(box).resize((768, 256), Image.LANCZOS).save(os.path.join(OUT, f"T_P3R_Eyes_{index}.png"))
    print("T_P3R_Eyes_0..6")


def frame_16x9(im, width=1920, height=1080):
    """Cover a 1920x1080 frame (scale to fill, centre crop)."""
    im = im.convert("RGB")
    scale = max(width / im.width, height / im.height)
    im = im.resize((round(im.width * scale), round(im.height * scale)), Image.LANCZOS)
    left = (im.width - width) // 2
    top = (im.height - height) // 2
    return im.crop((left, top, left + width, top + height))


def main():
    os.makedirs(OUT, exist_ok=True)
    for src, (name, how) in PICTURES.items():
        path = os.path.join(GEN, src)
        if not os.path.exists(path):
            print("missing", src)
            continue
        im = Image.open(path)
        if how == "key":
            im = key_green(im)
        elif how == "clean":
            im = clean_alpha(im)
        elif how == "hero":
            from toon import toon
            im = toon(clean_alpha(im))
        elif how == "hero-solid":
            # Falling across SYSTEM's white paper: the blazer a solid indigo, the colour of the disc behind him.
            from toon import toon, srgb
            im = toon(clean_alpha(im), deep=srgb(0.07, 0.09, 0.6), deep_alpha=1.0)
            # Mirrored: in the game his raised hand is up to the left of his head.
            im = im.transpose(Image.FLIP_LEFT_RIGHT)
        elif how == "ribbon":
            from toon import ribbon_only
            im = ribbon_only(clean_alpha(im))
        elif how == "frame":
            im = frame_16x9(im)
        else:
            im = im.convert("RGBA") if im.mode != "RGB" else im
        im.save(os.path.join(OUT, name + ".png"))
        print(name, im.size, im.mode)
        if name == "T_P3R_HeroHand":
            # The hand alone, the layer in front of the STATUS shard.
            from hand_layer import hand_layer
            hand_layer(im).save(os.path.join(OUT, "T_P3R_HeroHandFront.png"))
            print("T_P3R_HeroHandFront")
    eye_strips()
    import mosaic
    mosaic.write(OUT)
    import extras
    extras.write(OUT)
    bubble().save(os.path.join(OUT, "T_P3R_Bubble.png"))
    shard().save(os.path.join(OUT, "T_P3R_Shard.png"))
    print("T_P3R_Bubble, T_P3R_Shard")


if __name__ == "__main__":
    main()

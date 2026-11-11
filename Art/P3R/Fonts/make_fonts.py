"""Makes the Latin display cuts of the P3R study (P3R-*.ttf) from Roboto, the engine's own font (Apache License 2.0,
see LICENSE-Roboto.txt): the study's type is heavy and narrow, so most cuts are Roboto drawn narrower.

    python make_fonts.py < /dev/null

Roboto is read from the engine's Engine/Content/Slate/Fonts (UE_ENGINE_ROOT, else the Epic launcher's UE 5.8).
Narrowing scales every outline, advance and horizontal kerning/anchor value; the TrueType hinting is dropped (it would
no longer fit the outlines, and the distance-field atlas reads the outlines unhinted anyway).
"""
import json
import os

from fontTools.pens.transformPen import TransformPen
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont
from fontTools.ttLib.tables import ttProgram

HERE = os.path.dirname(os.path.abspath(__file__))

CUTS = {
    # file: (Roboto face, horizontal scale, family name); the scales match the widths of the cuts the study was laid
    # out with (Bahnschrift at wdth 75 / 87.5 / 75 / 100), so no text grows out of its box
    # heavy condensed: menu words, big background letters, numerals
    "P3R-Heavy.ttf": ("Roboto-Black.ttf", 0.671, "P3R Study Heavy"),
    # bold semi-condensed: names, labels
    "P3R-Bold.ttf": ("Roboto-Bold.ttf", 0.820, "P3R Study Bold"),
    # light condensed: PLV / HP / SP figures, guide text
    "P3R-Light.ttf": ("Roboto-Light.ttf", 0.686, "P3R Study Light"),
    # light regular width: the thin type of the early prototype
    "P3R-Thin.ttf": ("Roboto-Light.ttf", 0.987, "P3R Study Thin"),
}


def engine_fonts():
    root = os.environ.get("UE_ENGINE_ROOT")
    if not root:
        launcher = os.path.join(os.environ.get("ProgramData", r"C:\ProgramData"), "Epic", "UnrealEngineLauncher", "LauncherInstalled.dat")
        if os.path.exists(launcher):
            for item in json.load(open(launcher, encoding="utf-8"))["InstallationList"]:
                if item["AppName"] == "UE_5.8":
                    root = item["InstallLocation"]
    if not root:
        raise SystemExit("Unreal Engine 5.8 not found: set UE_ENGINE_ROOT")
    return os.path.join(root, "Engine", "Content", "Slate", "Fonts")


def scale_x_values(node, s, seen):
    """Scales every horizontal positioning value in a GPOS subtree (kerning pairs, mark anchors...)."""
    if id(node) in seen or isinstance(node, (str, int, float, bytes)) or node is None:
        return
    seen.add(id(node))
    if isinstance(node, (list, tuple)):
        for item in node:
            scale_x_values(item, s, seen)
        return
    attrs = getattr(node, "__dict__", None)
    if not attrs:
        return
    for name, value in list(attrs.items()):
        if name in ("XAdvance", "XPlacement", "XCoordinate") and isinstance(value, (int, float)):
            setattr(node, name, int(round(value * s)))
        else:
            scale_x_values(value, s, seen)


def narrow(font, s):
    glyf, hmtx, order = font["glyf"], font["hmtx"], font.getGlyphOrder()
    glyph_set = font.getGlyphSet()
    redrawn = {}
    for name in order:
        pen = TTGlyphPen(glyph_set)
        glyph_set[name].draw(TransformPen(pen, (s, 0, 0, 1, 0, 0)))
        redrawn[name] = pen.glyph()
    for name, glyph in redrawn.items():
        glyf[name] = glyph
    # Bounds after every glyph is replaced: a composite's bounds come from the glyphs it places.
    for name, glyph in redrawn.items():
        glyph.recalcBounds(glyf)
        advance, _ = hmtx[name]
        hmtx[name] = (int(round(advance * s)), getattr(glyph, "xMin", 0))
    if "GPOS" in font:
        scale_x_values(font["GPOS"].table, s, set())
    if "kern" in font:
        for sub in font["kern"].kernTables:
            sub.kernTable = {pair: int(round(v * s)) for pair, v in sub.kernTable.items()}
    font["OS/2"].xAvgCharWidth = int(round(font["OS/2"].xAvgCharWidth * s))


def drop_hinting(font):
    for tag in ("fpgm", "prep", "cvt ", "hdmx", "LTSH", "VDMX"):
        if tag in font:
            del font[tag]
    for name in font.getGlyphOrder():
        glyph = font["glyf"][name]
        if hasattr(glyph, "program"):
            empty = ttProgram.Program()
            empty.fromBytecode(b"")
            glyph.program = empty


def rename(font, family):
    # Modified Roboto must not carry Roboto's names (the license asks modified files to say they were changed).
    for record in font["name"].names:
        if record.nameID in (1, 4, 16):
            record.string = family
        elif record.nameID == 3:
            record.string = family + " (modified Roboto)"
        elif record.nameID == 6:
            record.string = family.replace(" ", "")


source = engine_fonts()
for out, (face, scale, family) in CUTS.items():
    font = TTFont(os.path.join(source, face))
    drop_hinting(font)
    if scale != 1.0:
        narrow(font, scale)
    rename(font, family)
    font.save(os.path.join(HERE, out))
    print(out, face, scale)

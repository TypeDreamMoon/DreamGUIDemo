# Builds the NieR UI test's assets in DevProject, run in the editor (see the end of the file):
#
#   UnrealEditor.exe DevProject.uproject -ExecutePythonScript=<this file> -nierquit
#
#   /Game/UITests/NieR/Textures/NieR_T_*   from _Tools/Textures/*.png (make_nier_textures.py draws them)
#   /Game/UITests/NieR/Fonts/              M PLUS 1p faces (_Tools/Fonts, OFL) and the DreamGUI fonts made from them
#   /Game/UITests/NieR/UI/Components/NieR_* the components, each a class of NieRComponents.h (or the base, for a pure-.dui one) fed by Components/NieR_*.dui
#   /Game/UITests/NieR/UI/NieR_*           the screens, each a NieR C++ screen class fed by DUI/UITests/NieR/NieR_*.dui
#   /Game/UITests/NieR/Maps/NieR_TestMap   an empty level whose game mode shows the start screen
#
# Re-running it re-imports the textures, keeps the fonts and blueprints, points them at their sources again and
# recompiles, so a changed .dui is picked up.

import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = "/Game/UITests/NieR"
TEXTURES = ROOT + "/Textures"
FONTS = ROOT + "/Fonts"
UI = ROOT + "/UI"
MAPS = ROOT + "/Maps"
# Compiled in this order: a widget's .dui loads the classes of the components it places, so they come first.
COMPONENTS = UI + "/Components"
WIDGETS = (
    (COMPONENTS, "NieR_Row", "/Script/DevUITests.NieRRow", "UITests/NieR/Components/NieR_Row.dui"),
    (COMPONENTS, "NieR_Tab", "/Script/DevUITests.NieRTab", "UITests/NieR/Components/NieR_Tab.dui"),
    (COMPONENTS, "NieR_Header", "/Script/DevUITests.NieRHeader", "UITests/NieR/Components/NieR_Header.dui"),
    (COMPONENTS, "NieR_HintBar", "/Script/DevUITests.NieRHintBar", "UITests/NieR/Components/NieR_HintBar.dui"),
    (COMPONENTS, "NieR_Prompt", "/Script/DevUITests.NieRPrompt", "UITests/NieR/Components/NieR_Prompt.dui"),
    (COMPONENTS, "NieR_ListPage", "/Script/DevUITests.NieRListPage", "UITests/NieR/Components/NieR_ListPage.dui"),
    (COMPONENTS, "NieR_Stat", "/Script/DevUITests.NieRComponent", "UITests/NieR/Components/NieR_Stat.dui"),
    (COMPONENTS, "NieR_StatusPanel", "/Script/DevUITests.NieRComponent", "UITests/NieR/Components/NieR_StatusPanel.dui"),
    (UI, "NieR_Title", "/Script/DevUITests.NieRTitleScreen", "UITests/NieR/NieR_Title.dui"),
    (UI, "NieR_Menu", "/Script/DevUITests.NieRMenuScreen", "UITests/NieR/NieR_Menu.dui"),
    (UI, "NieR_Settings", "/Script/DevUITests.NieRSettingsScreen", "UITests/NieR/NieR_Settings.dui"),
)
FACES = (("NieR_FontFace_Regular", "MPLUS1p-Regular.ttf"), ("NieR_FontFace_Medium", "MPLUS1p-Medium.ttf"),
         ("NieR_FontFace_Bold", "MPLUS1p-Bold.ttf"))
GAME_MODE = "/Script/DevUITests.NieRUITestGameMode"
failures = []


def log(message):
    unreal.log("[make_nier_assets] " + message)
    print("[make_nier_assets] " + message)


def import_files(files, destination, name_for):
    tasks = []
    for path in files:
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", path)
        task.set_editor_property("destination_path", destination)
        task.set_editor_property("destination_name", name_for(path))
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", False)
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    imported = []
    for task in tasks:
        paths = list(task.get_editor_property("imported_object_paths"))
        if not paths:
            failures.append("import failed: " + task.get_editor_property("filename"))
            continue
        imported.extend(paths)
    return imported


def import_textures():
    folder = os.path.join(HERE, "Textures")
    files = sorted(os.path.join(folder, f) for f in os.listdir(folder) if f.lower().endswith(".png"))
    paths = import_files(files, TEXTURES, lambda p: os.path.splitext(os.path.basename(p))[0])
    textures = []
    for path in paths:
        texture = unreal.load_asset(path)
        if not isinstance(texture, unreal.Texture2D):
            continue
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        texture.set_editor_property("srgb", True)
        textures.append(texture)
    log("textures: %d" % len(textures))
    return textures


def import_faces():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    faces = {}
    missing = []
    for name, file_name in FACES:
        existing = unreal.load_asset(FONTS + "/" + name)
        if isinstance(existing, unreal.FontFace):
            faces[name] = existing
        else:
            missing.append((name, os.path.join(HERE, "Fonts", file_name)))
    if missing:
        by_file = dict((path, name) for name, path in missing)
        for path in import_files([path for _, path in missing], FONTS, lambda p: by_file[p]):
            asset = unreal.load_asset(path)
            if isinstance(asset, unreal.FontFace):
                faces[asset.get_name()] = asset
    for name, _ in FACES:
        if name not in faces:
            failures.append("no font face " + name)
    log("font faces: %s" % ", ".join(sorted(faces)))
    return faces


def make_font(name, face, bold=None):
    path = FONTS + "/" + name
    font = unreal.load_asset(path)
    if font is None:
        font = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(name, FONTS, unreal.load_asset("/DreamGUI/DefaultFont_DistanceField"))
    if font is None:
        failures.append("cannot make " + path)
        return None
    font.set_editor_property("font_type", unreal.DreamUIDynamicFontDataType.ENGINE_FONT)
    font.set_editor_property("engine_font", face)
    font.set_editor_property("sdf_source", unreal.DreamUISdfSource.OUTLINE_MULTI_CHANNEL)
    entry = unreal.DreamUIFontFallback()
    entry.set_editor_property("font", unreal.load_asset("/DreamGUI/DefaultFont_DistanceField_CJK"))
    font.set_editor_property("fallbacks", [entry])
    font.set_editor_property("bold_font", bold)
    font.set_editor_property("italic_font", None)
    font.set_editor_property("bold_italic_font", None)
    log("font " + path + " <- " + face.get_path_name())
    return font


def make_widgets():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    if not hasattr(unreal, "BlueprintEditorLibrary"):
        unreal.load_module("BlueprintEditorLibrary")
    library = unreal.BlueprintEditorLibrary
    made = []
    for folder, name, parent_path, source in WIDGETS:
        parent = unreal.load_class(None, parent_path)
        if parent is None:
            failures.append("no class " + parent_path + " (build DevProject)")
            continue
        blueprint = unreal.load_asset(folder + "/" + name)
        if blueprint is None:
            blueprint = asset_tools.create_asset(name, folder, unreal.DreamWidgetBlueprint, unreal.DreamWidgetBlueprintFactory())
        if blueprint is None:
            failures.append("cannot create " + folder + "/" + name)
            continue
        if library.generated_class(blueprint) is None or not unreal.MathLibrary.class_is_child_of(library.generated_class(blueprint), parent):
            library.reparent_blueprint(blueprint, parent)
        defaults = unreal.get_default_object(library.generated_class(blueprint))
        defaults.set_editor_property("SourceFile", unreal.FilePath(file_path=source))
        if not library.compile_blueprint(blueprint):
            failures.append("compile failed: " + name)
        log("widget " + blueprint.get_path_name() + " <- " + source)
        made.append(blueprint)
    return made


def make_map():
    # Through the level editor, never as a loose World asset: a World object Python still holds when a map of the same
    # package loads is a "world memory leak", which the editor treats as fatal.
    path = MAPS + "/NieR_TestMap"
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    exists = unreal.EditorAssetLibrary.does_asset_exist(path) if hasattr(unreal, "EditorAssetLibrary") else False
    if not exists:
        exists = unreal.AssetRegistryHelpers.get_asset_registry().get_asset_by_object_path(path + ".NieR_TestMap").is_valid()
    if not (levels.load_level(path) if exists else levels.new_level(path)):
        failures.append("cannot open " + path)
        return
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    game_mode = unreal.load_class(None, GAME_MODE)
    settings = world.get_world_settings() if world is not None else None
    if settings is None or game_mode is None:
        failures.append("no world settings or no game mode for " + path)
        return
    settings.set_editor_property("default_game_mode", game_mode)
    if not levels.save_current_level():
        failures.append("cannot save " + path)
        return
    log("map " + path + " game mode " + GAME_MODE)


def main():
    unreal.AssetRegistryHelpers.get_asset_registry().wait_for_completion()
    textures = import_textures()
    faces = import_faces()
    fonts = []
    if len(faces) == len(FACES):
        bold = make_font("NieR_Font_Bold", faces["NieR_FontFace_Bold"])
        medium = make_font("NieR_Font_Medium", faces["NieR_FontFace_Medium"], bold)
        regular = make_font("NieR_Font", faces["NieR_FontFace_Regular"], bold)
        fonts = [f for f in (bold, medium, regular) if f is not None]
    assets = list(textures) + list(faces.values()) + fonts
    if assets and not unreal.EditorLoadingAndSavingUtils.save_packages([a.get_outermost() for a in assets], False):
        failures.append("saving textures/fonts failed")
    widgets = make_widgets()
    if widgets and not unreal.EditorLoadingAndSavingUtils.save_packages([b.get_outermost() for b in widgets], False):
        failures.append("saving widgets failed")
    make_map()
    if failures:
        for failure in failures:
            unreal.log_error("[make_nier_assets] " + failure)
            print("[make_nier_assets] FAILED: " + failure)
    else:
        log("done")


# Importing needs Slate, which a -run=pythonscript commandlet does not have: run it in the editor,
#   UnrealEditor.exe DevProject.uproject -ExecutePythonScript=<this file> -nierquit
# and -nierquit closes the editor when it is done.
main()
if "-nierquit" in unreal.SystemLibrary.get_command_line().lower():
    unreal.SystemLibrary.quit_editor()

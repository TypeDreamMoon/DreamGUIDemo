# Builds the Detroit UI test's assets in DevProject, run in the editor (see the end of the file):
#
#   UnrealEditor.exe DevProject.uproject -ExecutePythonScript=<this file> -detroitquit
#
#   /Game/UITests/Detroit/Fonts/              Roboto faces (the engine's own TTFs, Apache 2.0) and the DreamGUI fonts made from them
#   /Game/UITests/Detroit/UI/Components/*     the components, each a class of DetroitWidgets.h fed by Components/Detroit_*.dui
#   /Game/UITests/Detroit/UI/Detroit_*        the screens, fed by DUI/UITests/Detroit/Detroit_*.dui
#   /Game/UITests/Detroit/Maps/Detroit_TestMap  an empty level with a player start; its game mode builds the crime scene
#
# The materials are not made here: they are DreamShader sources (DShader/UITests/Detroit/Materials/*.dss), built with
# Plugins/Dream/DreamShader/.skill/dsc.ps1 check <file> -Shaders.
#
# Re-running it keeps the fonts and blueprints, points them at their sources again and recompiles, so a changed .dui is
# picked up.

import os

import unreal

ROOT = "/Game/UITests/Detroit"
FONTS = ROOT + "/Fonts"
UI = ROOT + "/UI"
MAPS = ROOT + "/Maps"
COMPONENTS = UI + "/Components"
ENGINE_FONTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.engine_content_dir()), "Slate", "Fonts")
# Compiled in this order: a widget's .dui loads the classes of the components it places, so they come first.
WIDGETS = (
    (COMPONENTS, "Detroit_ObjectiveRow", "/Script/DevUITests.DetroitObjectiveRow", "UITests/Detroit/Components/Detroit_ObjectiveRow.dui"),
    (COMPONENTS, "Detroit_ClueTag", "/Script/DevUITests.DetroitClueTag", "UITests/Detroit/Components/Detroit_ClueTag.dui"),
    (COMPONENTS, "Detroit_HitMarks", "/Script/DevUITests.DetroitAnalysisPanel", "UITests/Detroit/Components/Detroit_HitMarks.dui"),
    (COMPONENTS, "Detroit_Objectives", "/Script/DevUITests.DetroitObjectives", "UITests/Detroit/Components/Detroit_Objectives.dui"),
    (COMPONENTS, "Detroit_Jacket", "/Script/DevUITests.DetroitWidget", "UITests/Detroit/Components/Detroit_Jacket.dui"),
    (COMPONENTS, "Detroit_WallWriting", "/Script/DevUITests.DetroitWidget", "UITests/Detroit/Components/Detroit_WallWriting.dui"),
    (UI, "Detroit_Loading", "/Script/DevUITests.DetroitLoadingScreen", "UITests/Detroit/Detroit_Loading.dui"),
    (UI, "Detroit_HUD", "/Script/DevUITests.DetroitHUD", "UITests/Detroit/Detroit_HUD.dui"),
)
FACES = (("Detroit_FontFace_Light", "Roboto-Light.ttf"), ("Detroit_FontFace_Regular", "Roboto-Regular.ttf"),
         ("Detroit_FontFace_Medium", "Roboto-Medium.ttf"), ("Detroit_FontFace_Bold", "Roboto-Bold.ttf"))
GAME_MODE = "/Script/DevUITests.DetroitGameMode"
PLAYER_START = unreal.Vector(-250.0, -70.0, 100.0)
failures = []


def log(message):
    unreal.log("[make_detroit_assets] " + message)
    print("[make_detroit_assets] " + message)


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


def import_faces():
    faces = {}
    missing = []
    for name, file_name in FACES:
        existing = unreal.load_asset(FONTS + "/" + name)
        if isinstance(existing, unreal.FontFace):
            faces[name] = existing
        else:
            missing.append((name, os.path.join(ENGINE_FONTS, file_name)))
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
    path = MAPS + "/Detroit_TestMap"
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    exists = unreal.AssetRegistryHelpers.get_asset_registry().get_asset_by_object_path(path + ".Detroit_TestMap").is_valid()
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
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    starts = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.PlayerStart)]
    if not starts:
        actors.spawn_actor_from_class(unreal.PlayerStart, PLAYER_START, unreal.Rotator(0.0, 0.0, 0.0))
    else:
        starts[0].set_actor_location(PLAYER_START, False, False)
    if not levels.save_current_level():
        failures.append("cannot save " + path)
        return
    log("map " + path + " game mode " + GAME_MODE)


def main():
    unreal.AssetRegistryHelpers.get_asset_registry().wait_for_completion()
    faces = import_faces()
    fonts = []
    if len(faces) == len(FACES):
        bold = make_font("Detroit_Font_Bold", faces["Detroit_FontFace_Bold"])
        medium = make_font("Detroit_Font_Medium", faces["Detroit_FontFace_Medium"], bold)
        regular = make_font("Detroit_Font", faces["Detroit_FontFace_Regular"], bold)
        light = make_font("Detroit_Font_Light", faces["Detroit_FontFace_Light"], bold)
        fonts = [f for f in (bold, medium, regular, light) if f is not None]
    assets = list(faces.values()) + fonts
    if assets and not unreal.EditorLoadingAndSavingUtils.save_packages([a.get_outermost() for a in assets], False):
        failures.append("saving fonts failed")
    widgets = make_widgets()
    if widgets and not unreal.EditorLoadingAndSavingUtils.save_packages([b.get_outermost() for b in widgets], False):
        failures.append("saving widgets failed")
    make_map()
    if failures:
        for failure in failures:
            unreal.log_error("[make_detroit_assets] " + failure)
            print("[make_detroit_assets] FAILED: " + failure)
    else:
        log("done")


# Importing needs Slate, which a -run=pythonscript commandlet does not have: run it in the editor,
#   UnrealEditor.exe DevProject.uproject -ExecutePythonScript=<this file> -detroitquit
# and -detroitquit closes the editor when it is done.
main()
if "-detroitquit" in unreal.SystemLibrary.get_command_line().lower():
    unreal.SystemLibrary.quit_editor()

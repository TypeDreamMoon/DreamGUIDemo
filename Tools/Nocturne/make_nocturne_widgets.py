# Creates (when missing) and compiles ONE NOCTURNE//OS widget Blueprint from its .dui source, then saves it.
# Run inside the demo editor through UnrealBridge, one widget per exec:
#   exec "NAME='WBP_Nocturne_TestPanel'; exec(open(r'<this file>').read())"
# One per exec, components before the screens that place them.
import unreal

WIDGETS = {
    # name: (folder, parent class, source under DUI/)
    "WBP_Nocturne_TestPanel": ("/Game/Nocturne/UI", "/Script/DreamGUI.DreamTextUserWidget", "Nocturne/Nocturne_TestPanel.dui"),
    "WBP_Nocturne_Cursor": ("/Game/Nocturne/UI", "/Script/DreamGUI.DreamTextUserWidget", "Nocturne/Nocturne_Cursor.dui"),
    "WBP_Nocturne_ScreenFX": ("/Game/Nocturne/UI", "/Script/DreamGUI.DreamTextUserWidget", "Nocturne/Nocturne_ScreenFX.dui"),
    "WBP_Nocturne_Lens": ("/Game/Nocturne/UI", "/Script/DreamGUIDemo.NocturneLens", "Nocturne/Nocturne_Lens.dui"),
    "WBP_Nocturne_OptionRow": ("/Game/Nocturne/UI/Components", "/Script/DreamGUIDemo.NocturneOptionRow", "Nocturne/Components/Nocturne_OptionRow.dui"),
    "WBP_Nocturne_Settings": ("/Game/Nocturne/UI", "/Script/DreamGUIDemo.NocturneSettingsScreen", "Nocturne/Nocturne_Settings.dui"),
    "WBP_Nocturne_StarLabel": ("/Game/Nocturne/UI/Components", "/Script/DreamGUIDemo.NocturneStarLabel", "Nocturne/Components/Nocturne_StarLabel.dui"),
    "WBP_Nocturne_MainMenu": ("/Game/Nocturne/UI", "/Script/DreamGUIDemo.NocturneMainMenu", "Nocturne/Nocturne_MainMenu.dui"),
    "WBP_Nocturne_ItemGhost": ("/Game/Nocturne/UI/Components", "/Script/DreamGUIDemo.NocturneItemGhost", "Nocturne/Components/Nocturne_ItemGhost.dui"),
    "WBP_Nocturne_ItemTip": ("/Game/Nocturne/UI/Components", "/Script/DreamGUIDemo.NocturneItemTip", "Nocturne/Components/Nocturne_ItemTip.dui"),
    "WBP_Nocturne_ItemSlot": ("/Game/Nocturne/UI/Components", "/Script/DreamGUIDemo.NocturneItemSlot", "Nocturne/Components/Nocturne_ItemSlot.dui"),
    "WBP_Nocturne_Title": ("/Game/Nocturne/UI", "/Script/DreamGUIDemo.NocturneTitleScreen", "Nocturne/Nocturne_Title.dui"),
    "WBP_Nocturne_Archive": ("/Game/Nocturne/UI", "/Script/DreamGUIDemo.NocturneArchiveScreen", "Nocturne/Nocturne_Archive.dui"),
    "WBP_Nocturne_Wheel": ("/Game/Nocturne/UI", "/Script/DreamGUIDemo.NocturneWheelScreen", "Nocturne/Nocturne_Wheel.dui"),
    "WBP_Nocturne_MusicBox": ("/Game/Nocturne/UI", "/Script/DreamGUIDemo.NocturneMusicScreen", "Nocturne/Nocturne_MusicBox.dui"),
    "WBP_Nocturne_Log": ("/Game/Nocturne/UI", "/Script/DreamGUIDemo.NocturneLogScreen", "Nocturne/Nocturne_Log.dui"),
    "WBP_Nocturne_Depart": ("/Game/Nocturne/UI", "/Script/DreamGUIDemo.NocturneDepartScreen", "Nocturne/Nocturne_Depart.dui"),
    "WBP_Nocturne_Hud": ("/Game/Nocturne/UI", "/Script/DreamGUIDemo.NocturneHud", "Nocturne/Nocturne_Hud.dui"),
    "WBP_Nocturne_LayerFx": ("/Game/Nocturne/UI", "/Script/DreamGUIDemo.NocturneLayerFxScreen", "Nocturne/Nocturne_LayerFx.dui"),
    "WBP_Nocturne_Text": ("/Game/Nocturne/UI", "/Script/DreamGUIDemo.NocturneTextScreen", "Nocturne/Nocturne_Text.dui"),
    "WBP_Nocturne_Perf": ("/Game/Nocturne/UI", "/Script/DreamGUIDemo.NocturnePerfScreen", "Nocturne/Nocturne_Perf.dui"),
    "WBP_Nocturne_CardOpen": ("/Game/Nocturne/UI", "/Script/DreamGUIDemo.NocturneCardScreen", "Nocturne/Nocturne_CardOpen.dui"),
    "WBP_Nocturne_CardEnd": ("/Game/Nocturne/UI", "/Script/DreamGUIDemo.NocturneCardScreen", "Nocturne/Nocturne_CardEnd.dui"),
    "WBP_Nocturne_Marker": ("/Game/Nocturne/UI", "/Script/DreamGUI.DreamTextUserWidget", "Nocturne/Nocturne_Marker.dui"),
    "WBP_Demo_Panel": ("/Game/Demo", "/Script/DreamGUI.DreamTextUserWidget", "Demo/Demo_Panel.dui"),
    "WBP_Demo_Legacy": ("/Game/Demo", "/Script/DreamGUI.DreamTextUserWidget", "Demo/Demo_LegacySource.dui"),
    "WBP_Demo_Hello": ("/Game/Demo", "/Script/DreamGUI.DreamTextUserWidget", "Demo/Demo_Hello.dui"),
}

if NAME == "IMPORT_ADMIN":  # noqa: F821 -- the title's administrator picture, once
    tex_path = "/Game/Nocturne/Textures/T_Nocturne_Admin"
    if unreal.load_asset(tex_path) is None:
        task = unreal.AssetImportTask()
        task.filename = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()) + "Plugins/DreamGUI/Images/character.png"
        task.destination_path = "/Game/Nocturne/Textures"
        task.destination_name = "T_Nocturne_Admin"
        task.automated = True
        task.save = False
        task.replace_existing = True
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    tex = unreal.load_asset(tex_path)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("srgb", True)
    saved = unreal.EditorLoadingAndSavingUtils.save_packages([tex.get_outermost()], False)
    print("T_Nocturne_Admin", "imported", "saved" if saved else "NOT SAVED")
elif NAME == "EMOJI_FONT":  # noqa: F821 -- the text font with a colour emoji fallback, for the text shot's emoji line
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    folder = "/Game/Nocturne/Fonts"
    face_path = folder + "/FF_NotoColorEmoji"
    if unreal.load_asset(face_path) is None:
        task = unreal.AssetImportTask()
        task.filename = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()) + "Art/Fonts/NotoColorEmoji.ttf"
        task.destination_path = folder
        task.destination_name = "FF_NotoColorEmoji"
        task.automated = True
        task.save = False
        task.replace_existing = True
        tools.import_asset_tasks([task])
    face = unreal.load_asset(face_path)
    print("face", face)
    # The emoji face as a DreamUI font: a copy of the plugin's Latin font pointed at the emoji face, no fallbacks.
    emoji_path = folder + "/F_NotoColorEmoji"
    emoji = unreal.load_asset(emoji_path)
    if emoji is None:
        emoji = unreal.EditorAssetLibrary.duplicate_asset("/DreamGUI/DefaultFont_DistanceField", emoji_path)
    emoji.set_editor_property("fallbacks", [])
    emoji.set_editor_property("font_type", unreal.DreamUIDynamicFontDataType.ENGINE_FONT)
    emoji.set_editor_property("engine_font", face)
    # The text font: the plugin's CJK font with the emoji face added after its own fallbacks, for emoji only.
    text_path = folder + "/F_Nocturne_Emoji"
    text = unreal.load_asset(text_path)
    if text is None:
        text = unreal.EditorAssetLibrary.duplicate_asset("/DreamGUI/DefaultFont_DistanceField_CJK", text_path)
    fallbacks = [f for f in text.get_editor_property("fallbacks") if f.get_editor_property("font") != emoji]
    entry = unreal.DreamUIFontFallback()
    entry.set_editor_property("font", emoji)
    ranges = []
    for lo, hi in [(0x23, 0x23), (0x2A, 0x2A), (0x30, 0x39), (0xA9, 0xA9), (0xAE, 0xAE), (0x203C, 0x3299), (0x1F000, 0x1FAFF)]:
        r = unreal.Int32Interval()
        r.set_editor_property("min", lo)
        r.set_editor_property("max", hi)
        ranges.append(r)
    entry.set_editor_property("ranges", ranges)
    entry.set_editor_property("scale", 1.0)
    fallbacks.append(entry)
    text.set_editor_property("fallbacks", fallbacks)
    saved = unreal.EditorLoadingAndSavingUtils.save_packages([face.get_outermost(), emoji.get_outermost(), text.get_outermost()], False)
    print("emoji font", [str(f.get_editor_property("font").get_name()) for f in text.get_editor_property("fallbacks")], "saved" if saved else "NOT SAVED")
elif NAME == "DEMO_MAP":  # noqa: F821 -- the map to open and press Play on: an empty level run by NocturneGameMode
    path = "/Game/Nocturne/Maps/Nocturne_Demo"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorLoadingAndSavingUtils.load_map(path)
    else:
        unreal.EditorLevelLibrary.new_level(path)
    world = unreal.EditorLevelLibrary.get_editor_world()
    settings = world.get_world_settings()
    mode = unreal.load_class(None, "/Script/DreamGUIDemo.NocturneGameMode")
    settings.set_editor_property("default_game_mode", mode)
    saved = unreal.EditorLevelLibrary.save_current_level()
    print("Nocturne_Demo", "game mode", settings.get_editor_property("default_game_mode"), "saved" if saved else "NOT SAVED")
elif NAME == "IMPORT_ART":  # noqa: F821 -- the generated pictures: covers, save scenes, item studies
    import os
    art = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()) + "Art/Nocturne"
    kinds = [("Covers", "cover_", "T_Cover_"), ("Saves", "save_", "T_Save_"), ("Items", "item_", "T_Item_")]
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    made = []
    for folder, prefix, asset_prefix in kinds:
        src = art + "/" + folder
        if not os.path.isdir(src):
            continue
        for f in sorted(os.listdir(src)):
            if not (f.startswith(prefix) and f.endswith(".png")):
                continue
            name = asset_prefix + f[len(prefix):-4]
            dest = "/Game/Nocturne/Textures/" + folder
            if unreal.load_asset(dest + "/" + name) is None:
                task = unreal.AssetImportTask()
                task.filename = src + "/" + f
                task.destination_path = dest
                task.destination_name = name
                task.automated = True
                task.save = False
                task.replace_existing = True
                tools.import_asset_tasks([task])
            tex = unreal.load_asset(dest + "/" + name)
            if tex is None:
                print("FAILED", name)
                continue
            tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
            tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
            tex.set_editor_property("max_texture_size", 512)
            tex.set_editor_property("srgb", True)
            made.append(tex.get_outermost())
    saved = unreal.EditorLoadingAndSavingUtils.save_packages(made, False)
    print("art imported", len(made), "saved" if saved else "NOT SAVED")
else:
    name = NAME  # noqa: F821 -- set by the caller
    folder, parent_path, source = WIDGETS[name]
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    if not hasattr(unreal, "BlueprintEditorLibrary"):
        unreal.load_module("BlueprintEditorLibrary")
    lib = unreal.BlueprintEditorLibrary
    parent = unreal.load_class(None, parent_path)
    if parent is None:
        raise RuntimeError("no class " + parent_path + " (build the demo project)")

    bp = unreal.load_asset(folder + "/" + name)
    created = bp is None
    if created:
        bp = asset_tools.create_asset(name, folder, unreal.DreamWidgetBlueprint, unreal.DreamWidgetBlueprintFactory())
    gen = lib.generated_class(bp)
    if gen is None or not unreal.MathLibrary.class_is_child_of(gen, parent):
        lib.reparent_blueprint(bp, parent)
    cdo = unreal.get_default_object(lib.generated_class(bp))
    cdo.set_editor_property("SourceFile", unreal.FilePath(file_path=source))
    ok = lib.compile_blueprint(bp)
    saved = unreal.EditorLoadingAndSavingUtils.save_packages([bp.get_outermost()], False)
    print(name, "created" if created else "existing", "compiled" if ok else "COMPILE FAILED", "saved" if saved else "NOT SAVED")

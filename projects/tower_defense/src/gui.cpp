/*
    File: gui.cpp
    Author: Philip Haynes
*/

#include "gui.hpp"
#include "globals.hpp"

namespace Int {

const vec3 colorBack = {1.0, 0.4, 0.1};
const vec3 colorHighlightLow = {0.2, 0.45, 0.5};
const vec3 colorHighlightMedium = {0.4, 0.9, 1.0};
const vec3 colorHighlightHigh = {0.9, 0.98, 1.0};

Gui::~Gui() {
    for (Widget* widget : allWidgets) {
        delete widget;
    }
}

void Gui::EventAssetInit() {
    globals->assets.filesToLoad.Append("DroidSans.ttf");
    // globals->assets.filesToLoad.Append("LiberationSerif-Regular.ttf");
    // globals->assets.filesToLoad.Append("OpenSans-Regular.ttf");
    // globals->assets.filesToLoad.Append("Literata[wght].ttf");
    globals->assets.filesToLoad.Append("gamma.tga");
    globals->assets.filesToLoad.Append("click in 1.ogg");
    globals->assets.filesToLoad.Append("click in 2.ogg");
    globals->assets.filesToLoad.Append("click in 3.ogg");
    globals->assets.filesToLoad.Append("click in 4.ogg");
    globals->assets.filesToLoad.Append("click out 1.ogg");
    globals->assets.filesToLoad.Append("click out 2.ogg");
    globals->assets.filesToLoad.Append("click out 3.ogg");
    globals->assets.filesToLoad.Append("click out 4.ogg");
    globals->assets.filesToLoad.Append("click soft 1.ogg");
    globals->assets.filesToLoad.Append("click soft 2.ogg");
    globals->assets.filesToLoad.Append("Pop High.ogg");
    globals->assets.filesToLoad.Append("Pop Low.ogg");

    globals->assets.filesToLoad.Append("Cursor.png");
}

void Gui::EventAssetAcquire() {
    fontIndex = globals->assets.FindMapping("DroidSans.ttf");
    // fontIndex = globals->assets.FindMapping("LiberationSerif-Regular.ttf");
    // fontIndex = globals->assets.FindMapping("OpenSans-Regular.ttf");
    // fontIndex = globals->assets.FindMapping("Literata[wght].ttf");
    sndClickInSources[0].Create("click in 1.ogg");
    sndClickInSources[1].Create("click in 2.ogg");
    sndClickInSources[2].Create("click in 3.ogg");
    sndClickInSources[3].Create("click in 4.ogg");
    sndClickIn.sources = {
        &sndClickInSources[0],
        &sndClickInSources[1],
        &sndClickInSources[2],
        &sndClickInSources[3]
    };
    sndClickOutSources[0].Create("click out 1.ogg");
    sndClickOutSources[1].Create("click out 2.ogg");
    sndClickOutSources[2].Create("click out 3.ogg");
    sndClickOutSources[3].Create("click out 4.ogg");
    sndClickOut.sources = {
        &sndClickOutSources[0],
        &sndClickOutSources[1],
        &sndClickOutSources[2],
        &sndClickOutSources[3]
    };
    for (i32 i = 0; i < 4; i++) {
        sndClickInSources[i].SetGain(0.15);
        sndClickInSources[i].SetPitch(1.2);
        sndClickOutSources[i].SetGain(0.15);
        sndClickOutSources[i].SetPitch(1.2);
    }
    sndClickSoftSources[0].Create("click soft 1.ogg");
    sndClickSoftSources[1].Create("click soft 2.ogg");
    sndClickSoftSources[0].SetGain(0.01);
    sndClickSoftSources[1].SetGain(0.01);
    sndClickSoftSources[0].SetPitch(1.2);
    sndClickSoftSources[1].SetPitch(1.2);
    sndClickSoft.sources = {
        &sndClickSoftSources[0],
        &sndClickSoftSources[1]
    };
    sndPopHigh.Create("Pop High.ogg");
    sndPopLow.Create("Pop Low.ogg");
    sndPopHigh.SetGain(0.1);
    sndPopLow.SetGain(0.1);
    font = &globals->assets.fonts[fontIndex];

    cursorIndex = globals->assets.FindMapping("Cursor.png");
}

void Gui::EventInitialize() {
    mainMenu.Initialize();
    settingsMenu.Initialize();
    playMenu.Initialize();
}

void Gui::EventSync() {
    mouseoverWidget = nullptr;
    mouseoverDepth = -1;
    currentMenu = nextMenu;
    switch (currentMenu) {
    case MENU_MAIN:
        mainMenu.Update();
        break;
    case MENU_SETTINGS:
        settingsMenu.Update();
        break;
    case MENU_PLAY:
        playMenu.Update();
        break;
    }
    readyForDraw = true;
    if (globals->input.cursor != globals->input.cursorPrevious) {
        usingMouse = true;
    }
    if (globals->rawInput.AnyGP.Pressed()) {
        usingMouse = false;
    }
}

void Gui::EventDraw(Array<Rendering::DrawingContext> &contexts) {
    switch (currentMenu) {
    case MENU_MAIN:
        mainMenu.Draw(contexts.Back());
        break;
    case MENU_SETTINGS:
        settingsMenu.Draw(contexts.Back());
        break;
    case MENU_PLAY:
        playMenu.Draw(contexts.Back());
        break;
    }
    if (usingMouse) {
        // globals->rendering.DrawQuad(DrawingContext &context, i32 texIndex, vec4 color, vec2 position, vec2 scalePre, vec2 scalePost, optional vec2 origin = vec2(0.0), optional Radians32 rotation = 0.0)
        globals->rendering.DrawQuad(contexts.Back(), cursorIndex, vec4(1.0), globals->input.cursor, vec2(32.0 * scale), vec2(1.0), vec2(0.5));
    }
}

void AddWidget(Widget *parent, Widget *newWidget, bool deeper = false) {
    newWidget->depth = parent->depth + (deeper ? 1 : 0);
    if (newWidget->selectable) {
        parent->selectable = true;
    }
    parent->children.Append(newWidget);
    if (globals->gui.allWidgets.count(newWidget) == 0) {
        globals->gui.allWidgets.emplace(newWidget);
    }
}

void AddWidget(Widget *parent, Switch *newWidget) {
    newWidget->depth = parent->depth + 1;
    newWidget->parentDepth = parent->depth;
    if (newWidget->selectable) {
        parent->selectable = true;
    }
    parent->children.Append(newWidget);
    if (globals->gui.allWidgets.count(newWidget) == 0) {
        globals->gui.allWidgets.emplace(newWidget);
    }
}

void AddWidgetAsDefault(List *parent, Widget *newWidget, bool deeper = false) {
    newWidget->depth = parent->depth + (deeper ? 1 : 0);
    if (newWidget->selectable) {
        parent->selectable = true;
    }
    parent->selectionDefault = parent->children.size;
    parent->children.Append(newWidget);
    if (globals->gui.allWidgets.count(newWidget) == 0) {
        globals->gui.allWidgets.emplace(newWidget);
    }
}

void AddWidgetAsDefault(List *parent, Switch *newWidget) {
    newWidget->depth = parent->depth + 1;
    newWidget->parentDepth = parent->depth;
    if (newWidget->selectable) {
        parent->selectable = true;
    }
    parent->selectionDefault = parent->children.size;
    parent->children.Append(newWidget);
    if (globals->gui.allWidgets.count(newWidget) == 0) {
        globals->gui.allWidgets.emplace(newWidget);
    }
}

//
//      Menu implementations
//

void MainMenu::Initialize() {
    ListV *listV = new ListV();
    listV->color = vec4(0.0);
    listV->highlight = vec4(vec3(0.0), 0.1);

    Widget *spacer = new Widget();
    spacer->size.y = 0.3;
    AddWidget(listV, spacer);

    Text *title = new Text();
    title->alignH = Rendering::CENTER;
    title->bold = true;
    title->color = vec4(0.0, 0.0, 0.0, 1.0);
    title->colorOutline = vec4(1.0);
    title->outline = true;
    title->fontSize = 64.0;
    title->fontIndex = globals->gui.fontIndex;
    title->string = globals->ReadLocale("AzCore Tower Defense");
    AddWidget(listV, title);

    spacer = new Widget();
    spacer->size.y = 0.4;
    AddWidget(listV, spacer);

    ListV *buttonList = new ListV();
    buttonList->fractionWidth = false;
    buttonList->size = vec2(500.0, 0.0);
    buttonList->padding = vec2(16.0);

    buttonStart = new Button();
    buttonStart->string = globals->ReadLocale("Start");
    buttonStart->size.y = 64.0;
    buttonStart->fractionHeight = false;
    buttonStart->margin = vec2(16.0);
    AddWidget(buttonList, buttonStart);

    buttonSettings = new Button();
    buttonSettings->string = globals->ReadLocale("Settings");
    buttonSettings->size.y = 64.0;
    buttonSettings->fractionHeight = false;
    buttonSettings->margin = vec2(16.0);
    AddWidget(buttonList, buttonSettings);

    buttonExit = new Button();
    buttonExit->string = globals->ReadLocale("Exit");
    buttonExit->size.y = 64.0;
    buttonExit->fractionHeight = false;
    buttonExit->margin = vec2(16.0);
    buttonExit->highlightBG = vec4(colorBack, 0.9);
    buttonExit->keycodeActivators = {KC_KEY_ESC};
    AddWidget(buttonList, buttonExit);

    ListH *spacingList = new ListH();
    spacingList->color = vec4(0.0);
    spacingList->highlight = vec4(0.0);
    spacingList->size.y = 0.0;

    spacer = new Widget();
    spacer->size.x = 0.5;
    AddWidget(spacingList, spacer);

    AddWidgetAsDefault(spacingList, buttonList);

    AddWidgetAsDefault(listV, spacingList);

    AddWidget(&screen, listV);
}

void MainMenu::Update() {
    screen.Update(vec2(0.0), true);
    if (buttonStart->state.Released()) {
        globals->gui.nextMenu = MENU_PLAY;
        buttonStart->string = globals->ReadLocale("Continue");
    }
    if (buttonSettings->state.Released()) {
        globals->gui.nextMenu = MENU_SETTINGS;
    }
    if (buttonExit->state.Released()) {
        globals->exit = true;
    }
}

void MainMenu::Draw(Rendering::DrawingContext &context) {
    screen.Draw(context);
}

void SettingsMenu::Initialize() {
    ListV *listV = new ListV();
    listV->color = vec4(0.0);
    listV->highlight = vec4(0.0);

    Widget *spacer = new Widget();
    spacer->size.y = 0.3;
    AddWidget(listV, spacer);

    Text *title = new Text();
    title->alignH = Rendering::CENTER;
    title->bold = true;
    title->color = vec4(0.0, 0.0, 0.0, 1.0);
    title->colorOutline = vec4(1.0);
    title->outline = true;
    title->fontSize = 64.0;
    title->fontIndex = globals->gui.fontIndex;
    title->string = globals->ReadLocale("Settings");
    AddWidget(listV, title);

    spacer = new Widget();
    spacer->size.y = 0.4;
    AddWidget(listV, spacer);

    ListV *actualList = new ListV();
    actualList->fractionWidth = false;
    actualList->size.x = 500.0;
    actualList->size.y = 0.0;
    actualList->padding = vec2(24.0);

    Text *settingTextTemplate = new Text();
    settingTextTemplate->fontIndex = globals->gui.fontIndex;
    settingTextTemplate->fontSize = 20.0;
    settingTextTemplate->fractionHeight = true;
    settingTextTemplate->size.y = 1.0;
    settingTextTemplate->alignV = Rendering::CENTER;

    checkFullscreen = new Checkbox();
    checkFullscreen->checked = globals->fullscreen;

    TextBox *textboxTemplate = new TextBox();
    textboxTemplate->fontIndex = globals->gui.fontIndex;
    textboxTemplate->size.x = 64.0;
    textboxTemplate->alignH = Rendering::RIGHT;
    textboxTemplate->textFilter = TextFilterDigits;
    textboxTemplate->textValidate = TextValidateNonempty;

    Slider *sliderTemplate = new Slider();
    sliderTemplate->fractionHeight = true;
    sliderTemplate->fractionWidth = false;
    sliderTemplate->size = vec2(116.0, 1.0);
    sliderTemplate->valueMax = 100.0;

    textboxFramerate = new TextBox(*textboxTemplate);
    textboxFramerate->string = ToWString(ToString((i32)globals->framerate));


    for (i32 i = 0; i < 3; i++) {
        textboxVolumes[i] = new TextBox(*textboxTemplate);
        sliderVolumes[i] = new Slider(*sliderTemplate);
        textboxVolumes[i]->textFilter = TextFilterDecimalsPositive;
        textboxVolumes[i]->textValidate = TextValidateDecimalsPositive;
        sliderVolumes[i]->mirror = textboxVolumes[i];
    }
    textboxVolumes[0]->string = ToWString(ToString(globals->volumeMain*100.0, 10, 1));
    textboxVolumes[1]->string = ToWString(ToString(globals->volumeMusic*100.0, 10, 1));
    textboxVolumes[2]->string = ToWString(ToString(globals->volumeEffects*100.0, 10, 1));
    sliderVolumes[0]->value = globals->volumeMain*100.0;
    sliderVolumes[1]->value = globals->volumeMusic*100.0;
    sliderVolumes[2]->value = globals->volumeEffects*100.0;

    ListH *settingListTemplate = new ListH();
    settingListTemplate->size.y = 0.0;
    settingListTemplate->margin = vec2(8.0);
    settingListTemplate->padding = vec2(0.0);

    Array<Widget*> settingListItems = {
        checkFullscreen, nullptr,
        textboxFramerate, nullptr,
        nullptr, nullptr,
        sliderVolumes[0], textboxVolumes[0],
        sliderVolumes[1], textboxVolumes[1],
        sliderVolumes[2], textboxVolumes[2]
    };
    const char *settingListNames[] = {
        "Fullscreen",
        "Framerate",
        "Volume",
        "Main",
        "Music",
        "Effects"
    };

    for (i32 i = 0; i < settingListItems.size; i+=2) {
        if (settingListItems[i] == nullptr) {
            Text *settingText = new Text(*settingTextTemplate);
            settingText->string = globals->ReadLocale(settingListNames[i / 2]);
            settingText->alignH = Rendering::CENTER;
            settingText->fontSize = 24.0;
            AddWidget(actualList, settingText);
        } else {
            ListH *settingList = new ListH(*settingListTemplate);
            Text *settingText = new Text(*settingTextTemplate);
            settingText->string = globals->ReadLocale(settingListNames[i / 2]);
            AddWidget(settingList, settingText);
            AddWidgetAsDefault(settingList, settingListItems[i]);
            if (settingListItems[i+1] != nullptr) {
                AddWidget(settingList, settingListItems[i+1]);
            }

            AddWidget(actualList, settingList);
        }
    }

    ListH *buttonList = new ListH();
    buttonList->size.y = 0.0;
    buttonList->margin = vec2(0.0);
    buttonList->padding = vec2(0.0);
    buttonList->color = vec4(0.0);
    buttonList->highlight = vec4(0.0);

    buttonBack = new Button();
    buttonBack->string = globals->ReadLocale("Back");
    buttonBack->size.x = 1.0 / 2.0;
    buttonBack->size.y = 64.0;
    buttonBack->fractionHeight = false;
    buttonBack->margin = vec2(8.0);
    buttonBack->highlightBG = vec4(colorBack, 0.9);
    buttonBack->keycodeActivators = {KC_GP_BTN_B, KC_KEY_ESC};
    AddWidget(buttonList, buttonBack);

    buttonApply = new Button();
    buttonApply->string = globals->ReadLocale("Apply");
    buttonApply->size.x = 1.0 / 2.0;
    buttonApply->size.y = 64.0;
    buttonApply->fractionHeight = false;
    buttonApply->margin = vec2(8.0);
    AddWidgetAsDefault(buttonList, buttonApply);

    AddWidget(actualList, buttonList);

    ListH *spacingList = new ListH();
    spacingList->color = vec4(0.0);
    spacingList->highlight = vec4(0.0);
    spacingList->size.y = 0.0;

    spacer = new Widget();
    spacer->size.x = 0.5;
    AddWidget(spacingList, spacer);

    AddWidgetAsDefault(spacingList, actualList);

    AddWidgetAsDefault(listV, spacingList);

    AddWidget(&screen, listV);

    delete settingListTemplate;
    delete settingTextTemplate;
    delete sliderTemplate;
    delete textboxTemplate;
}

u64 WStringToU64(WString str) {
    u64 number = 0;
    u64 exponent = 1;
    for (i32 i = str.size-1; i >= 0; i--) {
        number += u64(str[i]-'0') * exponent;
        exponent *= 10;
    }
    return number;
}

void SettingsMenu::Update() {
    screen.Update(vec2(0.0), true);
    if (buttonApply->state.Released()) {
        globals->window.Fullscreen(checkFullscreen->checked);
        globals->fullscreen = checkFullscreen->checked;
        u64 framerate = 60;
        if (textboxFramerate->textValidate(textboxFramerate->string)) {
            framerate = clamp(WStringToU64(textboxFramerate->string), (u64)30, (u64)300);
            globals->Framerate((f32)framerate);
        }
        textboxFramerate->string = ToWString(ToString(framerate));
        globals->volumeMain = sliderVolumes[0]->value / 100.0;
        globals->volumeMusic = sliderVolumes[1]->value / 100.0;
        globals->volumeEffects = sliderVolumes[2]->value / 100.0;
        for (i32 i = 0; i < 3; i++) {
            textboxVolumes[i]->string = ToWString(ToString(sliderVolumes[i]->value, 10, 1));
        }
    }
    if (buttonBack->state.Released()) {
        globals->gui.nextMenu = MENU_MAIN;
    }
}

void SettingsMenu::Draw(Rendering::DrawingContext &context) {
    screen.Draw(context);
}

void UpgradesMenu::Initialize() {
    ListH *list = new ListH();
    list->fractionWidth = false;
    list->fractionHeight = false;
    list->size = 0.0;
    list->color = vec4(vec3(0.05), 0.8);
    list->highlight = list->color;
    list->padding *= 0.5;

    ListV *listStats = new ListV();
    listStats->fractionWidth = false;
    listStats->fractionHeight = false;
    listStats->size.x = 250.0;
    listStats->size.y = 0.0;
    listStats->margin = 0.0;
    listStats->padding = 0.0;
    listStats->color = 0.0;
    listStats->highlight = 0.0;

    Text *titleText = new Text();
    titleText->fontIndex = globals->gui.fontIndex;
    titleText->alignH = Rendering::CENTER;
    titleText->alignV = Rendering::CENTER;
    titleText->bold = true;
    titleText->fontSize = 24.0;
    titleText->fractionWidth = true;
    titleText->fractionHeight = false;
    titleText->size.x = 1.0;
    titleText->size.y = 0.0;
    titleText->string = globals->ReadLocale("Info");
    AddWidget(listStats, titleText);

    ListH *selectedTowerPriorityList = new ListH();
    selectedTowerPriorityList->fractionWidth = true;
    selectedTowerPriorityList->size.x = 1.0;
    selectedTowerPriorityList->fractionHeight = false;
    selectedTowerPriorityList->size.y = 0.0;
    selectedTowerPriorityList->padding = vec2(0.0);
    selectedTowerPriorityList->margin = vec2(0.0);
    selectedTowerPriorityList->color = 0.0;
    selectedTowerPriorityList->highlight = 0.0;
    Text *selectedTowerPriorityText = new Text();
    selectedTowerPriorityText->color = 1.0;
    selectedTowerPriorityText->size.x = 0.5;
    selectedTowerPriorityText->size.y = 1.0;
    selectedTowerPriorityText->fractionHeight = true;
    selectedTowerPriorityText->alignV = Rendering::CENTER;
    selectedTowerPriorityText->fontIndex = globals->gui.fontIndex;
    selectedTowerPriorityText->fontSize = 18.0;
    selectedTowerPriorityText->string = globals->ReadLocale("Priority");
    towerPriority = new Switch();
    towerPriority->size.x = 0.5;
    towerPriority->size.y = 0.0;
    towerPriority->padding = 0.0;
    for (i32 i = 0; i < 6; i++) {
        Text *priorityText = new Text();
        priorityText->selectable = true;
        priorityText->size.x = 1.0;
        priorityText->size.y = 22.0;
        priorityText->margin = 2.0;
        priorityText->fractionHeight = false;
        priorityText->fontIndex = globals->gui.fontIndex;
        priorityText->fontSize = 18.0;
        priorityText->alignV = Rendering::CENTER;
        priorityText->string = globals->ReadLocale(Entities::Tower::priorityStrings[i]);
        AddWidget(towerPriority, priorityText);
    }
    AddWidget(selectedTowerPriorityList, selectedTowerPriorityText);
    AddWidgetAsDefault(selectedTowerPriorityList, towerPriority);
    towerPriorityHideable = new Hideable(selectedTowerPriorityList);
    AddWidgetAsDefault(listStats, towerPriorityHideable);
    selectedTowerStats = new Text();
    selectedTowerStats->size.x = 1.0;
    selectedTowerStats->color = 1.0;
    selectedTowerStats->fontIndex = globals->gui.fontIndex;
    selectedTowerStats->fontSize = 18.0;
    AddWidget(listStats, selectedTowerStats);

    ListV *listUpgrades = new ListV();
    listUpgrades->fractionWidth = false;
    listUpgrades->fractionHeight = false;
    listUpgrades->size.x = 300.0;
    listUpgrades->size.y = 0.0;
    listUpgrades->margin = 0.0;
    listUpgrades->padding = 0.0;
    listUpgrades->color = 0.0;
    listUpgrades->highlight = 0.0;
    listUpgrades->selectionDefault = 1;

    titleText = new Text();
    titleText->fontIndex = globals->gui.fontIndex;
    titleText->alignH = Rendering::CENTER;
    titleText->alignV = Rendering::CENTER;
    titleText->bold = true;
    titleText->fontSize = 24.0;
    titleText->fractionWidth = true;
    titleText->fractionHeight = false;
    titleText->size.x = 1.0;
    titleText->size.y = 0.0;
    titleText->string = globals->ReadLocale("Upgrades");
    AddWidget(listUpgrades, titleText);

    const char *upgradeNameStrings[] = {
        "Range",
        "Firerate",
        "Accuracy",
        "Damage",
        "Multishot"
    };
    const char *upgradeDescriptionStrings[] = {
        "RangeDescription",
        "FirerateDescription",
        "AccuracyDescription",
        "DamageDescription",
        "MultishotDescription"
    };

    for (i32 i = 0; i < 5; i++) {
        ListV *listV = new ListV();
        listV->fractionHeight = false;
        listV->size = vec2(1.0, 0.0);
        listV->margin *= 0.5;
        listV->padding = 0.0;
        listV->color = 0.0;
        listV->highlight = 0.0;

        ListH *listH = new ListH();
        listH->fractionHeight = false;
        listH->size.y = 0.0;
        listH->margin = 0.0;
        listH->padding = 0.0;
        listH->color = 0.0;
        listH->highlight = 0.0;

        Text *upgradeName = new Text();
        upgradeName->fractionWidth = true;
        upgradeName->size.x = 0.35;
        upgradeName->fractionHeight = true;
        upgradeName->size.y = 1.0;
        upgradeName->margin *= 0.5;
        upgradeName->alignV = Rendering::CENTER;
        upgradeName->fontIndex = globals->gui.fontIndex;
        upgradeName->fontSize = 18.0;
        upgradeName->bold = true;
        upgradeName->string = globals->ReadLocale(upgradeNameStrings[i]);
        AddWidget(listH, upgradeName);

        upgradeStatus[i] = new Text();
        upgradeStatus[i]->fractionWidth = true;
        upgradeStatus[i]->size = vec2(0.4, 0.0);
        upgradeStatus[i]->margin *= 0.5;
        upgradeStatus[i]->alignV = Rendering::CENTER;
        upgradeStatus[i]->fontIndex = globals->gui.fontIndex;
        upgradeStatus[i]->fontSize = 14.0;
        upgradeStatus[i]->string = ToWString("0");
        AddWidget(listH, upgradeStatus[i]);

        upgradeButton[i] = new Button();
        upgradeButton[i]->fractionWidth = true;
        upgradeButton[i]->fractionHeight = true;
        upgradeButton[i]->size.x = 0.25;
        upgradeButton[i]->size.y = 1.0;
        upgradeButton[i]->margin *= 0.5;
        upgradeButton[i]->fontIndex = globals->gui.fontIndex;
        upgradeButton[i]->fontSize = 18.0;
        upgradeButton[i]->string = globals->ReadLocale("Buy");
        AddWidgetAsDefault(listH, upgradeButton[i]);

        AddWidgetAsDefault(listV, listH);

        Text *upgradeDescription = new Text();
        upgradeDescription->alignH = Rendering::CENTER;
        upgradeDescription->fractionWidth = true;
        upgradeDescription->size.x = 1.0;
        upgradeDescription->margin = 0.0;
        upgradeDescription->fontIndex = globals->gui.fontIndex;
        upgradeDescription->fontSize = 14.0;
        upgradeDescription->string = globals->ReadLocale(upgradeDescriptionStrings[i]);
        AddWidget(listV, upgradeDescription);

        upgradeHideable[i] = new Hideable(listV);
        AddWidget(listUpgrades, upgradeHideable[i]);
    }
    AddWidgetAsDefault(list, listStats);
    AddWidget(list, listUpgrades);
    hideable = new Hideable(list);
    AddWidget(&screen, hideable);
}

void UpgradesMenu::Update() {
    if (globals->entities.selectedTower != -1) {
        hideable->hidden = false;
        vec2 towerScreenPos = globals->entities.WorldPosToScreen(globals->entities.towers[globals->entities.selectedTower].physical.pos);
        hideable->position = towerScreenPos - vec2(hideable->sizeAbsolute.x / 2.0, 0.0);

        Entities::Tower &tower = globals->entities.towers.GetMutable(globals->entities.selectedTower);
        towerPriorityHideable->hidden = !Entities::towerHasPriority[tower.type];
        const Entities::TowerUpgradeables &upgradeables = Entities::towerUpgradeables[tower.type];
        for (i32 i = 0; i < 5; i++) {
            upgradeHideable[i]->hidden = !upgradeables.data[i];
        }
        WString costString = "\n" + globals->ReadLocale("Cost:") + ' ';
        if (upgradeables.data[0]) { // Range
            i64 cost = tower.sunkCost / 2;
            f32 newRange = tower.range * 1.25;
            bool canUpgrade = cost <= globals->entities.money;
            upgradeStatus[0]->string =
                ToString(tower.range/10.0) + "m > " + ToString(newRange/10.0) + "m" +
                costString + ToString(cost);
            upgradeButton[0]->highlightBG = vec4(canUpgrade? colorHighlightMedium : vec3(0.8, 0.1, 0.1), 1.0);
            if (upgradeButton[0]->state.Released() && canUpgrade) {
                tower.range = newRange;
                tower.field.basis.circle.r = newRange;
                tower.sunkCost += cost;
                globals->entities.money -= cost;
            }
        }
        if (upgradeables.data[1]) { // Firerate
            i64 cost = tower.sunkCost / 2;
            f32 newFirerate = tower.shootInterval / 1.5;
            bool canUpgrade = cost <= globals->entities.money && newFirerate >= 1.0/18.1;
            upgradeStatus[1]->string =
                ToString(1.0/tower.shootInterval) + "r/s > " + ToString(1.0/newFirerate) + "r/s" +
                costString + ToString(cost);
            upgradeButton[1]->highlightBG = vec4(canUpgrade? colorHighlightMedium : vec3(0.8, 0.1, 0.1), 1.0);
            if (upgradeButton[1]->state.Released() && canUpgrade) {
                tower.shootInterval = newFirerate;
                tower.sunkCost += cost;
                globals->entities.money -= cost;
            }
        }
        if (upgradeables.data[2]) { // Accuracy
            i64 cost = tower.sunkCost / 5;
            Degrees32 newSpread = tower.bulletSpread.value() / 1.5;
            bool canUpgrade = cost <= globals->entities.money;
            upgradeStatus[2]->string =
                ToString(tower.bulletSpread.value()) + "° > " + ToString(newSpread.value()) + "°" +
                costString + ToString(cost);
            upgradeButton[2]->highlightBG = vec4(canUpgrade? colorHighlightMedium : vec3(0.8, 0.1, 0.1), 1.0);
            if (upgradeButton[2]->state.Released() && canUpgrade) {
                tower.bulletSpread = newSpread;
                tower.sunkCost += cost;
                globals->entities.money -= cost;
            }
        }
        if (upgradeables.data[3]) { // Damage
            i64 cost = tower.sunkCost / 2;
            i32 newDamage = tower.damage * 3 / 2;
            bool canUpgrade = cost <= globals->entities.money;
            upgradeStatus[3]->string =
                ToString(tower.damage) + " > " + ToString(newDamage) +
                costString + ToString(cost);
            upgradeButton[3]->highlightBG = vec4(canUpgrade? colorHighlightMedium : vec3(0.8, 0.1, 0.1), 1.0);
            if (upgradeButton[3]->state.Released() && canUpgrade) {
                tower.damage = newDamage;
                tower.bulletExplosionDamage *= 2;
                tower.sunkCost += cost;
                globals->entities.money -= cost;
            }
        }
        if (upgradeables.data[4]) { // Multishot
            i64 cost = tower.sunkCost;
            i32 newBulletCount = tower.bulletCount * 2;
            if (tower.bulletCount >= 2) {
                newBulletCount = tower.bulletCount * 3 / 2;
                cost = cost * (newBulletCount - tower.bulletCount) / newBulletCount;
            }
            bool canUpgrade = cost <= globals->entities.money && newBulletCount <= 60;
            upgradeStatus[4]->string =
                ToString(tower.bulletCount) + " > " + ToString(newBulletCount) +
                costString + ToString(cost);
            upgradeButton[4]->highlightBG = vec4(canUpgrade? colorHighlightMedium : vec3(0.8, 0.1, 0.1), 1.0);
            if (upgradeButton[4]->state.Released() && canUpgrade) {
                tower.bulletCount = newBulletCount;
                tower.sunkCost += cost;
                globals->entities.money -= cost;
            }
        }
        selectedTowerStats->string =
            globals->ReadLocale("Kills") + ": " + ToString(tower.kills) + "\n"
            + globals->ReadLocale("Damage") + ": " + ToString(tower.damageDone);
        if (towerPriority->changed) {
            tower.priority = (Entities::Tower::TargetPriority)(towerPriority->choice);
        }
    } else {
        hideable->hidden = true;
    }
    screen.Update(vec2(0.0), !globals->entities.focusMenu); // Hideable will handle selection culling
}

void UpgradesMenu::Draw(Rendering::DrawingContext &context) {
    screen.Draw(context);
}

void PlayMenu::Initialize() {
    ListH *screenListH = new ListH();
    screenListH->fractionWidth = true;
    screenListH->size.x = 1.0;
    screenListH->padding = vec2(0.0);
    screenListH->margin = vec2(0.0);
    screenListH->color = 0.0;
    screenListH->highlight = 0.0;
    screenListH->occludes = false;
    AddWidget(&screen, screenListH);
    Widget *spacer = new Widget();
    spacer->fractionWidth = true;
    spacer->size.x = 1.0;
    AddWidget(screenListH, spacer);
    list = new ListV();
    list->fractionHeight = true;
    list->fractionWidth = false;
    list->margin = 0.0;
    list->size = vec2(300.0, 1.0);
    list->selectionDefault = 1;
    AddWidgetAsDefault(screenListH, list);

    Text *towerHeader = new Text();
    towerHeader->fontIndex = globals->gui.fontIndex;
    towerHeader->alignH = Rendering::CENTER;
    towerHeader->string = globals->ReadLocale("Towers");
    AddWidget(list, towerHeader);

    ListH *gridBase = new ListH();
    gridBase->fractionWidth = true;
    gridBase->size.x = 1.0;
    gridBase->fractionHeight = false;
    gridBase->size.y = 0.0;
    gridBase->padding = vec2(0.0);
    gridBase->margin = vec2(0.0);
    gridBase->color = 0.0;
    gridBase->highlight = 0.0;
    gridBase->selectionDefault = 0;

    Button *halfWidth = new Button();
    halfWidth->fractionWidth = true;
    halfWidth->size.x = 0.5;
    halfWidth->fractionHeight = false;
    halfWidth->size.y = 32.0;
    halfWidth->fontIndex = globals->gui.fontIndex;
    halfWidth->fontSize = 20.0;

    towerButtons.Resize(Entities::TOWER_MAX_RANGE + 1);
    for (i32 i = 0; i < towerButtons.size; i+=2) {
        ListH *grid = new ListH(*gridBase);
        for (i32 j = 0; j < 2; j++) {
            i32 index = i+j;
            if (index > towerButtons.size) break;
            towerButtons[index] = new Button(*halfWidth);
            towerButtons[index]->string = globals->ReadLocale(Entities::towerStrings[index]);
            towerButtons[index]->highlightBG = Entities::Tower(Entities::TowerType(index)).color;
            AddWidget(grid, towerButtons[index]);
        }
        towerButtonLists.Append(grid);
        AddWidget(list, grid);
    }

    towerInfo = new Text();
    towerInfo->size.x = 1.0;
    towerInfo->color = vec4(1.0);
    towerInfo->fontIndex = globals->gui.fontIndex;
    towerInfo->fontSize = 18.0;
    towerInfo->string = ToWString("$MONEY");
    AddWidget(list, towerInfo);

    spacer = new Widget();
    spacer->fractionHeight = true;
    spacer->size.y = 1.0;
    AddWidget(list, spacer);

    Button *fullWidth = new Button();
    fullWidth->fractionWidth = true;
    fullWidth->size.x = 1.0;
    fullWidth->fractionHeight = false;
    fullWidth->size.y = 32.0;
    fullWidth->fontIndex = globals->gui.fontIndex;

    ListH *waveList = new ListH(*gridBase);

    waveTitle = new Text();
    waveTitle->size.x = 0.5;
    waveTitle->size.y = 1.0;
    waveTitle->fractionHeight = true;
    waveTitle->alignV = Rendering::CENTER;
    waveTitle->colorOutline = vec4(1.0, 0.0, 0.5, 1.0);
    waveTitle->color = vec4(1.0);
    waveTitle->outline = true;
    waveTitle->fontIndex = globals->gui.fontIndex;
    waveTitle->fontSize = 30.0;
    // waveTitle->bold = true;
    waveTitle->margin.y = 0.0;
    waveTitle->string = ToWString("Nothing");
    AddWidget(waveList, waveTitle);
    buttonStartWave = new Button(*halfWidth);
    buttonStartWave->string = globals->ReadLocale("Start Wave");
    buttonStartWave->size.y = 32.0;
    buttonStartWave->keycodeActivators = {KC_GP_BTN_START, KC_KEY_SPACE};
    AddWidgetAsDefault(waveList, buttonStartWave);

    AddWidget(list, waveList);

    waveInfo = new Text();
    waveInfo->size.x = 1.0;
    waveInfo->color = vec4(1.0);
    waveInfo->fontIndex = globals->gui.fontIndex;
    waveInfo->fontSize = 20.0;
    waveInfo->string = ToWString("Nothing");
    AddWidget(list, waveInfo);

    buttonMenu = new Button(*fullWidth);
    buttonMenu->string = globals->ReadLocale("Menu");
    buttonMenu->keycodeActivators = {KC_GP_BTN_SELECT, KC_KEY_ESC};
    AddWidget(list, buttonMenu);

    delete gridBase;
    delete halfWidth;
    delete fullWidth;

    upgradesMenu.Initialize();
}

void PlayMenu::Update() {
    upgradesMenu.Update();
    WString towerInfoString = globals->ReadLocale("Money") + ": $" + ToString(globals->entities.money);
    i32 textTower = -1;
    if (globals->entities.placeMode) {
        textTower = globals->entities.towerType;
    } else {
        for (i32 i = 0; i < towerButtons.size; i++) {
            if (towerButtons[i]->highlighted) {
                textTower = i;
            }
        }
    }
    { // Make the grid work more nicely (hacky)
        i32 selection = -1;
        for (ListH *list : towerButtonLists) {
            if (list->selection >= 0) {
                selection = list->selection;
                break;
            }
        }
        if (selection != -1) {
            for (ListH *list : towerButtonLists) {
                list->selectionDefault = selection;
            }
        }
    }
    if (textTower != -1) {
        towerInfoString += "\n" + globals->ReadLocale("Cost") + ": $" + ToString(Entities::towerCosts[textTower]) + "\n" + globals->ReadLocale(Entities::towerDescriptions[textTower]);
    }
    towerInfo->string = towerInfoString;
    waveTitle->string = globals->ReadLocale("Wave") + ": " + ToString(globals->entities.wave);
    waveInfo->string =
        globals->ReadLocale("Wave Hitpoints Left") + ": " + ToString(globals->entities.hitpointsLeft) + "\n"
        + globals->ReadLocale("Lives") + ": " + ToString(globals->entities.lives);
    screen.Update(vec2(0.0), globals->entities.focusMenu);
    if (buttonMenu->state.Released()) {
        globals->gui.nextMenu = MenuEnum::MENU_MAIN;
        globals->objects.paused = true;
        if (globals->entities.waveActive) {
            buttonStartWave->string = globals->ReadLocale("Resume");
        }
    }
}

void PlayMenu::Draw(Rendering::DrawingContext &context) {
    upgradesMenu.Draw(context);
    screen.Draw(context);
}

//
//      Widget implementations beyond this point
//

Widget::Widget() : children(), margin(8.0), size(1.0), fractionWidth(true), fractionHeight(true), minSize(0.0), maxSize(-1.0), position(0.0), sizeAbsolute(0.0), positionAbsolute(0.0), depth(0), selectable(false), highlighted(false), occludes(false) {}

void Widget::UpdateSize(vec2 container) {
    sizeAbsolute = vec2(0.0);
    if (size.x > 0.0) {
        sizeAbsolute.x = fractionWidth ? (container.x * size.x - margin.x * 2.0) : size.x;
    } else {
        sizeAbsolute.x = 0.0;
    }
    if (size.y > 0.0) {
        sizeAbsolute.y = fractionHeight ? (container.y * size.y - margin.y * 2.0) : size.y;
    } else {
        sizeAbsolute.y = 0.0;
    }
    LimitSize();
}

void Widget::LimitSize() {
    if (maxSize.x >= 0.0) {
        sizeAbsolute.x = median(minSize.x, sizeAbsolute.x, maxSize.x);
    } else {
        sizeAbsolute.x = max(minSize.x, sizeAbsolute.x);
    }
    if (maxSize.y >= 0.0) {
        sizeAbsolute.y = median(minSize.y, sizeAbsolute.y, maxSize.y);
    } else {
        sizeAbsolute.y = max(minSize.y, sizeAbsolute.y);
    }
}

void Widget::PushScissor(Rendering::DrawingContext &context) const {
    if (sizeAbsolute.x != 0.0 && sizeAbsolute.y != 0.0) {
        vec2i topLeft = vec2i(
            positionAbsolute.x * globals->gui.scale,
            positionAbsolute.y * globals->gui.scale
        );
        vec2i botRight = vec2i(
            ceil((positionAbsolute.x + sizeAbsolute.x) * globals->gui.scale),
            ceil((positionAbsolute.y + sizeAbsolute.y) * globals->gui.scale)
        );
        globals->rendering.PushScissor(context, topLeft, botRight);
    }
}

void Widget::PopScissor(Rendering::DrawingContext &context) const {
    if (sizeAbsolute.x != 0.0 && sizeAbsolute.y != 0.0) {
        globals->rendering.PopScissor(context);
    }
}

void Widget::Update(vec2 pos, bool selected) {
    pos += margin + position;
    positionAbsolute = pos;
    highlighted = selected;
    for (Widget* child : children) {
        child->Update(pos, selected);
    }
}

void Widget::Draw(Rendering::DrawingContext &context) const {
    for (const Widget* child : children) {
        child->Draw(context);
    }
}

void Widget::OnHide() {
    for (Widget *child : children) {
        child->OnHide();
    }
}

bool Widget::Selectable() const {
    return selectable;
}

bool Widget::MouseOver() const {
    vec2 mouse;
    if (globals->gui.usingMouse) {
        mouse = vec2(globals->input.cursor) / globals->gui.scale;
    } else {
        // if (globals->gui.currentMenu == MENU_PLAY) {
        //     mouse = globals->entities.WorldPosToScreen(globals->entities.mouse);
        // } else {
            mouse = -1.0;
        // }
    }
    return mouse.x == median(positionAbsolute.x, mouse.x, positionAbsolute.x + sizeAbsolute.x)
        && mouse.y == median(positionAbsolute.y, mouse.y, positionAbsolute.y + sizeAbsolute.y);
}

void Widget::FindMouseoverDepth(i32 actualDepth) {
    if (actualDepth <= globals->gui.mouseoverDepth) return;
    if (MouseOver()) {
        if (occludes) {
            globals->gui.mouseoverDepth = actualDepth;
            globals->gui.mouseoverWidget = this;
        }
        actualDepth++;
        for (Widget *child : children) {
            child->FindMouseoverDepth(actualDepth);
        }
    }
}

Screen::Screen() {
    margin = vec2(0.0);
}

void Screen::Update(vec2 pos, bool selected) {
    UpdateSize(globals->rendering.screenSize / globals->gui.scale);
    Widget::Update(pos + position, selected);
    if (selected) {
        FindMouseoverDepth(0);
    }
}

void Screen::UpdateSize(vec2 container) {
    sizeAbsolute = container - margin * 2.0;
    for (Widget* child : children) {
        child->UpdateSize(sizeAbsolute);
    }
}

List::List() : padding(8.0), color(0.05, 0.05, 0.05, 0.9), highlight(0.05, 0.05, 0.05, 0.9), select(0.2, 0.2, 0.2, 0.0), selection(-2), selectionDefault(-1) { occludes = true; }

bool List::UpdateSelection(bool selected, u8 keyCodeSelect, u8 keyCodeBack, u8 keyCodeIncrement, u8 keyCodeDecrement) {
    highlighted = selected;
    if (selected) {
        if (globals->gui.controlDepth == depth) {
            if (selection >= 0 && selection < children.size && globals->objects.Released(keyCodeSelect)) {
                globals->gui.controlDepth = children[selection]->depth;
            }
            if (globals->objects.Pressed(keyCodeIncrement)) {
                for (selection = max(selection+1, 0); selection < children.size; selection++) {
                    if (children[selection]->Selectable()) {
                        break;
                    }
                }
                if (selection == children.size) {
                    for (selection = 0; selection < children.size; selection++) {
                        if (children[selection]->Selectable()) {
                            break;
                        }
                    }
                }
                if (selection == children.size) {
                    selection = -1;
                }
            } else if (globals->objects.Pressed(keyCodeDecrement)) {
                if (selection < 0) {
                    selection = children.size - 1;
                } else {
                    --selection;
                }
                for (; selection >= 0; selection--) {
                    if (children[selection]->Selectable()) {
                        break;
                    }
                }
                if (selection == -1) {
                    for (selection = children.size-1; selection >= 0; selection--) {
                        if (children[selection]->Selectable()) {
                            break;
                        }
                    }
                }
            }
            if (selection == -2) {
                selection = selectionDefault;
            }
        } else if (globals->gui.controlDepth == depth+1 && globals->objects.Released(keyCodeBack)) {
            globals->gui.controlDepth = depth;
        }
        if (globals->gui.controlDepth > depth) {
            highlighted = false;
        }
    } else {
        selection = -2;
    }
    if (globals->gui.controlDepth == depth && selected) {
        bool reselect = false;
        if (globals->gui.usingMouse && globals->input.cursor != globals->input.cursorPrevious) {
            if (MouseOver()) {
                reselect = true;
            }
            selection = -1;
        } else if (selection == -1 && !globals->gui.usingMouse && globals->rawInput.AnyGP.state != 0) {
            selection = -2;
        }
        return reselect;
    }
    return false;
}

void List::Draw(Rendering::DrawingContext &context) const {
    if ((highlighted ? highlight.a : color.a) > 0.0) {
        globals->rendering.DrawQuad(context, Rendering::texBlank, highlighted ? highlight : color, positionAbsolute * globals->gui.scale, vec2(1.0), sizeAbsolute * globals->gui.scale);
    }
    if (selection >= 0 && select.a > 0.0) {
        vec2 selectionPos = children[selection]->positionAbsolute;
        vec2 selectionSize = children[selection]->sizeAbsolute;
        globals->rendering.DrawQuad(context, Rendering::texBlank, select, selectionPos * globals->gui.scale, vec2(1.0), selectionSize * globals->gui.scale);
    }
    PushScissor(context);
    Widget::Draw(context);
    PopScissor(context);
}

void ListV::UpdateSize(vec2 container) {
    sizeAbsolute = vec2(0.0);
    if (size.x > 0.0) {
        sizeAbsolute.x = fractionWidth ? (container.x * size.x - margin.x * 2.0) : size.x;
    } else {
        sizeAbsolute.x = padding.x * 2.0;
    }
    if (size.y > 0.0) {
        sizeAbsolute.y = fractionHeight ? (container.y * size.y - margin.y * 2.0) : size.y;
    } else {
        sizeAbsolute.y = padding.y * 2.0;
    }
    LimitSize();
    vec2 sizeForInheritance = sizeAbsolute - padding * 2.0;
    if (size.x == 0.0) {
        for (Widget* child : children) {
            child->UpdateSize(sizeForInheritance);
            vec2 childSize = child->GetSize();
            sizeAbsolute.x = max(sizeAbsolute.x, childSize.x + padding.x * 2.0);
        }
    }
    sizeForInheritance = sizeAbsolute - padding * 2.0;
    for (Widget* child : children) {
        if (child->size.y == 0.0) {
            child->UpdateSize(sizeForInheritance);
            sizeForInheritance.y -= child->GetSize().y;
        } else {
            if (!child->fractionHeight) {
                sizeForInheritance.y -= child->size.y + child->margin.y * 2.0;
            }
        }
    }
    for (Widget* child : children) {
        child->UpdateSize(sizeForInheritance);
        vec2 childSize = child->GetSize();
        if (size.x == 0.0) {
            sizeAbsolute.x = max(sizeAbsolute.x, childSize.x + padding.x * 2.0);
        }
        if (size.y == 0.0) {
            sizeAbsolute.y += childSize.y;
        }
    }
    LimitSize();
}

void ListV::Update(vec2 pos, bool selected) {
    pos += margin + position;
    positionAbsolute = pos;
    const bool mouseSelect = UpdateSelection(selected, KC_GP_BTN_A, KC_GP_BTN_B, KC_GP_AXIS_LS_DOWN, KC_GP_AXIS_LS_UP);
    pos += padding;
    if (mouseSelect) {
        f32 childY = pos.y;
        for (selection = 0; selection < children.size; selection++) {
            Widget *child = children[selection];
            if (!child->Selectable()) {
                childY += child->GetSize().y;
                continue;
            }
            child->positionAbsolute.x = pos.x + child->margin.x;
            child->positionAbsolute.y = childY + child->margin.y;
            if (child->MouseOver()) {
                break;
            }
            childY += child->GetSize().y;
        }
        if (selection == children.size) {
            selection = -1;
        }
    }
    for (i32 i = 0; i < children.size; i++) {
        Widget *child = children[i];
        child->Update(pos, selected && i == selection);
        pos.y += child->GetSize().y;
    }
}

ListH::ListH() {
    color = vec4(0.0, 0.0, 0.0, 0.9);
    highlight = vec4(0.1, 0.1, 0.1, 0.9);
    occludes = true;
}

void ListH::UpdateSize(vec2 container) {
    sizeAbsolute = vec2(0.0);
    if (size.x > 0.0) {
        sizeAbsolute.x = fractionWidth ? (container.x * size.x - margin.x * 2.0) : size.x;
    } else {
        sizeAbsolute.x = padding.x * 2.0;
    }
    if (size.y > 0.0) {
        sizeAbsolute.y = fractionHeight ? (container.y * size.y - margin.y * 2.0) : size.y;
    } else {
        sizeAbsolute.y = padding.y * 2.0;
    }
    LimitSize();
    vec2 sizeForInheritance = sizeAbsolute - padding * 2.0;
    if (size.y == 0.0) {
        for (Widget* child : children) {
            child->UpdateSize(sizeForInheritance);
            vec2 childSize = child->GetSize();
            sizeAbsolute.y = max(sizeAbsolute.y, childSize.y + padding.y * 2.0);
        }
        sizeForInheritance = sizeAbsolute - padding * 2.0;
    }
    for (Widget* child : children) {
        if (child->size.x == 0.0) {
            child->UpdateSize(sizeForInheritance);
            sizeForInheritance.x -= child->GetSize().x;
        } else {
            if (!child->fractionWidth) {
                sizeForInheritance.x -= child->size.x + child->margin.x * 2.0;
            }
        }
    }
    for (Widget* child : children) {
        child->UpdateSize(sizeForInheritance);
        vec2 childSize = child->GetSize();
        if (size.x == 0.0) {
            sizeAbsolute.x += childSize.x;
        }
        if (size.y == 0.0) {
            sizeAbsolute.y = max(sizeAbsolute.y, childSize.y + padding.y * 2.0);
        }
    }
    LimitSize();
}

void ListH::Update(vec2 pos, bool selected) {
    pos += margin + position;
    positionAbsolute = pos;
    const bool mouseSelect = UpdateSelection(selected, KC_GP_BTN_A, KC_GP_BTN_B, KC_GP_AXIS_LS_RIGHT, KC_GP_AXIS_LS_LEFT);
    pos += padding;
    if (mouseSelect) {
        f32 childX = pos.x;
        for (selection = 0; selection < children.size; selection++) {
            Widget *child = children[selection];
            if (child->Selectable()) {
                child->positionAbsolute.x = childX + child->margin.x;
                child->positionAbsolute.y = pos.y + child->margin.y;
                if (child->MouseOver()) {
                    break;
                }
            }
            childX += child->GetSize().x;
        }
        if (selection == children.size) {
            selection = -1;
        }
    }
    for (i32 i = 0; i < children.size; i++) {
        Widget *child = children[i];
        child->Update(pos, selected && i == selection);
        pos.x += child->GetSize().x;
    }
}

Switch::Switch() : choice(0), open(false), changed(false) {
    selectable = true;
    selectionDefault = 0;
    color = vec4(vec3(0.2), 0.9);
    highlight = vec4(colorHighlightMedium, 0.9);
    select = vec4(colorHighlightMedium, 0.9);
}

void Switch::UpdateSize(vec2 container) {
    if (open) {
        ListV::UpdateSize(container);
    } else {
        sizeAbsolute = vec2(0.0);
        if (size.x > 0.0) {
            sizeAbsolute.x = fractionWidth ? (container.x * size.x - margin.x * 2.0) : size.x;
        } else {
            sizeAbsolute.x = padding.x * 2.0;
        }
        if (size.y > 0.0) {
            sizeAbsolute.y = fractionHeight ? (container.y * size.y - margin.y * 2.0) : size.y;
        } else {
            sizeAbsolute.y = padding.y * 2.0;
        }
        LimitSize();
        Widget *child = children[choice];
        vec2 sizeForInheritance = sizeAbsolute - padding * 2.0;
        if (size.x == 0.0) {
            child->UpdateSize(sizeForInheritance);
            vec2 childSize = child->GetSize();
            sizeAbsolute.x = max(sizeAbsolute.x, childSize.x + padding.x * 2.0);
        }
        sizeForInheritance = sizeAbsolute - padding * 2.0;
        if (child->size.y == 0.0) {
            child->UpdateSize(sizeForInheritance);
            sizeForInheritance.y -= child->GetSize().y;
        } else {
            if (!child->fractionHeight) {
                sizeForInheritance.y -= child->size.y + child->margin.y * 2.0;
            }
        }
        child->UpdateSize(sizeForInheritance);
        vec2 childSize = child->GetSize();
        if (size.x == 0.0) {
            sizeAbsolute.x = max(sizeAbsolute.x, childSize.x + padding.x * 2.0);
        }
        if (size.y == 0.0) {
            sizeAbsolute.y += childSize.y;
        }
        LimitSize();
    }
}

void Switch::Update(vec2 pos, bool selected) {
    changed = false;
    if (open) {
        ListV::Update(pos, selected);
        if (globals->objects.Released(KC_MOUSE_LEFT) || globals->objects.Released(KC_GP_BTN_A)) {
            if (selection >= 0) {
                choice = selection;
                changed = true;
            }
            open = false;
        }
        if (globals->objects.Released(KC_GP_BTN_B)) {
            open = false;
        }
        if (!open) {
            globals->gui.controlDepth = parentDepth;
        }
    } else {
        highlighted = selected;
        positionAbsolute = pos + margin;
        if (globals->objects.Pressed(KC_MOUSE_LEFT) && MouseOver()) {
            open = true;
        }
        if (selected && globals->objects.Released(KC_GP_BTN_A)) {
            open = true;
        }
        if (open) {
            globals->gui.controlDepth = depth;
            selection = choice;
        }
        children[choice]->Update(pos + padding + margin + position, selected);
    }
}

void Switch::Draw(Rendering::DrawingContext &context) const {
    if (color.a > 0.0) {
        globals->rendering.DrawQuad(context, Rendering::texBlank, (highlighted && !open) ? highlight : color, positionAbsolute * globals->gui.scale, vec2(1.0), sizeAbsolute * globals->gui.scale);
    }
    PushScissor(context);
    if (open) {
        if (selection >= 0 && select.a > 0.0) {
            Widget *child = children[selection];
            vec2 selectionPos = child->positionAbsolute - child->margin;
            vec2 selectionSize = child->sizeAbsolute + child->margin * 2.0;
            globals->rendering.DrawQuad(context, Rendering::texBlank, select, selectionPos * globals->gui.scale, vec2(1.0), selectionSize * globals->gui.scale);
        }
        Widget::Draw(context);
    } else {
        children[choice]->Draw(context);
    }
    PopScissor(context);
}

void Switch::OnHide() {
    Widget::OnHide();
    open = false;
    globals->gui.controlDepth = parentDepth;
}

Text::Text() : stringFormatted(), string(), padding(0.1), fontSize(32.0), fontIndex(1), bold(false), paddingEM(true), alignH(Rendering::LEFT), alignV(Rendering::TOP), color(vec3(1.0), 1.0), colorOutline(vec3(0.0), 1.0), highlight(vec3(0.0), 1.0), highlightOutline(vec3(1.0), 1.0), outline(false) {
    size.y = 0.0;
}

void Text::UpdateSize(vec2 container) {
    if (size.x > 0.0) {
        sizeAbsolute.x = fractionWidth ? (container.x * size.x - margin.x * 2.0) : size.x;
    } else {
        sizeAbsolute.x = globals->rendering.StringWidth(stringFormatted, fontIndex) * fontSize + padding.x * (paddingEM ? fontSize * 2.0 : 2.0);
    }
    if (size.y > 0.0) {
        sizeAbsolute.y = fractionHeight ? (container.y * size.y - margin.y * 2.0) : size.y;
    } else {
        sizeAbsolute.y = Rendering::StringHeight(stringFormatted) * fontSize + padding.y * (paddingEM ? fontSize * 2.0 : 2.0);
    }
    LimitSize();
}

void Text::Update(vec2 pos, bool selected) {
    if (size.x != 0.0) {
        stringFormatted = globals->rendering.StringAddNewlines(string, fontIndex, sizeAbsolute.x / fontSize);
    } else {
        stringFormatted = string;
    }
    Widget::Update(pos, selected);
}

void Text::Draw(Rendering::DrawingContext &context) const {
    PushScissor(context);
    vec2 paddingAbsolute = padding;
    if (paddingEM) paddingAbsolute *= fontSize;
    vec2 drawPos = (positionAbsolute + paddingAbsolute) * globals->gui.scale;
    vec2 scale = vec2(fontSize) * globals->gui.scale;
    vec2 textArea = (sizeAbsolute - paddingAbsolute * 2.0) * globals->gui.scale;
    if (alignH == Rendering::CENTER) {
        drawPos.x += textArea.x * 0.5;
    } else if (alignH == Rendering::RIGHT) {
        drawPos.x += textArea.x;
    }
    if (alignV == Rendering::CENTER) {
        drawPos.y += textArea.y * 0.5;
    } else if (alignV == Rendering::BOTTOM) {
        drawPos.y += textArea.y;
    }
    f32 bounds = bold ? 0.425 : 0.525;
    if (outline) {
        globals->rendering.DrawText(context, stringFormatted, fontIndex, highlighted? highlightOutline : colorOutline, drawPos, scale, alignH, alignV, textArea.x, 0.1, bounds - 0.2);
    }
    globals->rendering.DrawText(context, stringFormatted, fontIndex, highlighted? highlight : color, drawPos, scale, alignH, alignV, textArea.x, 0.0, bounds);
    PopScissor(context);
}

Image::Image() : texIndex(0) { occludes = true; }

void Image::Draw(Rendering::DrawingContext &context) const {
    globals->rendering.DrawQuad(context, texIndex, vec4(1.0), positionAbsolute * globals->gui.scale, vec2(1.0), sizeAbsolute * globals->gui.scale);
}

Button::Button() : string(), colorBG(vec3(0.15), 0.9), highlightBG(colorHighlightMedium, 0.9), colorText(vec3(1.0), 1.0), highlightText(vec3(0.0), 1.0), fontIndex(1), fontSize(28.0), state(), keycodeActivators() {
    state.canRepeat = false;
    selectable = true;
    occludes = true;
}

void Button::Update(vec2 pos, bool selected) {
    Widget::Update(pos, selected);
    {
        bool mouseoverNew = MouseOver();
        if (mouseoverNew && !mouseover) {
            globals->gui.sndClickSoft.Play();
        }
        mouseover = mouseoverNew;
    }
    state.Tick(0.0);
    if (mouseover) {
        if (globals->objects.Pressed(KC_MOUSE_LEFT)) {
            state.Press();
        }
        if (globals->objects.Released(KC_MOUSE_LEFT)) {
            state.Release();
        }
    }
    if (globals->gui.controlDepth == depth) {
        if (selected) {
            if (globals->objects.Pressed(KC_GP_BTN_A)) {
                state.Press();
            }
            if (globals->objects.Released(KC_GP_BTN_A)) {
                state.Release();
            }
        }
        for (u8 kc : keycodeActivators) {
            if (globals->objects.Pressed(kc)) {
                state.Press();
            }
            if (globals->objects.Released(kc)) {
                state.Release();
            }
        }
    }
    if (state.Pressed()) {
        globals->gui.sndClickIn.Play();
    }
    if (state.Released()) {
        globals->gui.sndClickOut.Play();
    }
    highlighted = selected || mouseover || state.Down();
}

void Button::Draw(Rendering::DrawingContext &context) const {
    PushScissor(context);
    f32 scale;
    if (state.Down()) {
        scale = 0.9;
    } else {
        scale = 1.0;
    }
    scale *= globals->gui.scale;
    vec2 drawPos = (positionAbsolute + sizeAbsolute * 0.5) * globals->gui.scale;
    globals->rendering.DrawQuad(context, Rendering::texBlank, highlighted ? highlightBG : colorBG, drawPos, vec2(1.0), sizeAbsolute * scale, vec2(0.5));
    globals->rendering.DrawText(context, string, fontIndex,  highlighted ? highlightText : colorText, drawPos, vec2(fontSize * scale), Rendering::CENTER, Rendering::CENTER, sizeAbsolute.x * globals->gui.scale);
    PopScissor(context);
}

Checkbox::Checkbox() : colorOff(vec3(0.15), 0.9), highlightOff(colorHighlightLow, 0.9), colorOn(colorHighlightMedium, 1.0), highlightOn(colorHighlightHigh, 1.0), transition(0.0), checked(false) {
    selectable = true;
    size = vec2(48.0, 24.0);
    fractionWidth = false;
    fractionHeight = false;
    occludes = true;
}

void Checkbox::Update(vec2 pos, bool selected) {
    Widget::Update(pos, selected);
    const bool mouseover = MouseOver();
    if (globals->gui.controlDepth != depth) {
        highlighted = false;
    }
    if (mouseover) {
        highlighted = true;
        if (globals->objects.Released(KC_MOUSE_LEFT)) {
            checked = !checked;
            if (checked) {
                globals->gui.sndPopHigh.Play();
            } else {
                globals->gui.sndPopLow.Play();
            }
        }
    }
    if (globals->gui.controlDepth == depth) {
        if (selected) {
            if (globals->objects.Released(KC_GP_BTN_A)) {
                checked = !checked;
                if (checked) {
                    globals->gui.sndPopHigh.Play();
                } else {
                    globals->gui.sndPopLow.Play();
                }
            }
        }
    }
    if (checked) {
        transition = decay(transition, 1.0, 0.05, globals->objects.timestep);
    } else {
        transition = decay(transition, 0.0, 0.05, globals->objects.timestep);
    }
}

void Checkbox::Draw(Rendering::DrawingContext &context) const {
    const vec4 &colorOnActual = highlighted ? highlightOn : colorOn;
    const vec4 &colorOffActual = highlighted ? highlightOff : colorOff;
    vec4 colorActual = lerp(colorOffActual, colorOnActual, transition);
    vec2 switchPos = (positionAbsolute + sizeAbsolute * vec2(lerp(0.0625, 0.5625, transition), 0.125)) * globals->gui.scale;
    globals->rendering.DrawQuad(context, Rendering::texBlank, colorActual, positionAbsolute * globals->gui.scale, vec2(1.0), sizeAbsolute * globals->gui.scale);
    globals->rendering.DrawQuad(context, Rendering::texBlank, vec4(vec3(0.0), 0.8), switchPos, vec2(1.0), (sizeAbsolute * vec2(0.375, 0.75)) * globals->gui.scale);
}

TextBox::TextBox() : string(), colorBG(vec3(0.15), 0.9), highlightBG(vec3(0.2), 0.9), errorBG(0.1, 0.0, 0.0, 0.9), colorText(vec3(1.0), 1.0), highlightText(vec3(1.0), 1.0), errorText(1.0, 0.5, 0.5, 1.0), padding(2.0), cursor(0), fontIndex(1), fontSize(17.39), cursorBlinkTimer(0.0), alignH(Rendering::LEFT), textFilter(TextFilterBasic), textValidate(TextValidateAll), entry(false), multiline(false) {
    // selectable = true;
    occludes = true;
    fractionWidth = false;
    fractionHeight = false;
    size.x = 200.0;
    size.y = 0.0;
    minSize.y = 24.0;
}

inline bool IsWhitespace(const char32 &c) {
    return c == ' ' || c == '\t' || c == '\n' || c == 0;
}

bool TextFilterBasic(char32 c) {
    return c >= ' ' && c <= '~';
}

bool TextFilterWordSingle(char32 c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool TextFilterWordMultiple(char32 c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == ' ';
}

bool TextFilterDecimals(char32 c) {
    return c == '-' || c == '.' || (c >= '0' && c <= '9');
}

bool TextFilterDecimalsPositive(char32 c) {
    return c == '.' || (c >= '0' && c <= '9');
}

bool TextFilterIntegers(char32 c) {
    return c == '-' || (c >= '0' && c <= '9');
}

bool TextFilterDigits(char32 c) {
    return c >= '0' && c <= '9';
}

bool TextValidateAll(const WString &string) {
    return true;
}

bool TextValidateNonempty(const WString &string) {
    return string.size != 0;
}

bool TextValidateDecimals(const WString &string) {
    if (string.size == 0) return false;
    if (string.size == 1 && (string[0] == '.' || string[0] == '-')) return false;
    if (string.size == 2 && (string[0] == '-' && string[1] == '.')) return false;
    i32 cur;
    if (string[0] == '-') cur = 1; else cur = 0;
    bool point = false;
    for (; cur < string.size; cur++) {
        const char32 &c = string[cur];
        if (c == '.') {
            if (point) return false;
            point = true;
            continue;
        }
        if (!TextFilterDigits(c)) return false;
    }
    return true;
}

bool TextValidateDecimalsPositive(const WString &string) {
    if (string.size == 0) return false;
    if (string.size == 1 && string[0] == '.') return false;
    bool point = false;
    for (i32 cur = 0; cur < string.size; cur++) {
        const char32 &c = string[cur];
        if (c == '.') {
            if (point) return false;
            point = true;
            continue;
        }
        if (!TextFilterDigits(c)) return false;
    }
    return true;
}

bool TextValidateIntegers(const WString &string) {
    if (string.size == 0) return false;
    i32 cur;
    if (string[0] == '-') cur = 1; else cur = 0;
    for (; cur < string.size; cur++) {
        const char32 &c = string[cur];
        if (!TextFilterDigits(c)) return false;
    }
    return true;
}

void TextBox::CursorFromPosition(vec2 position) {
    vec2 cursorPos = 0.0;
    f32 spaceScale, spaceWidth;
    spaceWidth = globals->assets.CharacterWidth(' ', fontIndex) * fontSize;
    const char32 *lineString = &stringFormatted[0];
    i32 formatNewlines = 0;
    cursor = 0;
    cursorPos.y += fontSize * Rendering::lineHeight + positionAbsolute.y + padding.y;
    if (cursorPos.y <= position.y / globals->gui.scale) {
        for (; cursor < stringFormatted.size; cursor++) {
            const char32 &c = stringFormatted[cursor];
            if (c == '\n') {
                if (string[cursor-formatNewlines] != '\n' && string[cursor-formatNewlines] != ' ') {
                    formatNewlines++;
                }
                lineString = &c+1;
                cursorPos.y += fontSize * Rendering::lineHeight;
                if (cursorPos.y > position.y / globals->gui.scale) {
                    cursor++;
                    break;
                }
            }
        }
    }
    globals->rendering.LineCursorStartAndSpaceScale(cursorPos.x, spaceScale, fontSize, spaceWidth, fontIndex, lineString, sizeAbsolute.x - padding.x * 2.0, alignH);
    cursorPos.x += positionAbsolute.x + padding.x;
    if (alignH == Rendering::CENTER) {
        cursorPos.x += sizeAbsolute.x * 0.5 - padding.x;
    } else if (alignH == Rendering::RIGHT) {
        cursorPos.x += sizeAbsolute.x - padding.x * 2.0;
    }
    cursorPos *= globals->gui.scale;
    spaceWidth *= spaceScale * globals->gui.scale;
    f32 advanceCarry;
    for (; cursor < stringFormatted.size; cursor++) {
        const char32 &c = stringFormatted[cursor];
        if (c == '\n') {
            break;
        }
        if (c == ' ') {
            advanceCarry = spaceWidth * 0.5;
        } else {
            advanceCarry = globals->assets.CharacterWidth(c, fontIndex) * fontSize * globals->gui.scale * 0.5;
        }
        cursorPos.x += advanceCarry;
        if (cursorPos.x > position.x) {
            break;
        }
        cursorPos.x += advanceCarry;
    }
    cursor -= formatNewlines;
}

vec2 TextBox::PositionFromCursor() const {
    vec2 cursorPos = 0.0;
    f32 spaceScale, spaceWidth;
    spaceWidth = globals->assets.CharacterWidth(' ', fontIndex) * fontSize;
    const char32 *lineString = &stringFormatted[0];
    i32 lineStart = 0;
    i32 formatNewlines = 0;
    for (i32 i = 0; i < cursor+formatNewlines; i++) {
        const char32 &c = stringFormatted[i];
        if (c == '\n') {
            if (string[i-formatNewlines] != '\n' && string[i-formatNewlines] != ' ') {
                formatNewlines++;
            }
            cursorPos.y += fontSize * Rendering::lineHeight;
            lineString = &c+1;
            lineStart = i+1;
        }
    }
    globals->rendering.LineCursorStartAndSpaceScale(cursorPos.x, spaceScale, fontSize, spaceWidth, fontIndex, lineString, sizeAbsolute.x - padding.x * 2.0, alignH);
    spaceWidth *= spaceScale;
    for (i32 i = lineStart; i < cursor+formatNewlines; i++) {
        const char32 &c = stringFormatted[i];
        if (c == '\n') {
            break;
        }
        if (c == ' ') {
            cursorPos.x += spaceWidth;
        } else {
            cursorPos.x += globals->assets.CharacterWidth(c, fontIndex) * fontSize;
        }
    }
    if (alignH == Rendering::CENTER) {
        cursorPos.x += sizeAbsolute.x * 0.5 - padding.x;
    } else if (alignH == Rendering::RIGHT) {
        cursorPos.x += sizeAbsolute.x - padding.x * 2.0;
    }
    cursorPos += positionAbsolute + padding;
    cursorPos *= globals->gui.scale;
    return cursorPos;
}

void TextBox::UpdateSize(vec2 container) {
    if (size.x > 0.0) {
        sizeAbsolute.x = fractionWidth ? (container.x * size.x - margin.x * 2.0) : size.x;
    } else {
        sizeAbsolute.x = globals->rendering.StringWidth(stringFormatted, fontIndex) * fontSize + padding.x * 2.0;
    }
    if (size.y > 0.0) {
        sizeAbsolute.y = fractionHeight ? (container.y * size.y - margin.y * 2.0) : size.y;
    } else {
        sizeAbsolute.y = Rendering::StringHeight(stringFormatted) * fontSize + padding.y * 2.0;
    }
    LimitSize();
}

void TextBox::Update(vec2 pos, bool selected) {
    if (entry) {
        cursorBlinkTimer += globals->objects.timestep;
        if (cursorBlinkTimer > 1.0) {
            cursorBlinkTimer -= 1.0;
        }
        highlighted = true;
        if (globals->input.AnyKey.Pressed()) {
            for (i32 i = 0; i < globals->input.typingString.size; i++) {
                const char32 c = globals->input.typingString[i];
                if (textFilter(c)) {
                    string.Insert(cursor, c);
                    cursorBlinkTimer = 0.0;
                    cursor++;
                }
            }
        }
        globals->input.typingString.Clear();
        if (globals->input.Pressed(KC_KEY_BACKSPACE)) {
            if (cursor <= string.size && cursor > 0) {
                string.Erase(cursor-1);
                cursorBlinkTimer = 0.0;
                cursor--;
            }
        }
        if (globals->input.Pressed(KC_KEY_DELETE)) {
            if (cursor < string.size) {
                string.Erase(cursor);
                cursorBlinkTimer = 0.0;
            }
        }
        if (globals->input.Pressed(KC_KEY_HOME)) {
            if (globals->input.Down(KC_KEY_LEFTCTRL) || globals->input.Down(KC_KEY_RIGHTCTRL) || !multiline) {
                cursor = 0;
            } else {
                for (cursor--; cursor >= 0; cursor--) {
                    if (string[cursor] == '\n') break;
                }
                cursor++;
            }
            cursorBlinkTimer = 0.0;
        }
        if (globals->input.Pressed(KC_KEY_END)) {
            if (globals->input.Down(KC_KEY_LEFTCTRL) || globals->input.Down(KC_KEY_RIGHTCTRL) || !multiline) {
                cursor = string.size;
            } else {
                for (; cursor < string.size; cursor++) {
                    if (string[cursor] == '\n') break;
                }
            }
            cursorBlinkTimer = 0.0;
        }
        if (multiline) {
            if (globals->input.Pressed(KC_KEY_ENTER)) {
                string.Insert(cursor, '\n');
                cursor++;
                cursorBlinkTimer = 0.0;
            }
            if (globals->input.Pressed(KC_KEY_UP)) {
                vec2 cursorPos = PositionFromCursor();
                cursorPos.y -= fontSize * globals->gui.scale * Rendering::lineHeight * 0.5;
                CursorFromPosition(cursorPos);
                cursorBlinkTimer = 0.0;
            }
            if (globals->input.Pressed(KC_KEY_DOWN)) {
                vec2 cursorPos = PositionFromCursor();
                cursorPos.y += fontSize * globals->gui.scale * Rendering::lineHeight * 1.5;
                CursorFromPosition(cursorPos);
                cursorBlinkTimer = 0.0;
            }
        }
        if (globals->input.Pressed(KC_KEY_LEFT)) {
            cursorBlinkTimer = 0.0;
            if (globals->input.Down(KC_KEY_LEFTCTRL) || globals->input.Down(KC_KEY_RIGHTCTRL)) {
                if (IsWhitespace(string[--cursor])) {
                    for (; cursor > 0; cursor--) {
                        const char32 c = string[cursor];
                        if (!IsWhitespace(c)) {
                            cursor++;
                            break;
                        }
                    }
                } else {
                    for (; cursor > 0; cursor--) {
                        const char32 c = string[cursor];
                        if (IsWhitespace(c)) {
                            cursor++;
                            break;
                        }
                    }
                }
                cursor = max(0, cursor);
            } else {
                cursor = max(0, cursor-1);
            }
        }
        if (globals->input.Pressed(KC_KEY_RIGHT)) {
            cursorBlinkTimer = 0.0;
            if (globals->input.Down(KC_KEY_LEFTCTRL) || globals->input.Down(KC_KEY_RIGHTCTRL)) {
                if (IsWhitespace(string[cursor])) {
                    for (cursor++; cursor < string.size; cursor++) {
                        const char32 c = string[cursor];
                        if (!IsWhitespace(c)) {
                            break;
                        }
                    }
                } else {
                    for (cursor++; cursor < string.size; cursor++) {
                        const char32 c = string[cursor];
                        if (IsWhitespace(c)) {
                            break;
                        }
                    }
                }
                cursor = min(string.size, cursor);
            } else {
                cursor = min(string.size, cursor+1);
            }
        }
        if (!multiline && globals->input.Pressed(KC_KEY_ENTER)) {
            entry = false;
        }
    }
    if (size.x != 0.0 && multiline) {
        stringFormatted = globals->rendering.StringAddNewlines(string, fontIndex, (sizeAbsolute.x - padding.x * 2.0) / fontSize);
    } else {
        stringFormatted = string;
    }
    Widget::Update(pos, selected);
    bool mouseover = MouseOver();
    if (globals->gui.controlDepth != depth) {
        highlighted = false;
    }
    if (mouseover) {
        highlighted = true;
    }
    if (globals->objects.Pressed(KC_MOUSE_LEFT)) {
        if (mouseover) {
            if (globals->gui.controlDepth == depth) {
                globals->gui.controlDepth = depth+1;
            }
            const vec2 mouse = vec2(globals->input.cursor);
            CursorFromPosition(mouse);
            cursorBlinkTimer = 0.0;
        }
        if (!mouseover && entry && globals->gui.controlDepth == depth+1) {
            globals->gui.controlDepth = depth;
            entry = false;
        } else {
            entry = mouseover;
        }
    }
    if (globals->gui.controlDepth == depth) {
        if (selected) {
            if (globals->objects.Released(KC_GP_BTN_A)) {
                entry = true;
                globals->gui.controlDepth++;
            }
        }
    } else if (globals->gui.controlDepth == depth+1) {
        if (selected) {
            if (globals->objects.Released(KC_GP_BTN_B)) {
                entry = false;
                globals->gui.controlDepth--;
            }
        }
    }
}

void TextBox::Draw(Rendering::DrawingContext &context) const {
    vec4 bg, text;
    if (!textValidate(string)) {
        // These names are confusing...
        bg = errorBG;
        text = errorText;
    } else {
        bg = highlighted ? highlightBG : colorBG;
        text = highlighted ? highlightText : colorText;
    }
    PushScissor(context);
    vec2 drawPosText = (positionAbsolute + padding) * globals->gui.scale;
    vec2 scale = vec2(fontSize * globals->gui.scale);
    vec2 textArea = (sizeAbsolute - padding * 2.0) * globals->gui.scale;
    if (alignH == Rendering::CENTER) {
        drawPosText.x += textArea.x * 0.5;
    } else if (alignH == Rendering::RIGHT) {
        drawPosText.x += textArea.x;
    }
    vec2 drawPos = positionAbsolute * globals->gui.scale;
    globals->rendering.DrawQuad(context, Rendering::texBlank, bg, drawPos, vec2(1.0), sizeAbsolute * globals->gui.scale);
    globals->rendering.DrawText(context, stringFormatted, fontIndex, text, drawPosText, scale, alignH, Rendering::TOP, textArea.x);
    if (cursorBlinkTimer < 0.5 && entry) {
        vec2 cursorPos = PositionFromCursor();
        cursorPos.y -= fontSize * globals->gui.scale * 0.1;
        globals->rendering.DrawQuad(context, Rendering::texBlank, text, cursorPos, vec2(1.0), vec2(1.0, fontSize * globals->gui.scale * Rendering::lineHeight));
    }
    PopScissor(context);
}

Slider::Slider() :
value(1.0),                     valueMin(0.0),
valueMax(1.0),                  mirror(nullptr),
colorBG(vec3(0.15), 0.9),       colorSlider(colorHighlightMedium, 1.0),
highlightBG(vec3(0.2), 0.9),    highlightSlider(colorHighlightHigh, 1.0),
grabbed(false), left(), right()
{
    occludes = true;
    selectable = true;
    left.canRepeat = true;
    right.canRepeat = true;
}

void Slider::Update(vec2 pos, bool selected) {
    Widget::Update(pos, selected);
    mouseover = MouseOver();
    left.Tick(globals->objects.timestep);
    right.Tick(globals->objects.timestep);
    if (selected) {
        bool lmbDown = globals->objects.Down(KC_MOUSE_LEFT);
        if (globals->objects.Pressed(KC_GP_AXIS_LS_LEFT)) {
            left.Press();
        } else if (left.Down() && !lmbDown) {
            left.Release();
        }
        if (globals->objects.Pressed(KC_GP_AXIS_LS_RIGHT)) {
            right.Press();
        } else if (right.Down() && !lmbDown) {
            right.Release();
        }
    }
    if (mouseover && !grabbed) {
        i32 mousePos = 0;
        f32 mouseX = (f32)globals->input.cursor.x / globals->gui.scale - positionAbsolute.x;
        f32 sliderX = map(value, valueMin, valueMax, 0.0, sizeAbsolute.x - 16.0);
        if (mouseX < sliderX) {
            mousePos = -1;
        } else if (mouseX > sliderX+16.0) {
            mousePos = 1;
        }
        if (globals->objects.Pressed(KC_MOUSE_LEFT)) {
            if (mousePos == 0) {
                grabbed = true;
            } else if (mousePos == 1) {
                right.Press();
            } else {
                left.Press();
            }
        }
    }
    bool updated = false;
    f32 scale = (valueMax - valueMin) / (sizeAbsolute.x - 16.0);
    if (grabbed) {
        f32 moved = f32(globals->input.cursor.x - globals->input.cursorPrevious.x) / globals->gui.scale * scale;
        if (globals->objects.Down(KC_KEY_LEFTSHIFT)) {
            moved /= 10.0;
        }
        if (moved != 0.0) updated = true;
        value = clamp(value + moved, valueMin, valueMax);
    }
    if (!globals->objects.Down(KC_KEY_LEFTSHIFT)) {
        scale *= 10.0;
    }
    if (right.Pressed()) {
        value = clamp(value + scale, valueMin, valueMax);
        updated = true;
    }
    if (left.Pressed()) {
        value = clamp(value - scale, valueMin, valueMax);
        updated = true;
    }
    if (globals->objects.Released(KC_MOUSE_LEFT)) {
        grabbed = false;
        if (right.Down()) {
            right.Release();
        }
        if (left.Down()) {
            left.Release();
        }
    }
    if (mirror != nullptr) {
        if (updated) {
            mirror->string = ToWString(ToString(value, 10, 1));
            i32 dot = -1;
            for (i32 i = 0; i < mirror->string.size; i++) {
                char32 &c = mirror->string[i];
                if (c == '.') {
                    dot = i;
                    break;
                }
            }
            if (dot != -1) {
                mirror->string.Resize(dot+2);
            }
        } else if (mirror->entry) {
            if (mirror->textValidate(mirror->string)) {
                value = clamp(WStringToF32(mirror->string), valueMin, valueMax);
            }
        }
    }
}

void Slider::Draw(Rendering::DrawingContext &context) const {
    vec4 bg = highlighted ? highlightBG : colorBG;
    vec4 slider = highlighted ? highlightSlider : colorSlider;
    vec2 drawPos = positionAbsolute * globals->gui.scale;
    globals->rendering.DrawQuad(context, Rendering::texBlank, bg, drawPos, vec2(1.0), sizeAbsolute * globals->gui.scale);
    drawPos.x += map(value, valueMin, valueMax, 2.0, sizeAbsolute.x - 16.0) * globals->gui.scale;
    drawPos.y += 2.0 * globals->gui.scale;
    globals->rendering.DrawQuad(context, Rendering::texBlank, slider, drawPos, vec2(1.0), vec2(12.0, sizeAbsolute.y - 4.0) * globals->gui.scale);
}

Hideable::Hideable(Widget *child) : hidden(false), hiddenPrev(false) {
    margin = 0.0;
    AddWidget(this, child);
    size = child->size; // We need to inherit this for Lists to work properly
    fractionWidth = child->fractionWidth;
    fractionHeight = child->fractionHeight;
    occludes = child->occludes;
}

void Hideable::UpdateSize(vec2 container) {
    if (hidden) {
        sizeAbsolute = 0.0;
    } else {
        children[0]->UpdateSize(container);
        sizeAbsolute = children[0]->GetSize();
    }
}

void Hideable::Update(vec2 pos, bool selected) {
    if (!hidden) {
        children[0]->Update(pos + position, selected);
        positionAbsolute = children[0]->positionAbsolute;
    }
    if (hidden && !hiddenPrev) {
        children[0]->OnHide();
    }
    hiddenPrev = hidden;
}

void Hideable::Draw(Rendering::DrawingContext &context) const {
    if (!hidden) {
       children[0]->Draw(context);
    }
}

bool Hideable::Selectable() const {
    return selectable && !hidden;
}

} // namespace Int

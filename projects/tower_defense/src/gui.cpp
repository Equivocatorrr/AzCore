/*
	File: gui.cpp
	Author: Philip Haynes
*/

#include "gui.hpp"
#include "entities.hpp"

#include "Az2D/game_systems.hpp"
#include "Az2D/settings.hpp"
#include "AzCore/Profiling.hpp"

namespace Az2D::Gui {

using Entities::entities;
using GameSystems::sys;

using namespace AzCore;

Gui *gui = nullptr;

const vec3 colorBack = {1.0f, 0.4f, 0.1f};
const vec3 colorHighlightLow = {0.2f, 0.45f, 0.5f};
const vec3 colorHighlightMedium = {0.4f, 0.9f, 1.0f};
const vec3 colorHighlightHigh = {0.9f, 0.98f, 1.0f};

const f32 backgroundOpacity = 0.7f;
const f32 buttonBaseOpacity = 0.4f;

Gui::Gui() {
	gui = this;
}

void Gui::EventAssetsRequest() {
	GuiBasic::EventAssetsRequest();
	texCursor = sys->assets.RequestTexture("Cursor.png");
}

void Gui::EventInitialize() {
	GuiBasic::EventInitialize();
	system.defaults.buttonText.fontSize = 28.0f;
	system.defaults.buttonText.color = vec4(vec3(1.0f), 1.0f);
	system.defaults.buttonText.colorHighlighted = vec4(vec3(0.0f), 1.0f);
	system.defaults.buttonText.SetHeightFraction(1.0f);
	system.defaults.buttonText.padding = 0.0f;
	system.defaults.buttonText.margin = 0.0f;
	system.defaults.buttonText.data = TextMetadata{Rendering::CENTER, Rendering::CENTER};
	menuMain.Initialize();
	menuSettings.Initialize();
	menuPlay.Initialize();
}

void Gui::EventSync() {
	AZCORE_PROFILING_SCOPED_TIMER(Az2D::Gui::Gui::EventSync)
	GuiBasic::EventSync();
	static bool wasPaused;
	static bool once = true;
	if (console) {
		if (once) {
			wasPaused = sys->paused;
			once = false;
		}
		sys->paused = true;
	} else {
		if (!once) {
			sys->paused = wasPaused;
			once = true;
		}
		currentMenu = nextMenu;
		switch (currentMenu) {
		case Menu::MAIN:
			menuMain.Update();
			break;
		case Menu::SETTINGS:
			menuSettings.Update();
			break;
		case Menu::PLAY:
			menuPlay.Update();
			break;
		}
	}
}

void Gui::EventDraw(Array<Rendering::DrawingContext> &contexts) {
	AZCORE_PROFILING_SCOPED_TIMER(Az2D::Gui::Gui::EventDraw)
	switch (currentMenu) {
	case Menu::MAIN:
		menuMain.Draw(contexts.Back());
		break;
	case Menu::SETTINGS:
		menuSettings.Draw(contexts.Back());
		break;
	case Menu::PLAY:
		menuPlay.Draw(contexts.Back());
		break;
	}
	GuiBasic::EventDraw(contexts);
	if (system.inputMethod == GuiGeneric::InputMethod::MOUSE) {
		sys->rendering.DrawQuad(contexts.Back(), sys->input.cursor, vec2(32.0f * system.scale), vec2(1.0f), vec2(0.5f), 0.0f, Rendering::PIPELINE_BASIC_2D, vec4(1.0f), texCursor);
	}
}

void MainMenu::Initialize() {
	screen = gui->system.CreateScreen();
	azgui::ListV *listV = gui->system.CreateListV(screen);
	listV->SetWidthContents();
	listV->SetHeightFraction(1.0f);
	listV->padding = vec2(40.0f);
	listV->color = 0.0f;
	listV->colorHighlighted = 0.0f;
	
	constexpr f32 slant = 32.0f;
	
	azgui::Text *title = gui->system.CreateText(listV);
	title->position.x = 4.0f * slant;
	title->data = TextMetadata();
	title->data.Get<TextMetadata>().alignH = Rendering::CENTER;
	title->bold = true;
	title->color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	title->colorOutline = vec4(1.0f);
	title->outline = true;
	title->fontSize = 48.0f;
	title->string = sys->ReadLocale("AzCore Tower Defense");
	title->SetWidthPixel(256.0f);
	title->SetHeightContents();
	
	azgui::Spacer *spacer = gui->system.CreateSpacer(listV, 1.0f);
	
	azgui::Button buttonTemplate;
	buttonTemplate.SetSizePixel(vec2(256.0f, 64.0f));
	buttonTemplate.margin = vec2(16.0f);
	buttonTemplate.padding = vec2(16.0f);
	buttonTemplate.color = vec4(vec3(0.0f), buttonBaseOpacity);

	buttonContinue = gui->system.CreateButtonFrom(nullptr, buttonTemplate);
	buttonContinue->position.x = 3.0f * slant;
	buttonContinue->AddDefaultText(sys->ReadLocale("Continue"));
	buttonContinue->keycodeActivators = {KC_KEY_ESC};

	continueHideable = gui->system.CreateHideable(listV, buttonContinue);
	continueHideable->hidden = true;

	buttonNewGame = gui->system.CreateButtonFrom(listV, buttonTemplate);
	buttonNewGame->position.x = 2.0f * slant;
	buttonNewGame->AddDefaultText(sys->ReadLocale("New Game"));

	buttonSettings = gui->system.CreateButtonFrom(listV, buttonTemplate);
	buttonSettings->position.x = slant;
	buttonSettings->AddDefaultText(sys->ReadLocale("Settings"));

	buttonExit = gui->system.CreateButtonFrom(listV, buttonTemplate);
	buttonExit->AddDefaultText(sys->ReadLocale("Exit"));
	buttonExit->colorHighlighted = vec4(colorBack, 0.9f);
}

void MainMenu::Update() {
	screen->Update(vec2(0.0f), true);
	if (buttonContinue->state.Released()) {
		gui->nextMenu = Gui::Menu::PLAY;
	}
	if (buttonNewGame->state.Released()) {
		gui->nextMenu = Gui::Menu::PLAY;
		continueHideable->hidden = false;
	}
	if (buttonSettings->state.Released()) {
		gui->nextMenu = Gui::Menu::SETTINGS;
		gui->menuSettings.Reset();
	}
	if (buttonExit->state.Released()) {
		sys->exit = true;
	}
}

void MainMenu::Draw(Rendering::DrawingContext &context) {
	Any anyContext = &context;
	screen->Draw(anyContext);
}

void SettingsMenu::Reset() {
	checkFullscreen->checked = Settings::ReadBool(Settings::sFullscreen);
	checkVSync->checked = Settings::ReadBool(Settings::sVSync);
	framerateHideable->hidden = Settings::ReadBool(Settings::sVSync);
	textboxFramerate->string = ToWString(ToString((i32)Settings::ReadReal(Settings::sFramerate)));
	f32 volumeMain = (f32)az::ampToDecibels(Settings::ReadReal(Settings::sVolumeMain));
	f32 volumeMusic = (f32)az::ampToDecibels(Settings::ReadReal(Settings::sVolumeMusic));
	f32 volumeEffects = (f32)az::ampToDecibels(Settings::ReadReal(Settings::sVolumeEffects));
	sliderVolumes[0]->SetValue(volumeMain);
	sliderVolumes[1]->SetValue(volumeMusic);
	sliderVolumes[2]->SetValue(volumeEffects);
	for (i32 i = 0; i < 3; i++) {
		sliderVolumes[i]->UpdateMirror();
	}
	f32 guiScale = Settings::ReadReal(Settings::sGuiScale)*100.0f;
	sliderGuiScale->SetValue(guiScale);
	sliderGuiScale->UpdateMirror();
}

void SettingsMenu::Initialize() {
	screen = gui->system.CreateScreen();
	azgui::ListV *listV = gui->system.CreateListV(screen);
	listV->SetWidthPixel(500.0f);
	listV->SetHeightFraction(1.0f);
	listV->padding = vec2(40.0f);
	listV->color = vec4(0.0f);
	listV->colorHighlighted = vec4(0.0f);

	azgui::Text *title = gui->system.CreateText(listV);
	title->data = TextMetadata{Rendering::CENTER, Rendering::TOP};
	title->bold = true;
	title->color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	title->colorOutline = vec4(1.0f);
	title->outline = true;
	title->fontSize = 64.0f;
	title->string = sys->ReadLocale("Settings");
	title->SetWidthFraction(1.0f);
	title->SetHeightContents();
	
	azgui::Spacer *spacer = gui->system.CreateSpacer(listV, 1.0f);

	azgui::Text settingTextTemplate;
	settingTextTemplate.fontSize = 20.0f;
	settingTextTemplate.SetHeightFraction(1.0f);
	settingTextTemplate.data = TextMetadata{Rendering::LEFT, Rendering::CENTER};

	checkFullscreen = gui->system.CreateCheckbox(nullptr);
	checkFullscreen->checked = Settings::ReadBool(Settings::sFullscreen);
	
	checkVSync = gui->system.CreateCheckbox(nullptr);
	checkVSync->checked = Settings::ReadBool(Settings::sVSync);

	azgui::Textbox textboxTemplate;
	textboxTemplate.SetWidthPixel(64.0f);
	textboxTemplate.SetHeightFraction(1.0f);
	textboxTemplate.data = TextMetadata{Rendering::RIGHT, Rendering::CENTER};
	textboxTemplate.fontSize = 20.0f;
	textboxTemplate.textFilter = azgui::TextFilterDigits;
	textboxTemplate.textValidate = azgui::TextValidateNonempty;
	textboxTemplate.stringSuffix = ToWString("%");
	textboxTemplate.colorBG = vec4(vec3(0.0f), buttonBaseOpacity);

	azgui::Slider sliderTemplate;
	sliderTemplate.SetWidthPixel(116.0f);
	sliderTemplate.SetHeightFraction(1.0f);
	sliderTemplate.valueMin = -60.0f;
	sliderTemplate.valueMax = 0.0f;
	sliderTemplate.valueStep = 1.0f;
	sliderTemplate.valueTick = 3.0f;
	sliderTemplate.valueTickShiftMult = 1.0f / 3.0f;
	sliderTemplate.minOverride = true;
	sliderTemplate.minOverrideValue = -INFINITY;
	sliderTemplate.maxOverride = true;
	sliderTemplate.maxOverrideValue = -0.0f;
	sliderTemplate.mirrorPrecision = 0;
	sliderTemplate.colorBG = vec4(vec3(0.0f), buttonBaseOpacity);

	textboxFramerate = gui->system.CreateTextboxFrom(nullptr, textboxTemplate);
	textboxFramerate->stringSuffix = ToWString("fps");

	for (i32 i = 0; i < 3; i++) {
		textboxVolumes[i] = gui->system.CreateTextboxFrom(nullptr, textboxTemplate);
		sliderVolumes[i] = gui->system.CreateSliderFrom(nullptr, sliderTemplate);
		textboxVolumes[i]->stringSuffix = ToWString("dB");
		textboxVolumes[i]->textFilter = azgui::TextFilterBasic;
		textboxVolumes[i]->textValidate = azgui::TextValidateDecimalsNegativeAndInfinity;
		sliderVolumes[i]->mirror = textboxVolumes[i];
	}
	
	f32 guiScale = Settings::ReadReal(Settings::sGuiScale);
	textboxGuiScale = gui->system.CreateTextboxFrom(nullptr, textboxTemplate);
	textboxGuiScale->textFilter = azgui::TextFilterDigits;
	textboxGuiScale->textValidate = azgui::TextValidateNonempty;
	sliderGuiScale = gui->system.CreateSliderFrom(nullptr, sliderTemplate);
	sliderGuiScale->minOverride = false;
	sliderGuiScale->maxOverride = false;
	sliderGuiScale->value = guiScale*100.0f;
	sliderGuiScale->valueMax = 300.0f;
	sliderGuiScale->valueMin = 50.0f;
	sliderGuiScale->valueTick = 25.0f;
	sliderGuiScale->valueTickShiftMult = 0.2f;
	sliderGuiScale->mirror = textboxGuiScale;

	azgui::ListH settingListTemplate;
	settingListTemplate.SetHeightContents();
	settingListTemplate.margin = vec2(8.0f);
	settingListTemplate.padding = vec2(0.0f);
	settingListTemplate.color = vec4(vec3(0.0f), backgroundOpacity);

	StaticArray<azgui::Widget*, 16> settingListItems = {
		checkFullscreen, nullptr,
		textboxFramerate, nullptr,
		checkVSync, nullptr,
		sliderGuiScale, textboxGuiScale,
		nullptr, nullptr,
		sliderVolumes[0], textboxVolumes[0],
		sliderVolumes[1], textboxVolumes[1],
		sliderVolumes[2], textboxVolumes[2],
	};
	const char *settingListNames[] = {
		"Fullscreen",
		"Framerate",
		"VSync",
		"GUI Scale",
		"Volume",
		"Main",
		"Music",
		"Effects"
	};

	for (i32 i = 0; i < settingListItems.size; i+=2) {
		if (settingListItems[i] == nullptr) {
			azgui::Text *settingText = gui->system.CreateTextFrom(listV, settingTextTemplate);
			settingText->string = sys->ReadLocale(settingListNames[i / 2]);
			settingText->data = TextMetadata{Rendering::CENTER, Rendering::CENTER};
			settingText->fontSize = 24.0f;
			settingText->SetHeightContents();
		} else {
			azgui::ListH *settingList = gui->system.CreateListHFrom(nullptr, settingListTemplate);
			if (i == 2) {
				// Hideable Framerate
				framerateHideable = gui->system.CreateHideable(listV, settingList);
				framerateHideable->hidden = Settings::ReadBool(Settings::sVSync);
			} else {
				gui->system.AddWidget(listV, settingList);
			}
			azgui::Text *settingText = gui->system.CreateTextFrom(settingList, settingTextTemplate);
			settingText->string = sys->ReadLocale(settingListNames[i / 2]);
			gui->system.AddWidgetAsDefault(settingList, settingListItems[i]);
			if (settingListItems[i+1] != nullptr) {
				// So we can control the slider with the keyboard and gamepad
				settingListItems[i+1]->selectable = false;
				gui->system.AddWidget(settingList, settingListItems[i+1]);
			}
		}
	}

	azgui::ListH *buttonList = gui->system.CreateListH(listV);
	buttonList->SetHeightContents();
	buttonList->margin = vec2(0.0f);
	buttonList->padding = vec2(0.0f);
	buttonList->color = vec4(0.0f);
	buttonList->colorHighlighted = vec4(0.0f);
	
	azgui::Button buttonTemplate;
	buttonTemplate.SetWidthFraction(1.0f / 2.0f);
	buttonTemplate.SetHeightPixel(64.0f);
	buttonTemplate.margin = vec2(8.0f);
	buttonTemplate.color = vec4(vec3(0.0f), buttonBaseOpacity);

	buttonBack = gui->system.CreateButtonFrom(buttonList, buttonTemplate);
	buttonBack->AddDefaultText(sys->ReadLocale("Back"));
	buttonBack->colorHighlighted = vec4(colorBack, 0.9f);
	buttonBack->keycodeActivators = {KC_GP_BTN_B, KC_KEY_ESC};

	buttonApply = gui->system.CreateButtonAsDefaultFrom(buttonList, buttonTemplate);
	buttonApply->AddDefaultText(sys->ReadLocale("Apply"));

	Reset();
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
	framerateHideable->hidden = checkVSync->checked;
	screen->Update(vec2(0.0f), true);
	if (buttonApply->state.Released()) {
		sys->window.Fullscreen(checkFullscreen->checked);
		Settings::SetBool(Settings::sFullscreen, checkFullscreen->checked);
		Settings::SetBool(Settings::sVSync, checkVSync->checked);
		u64 framerate = 60;
		if (textboxFramerate->textValidate(textboxFramerate->string)) {
			framerate = clamp(WStringToU64(textboxFramerate->string), (u64)30, (u64)300);
			sys->SetFramerate((f32)framerate);
		}
		Settings::SetReal(Settings::sFramerate, (f64)framerate);
		textboxFramerate->string = ToWString(ToString(framerate));
		Settings::SetReal(Settings::sGuiScale, f64(sliderGuiScale->value / 100.0f));
		Settings::SetReal(Settings::sVolumeMain, f64(az::decibelsToAmp(sliderVolumes[0]->GetActualValue())));
		Settings::SetReal(Settings::sVolumeMusic, f64(az::decibelsToAmp(sliderVolumes[1]->GetActualValue())));
		Settings::SetReal(Settings::sVolumeEffects, f64(az::decibelsToAmp(sliderVolumes[2]->GetActualValue())));
	}
	if (buttonBack->state.Released()) {
		gui->nextMenu = Gui::Menu::MAIN;
	}
}

void SettingsMenu::Draw(Rendering::DrawingContext &context) {
	Any anyContext = &context;
	screen->Draw(anyContext);
}

void UpgradesMenu::Initialize() {
	screen = gui->system.CreateScreen();
	azgui::ListH *list = gui->system.CreateListH(nullptr);
	hideable = gui->system.CreateHideable(screen, list);
	list->SetSizeContents();
	list->color = vec4(vec3(0.0f), backgroundOpacity);
	list->colorHighlighted = list->color;
	list->padding *= 0.5f;

	azgui::ListV *listStats = gui->system.CreateListVAsDefault(list);
	listStats->SetWidthPixel(250.0f);
	listStats->SetHeightContents();
	listStats->margin = 0.0f;
	listStats->padding = 0.0f;
	listStats->color = 0.0f;
	listStats->colorHighlighted = 0.0f;

	towerName = gui->system.CreateText(listStats);
	towerName->data = TextMetadata{Rendering::CENTER, Rendering::CENTER};
	towerName->bold = true;
	towerName->fontSize = 24.0f;
	towerName->SetWidthFraction(1.0f);
	towerName->SetHeightContents();
	towerName->string = sys->ReadLocale("Info");

	azgui::ListH *selectedTowerPriorityList = gui->system.CreateListH(nullptr);
	towerPriorityHideable = gui->system.CreateHideableAsDefault(listStats, selectedTowerPriorityList);
	selectedTowerPriorityList->SetWidthFraction(1.0f);
	selectedTowerPriorityList->SetHeightContents();
	selectedTowerPriorityList->padding = vec2(0.0f);
	selectedTowerPriorityList->margin = vec2(0.0f);
	selectedTowerPriorityList->color = 0.0f;
	selectedTowerPriorityList->colorHighlighted = 0.0f;
	azgui::Text *selectedTowerPriorityText = gui->system.CreateText(selectedTowerPriorityList);
	selectedTowerPriorityText->color = 1.0f;
	selectedTowerPriorityText->SetSizeFraction(vec2(0.5f, 1.0f));
	selectedTowerPriorityText->data = TextMetadata{Rendering::LEFT, Rendering::CENTER};
	selectedTowerPriorityText->fontSize = 18.0f;
	selectedTowerPriorityText->string = sys->ReadLocale("Priority");
	towerPriority = gui->system.CreateSwitchAsDefault(selectedTowerPriorityList);
	towerPriority->SetWidthFraction(0.5f);
	towerPriority->SetHeightContents();
	towerPriority->padding = 0.0f;
	towerPriority->color = vec4(vec3(0.0f), buttonBaseOpacity);
	for (i32 i = 0; i < 6; i++) {
		azgui::Text *priorityText = gui->system.CreateText(towerPriority);
		priorityText->selectable = true;
		priorityText->SetWidthFraction(1.0f);
		priorityText->SetHeightPixel(22.0f);
		priorityText->margin = 2.0f;
		priorityText->fontSize = 18.0f;
		priorityText->data = TextMetadata{Rendering::LEFT, Rendering::CENTER};
		priorityText->string = sys->ReadLocale(Entities::Tower::priorityStrings[i]);
	}
	
	selectedTowerStats = gui->system.CreateText(listStats);
	selectedTowerStats->SetWidthFraction(1.0f);
	selectedTowerStats->color = 1.0f;
	selectedTowerStats->fontSize = 18.0f;

	azgui::ListV *listUpgrades = gui->system.CreateListV(list);
	listUpgrades->fractionWidth = false;
	listUpgrades->fractionHeight = false;
	listUpgrades->SetWidthPixel(300.0f);
	listUpgrades->SetHeightContents();
	listUpgrades->margin = 0.0f;
	listUpgrades->padding = 0.0f;
	listUpgrades->color = 0.0f;
	listUpgrades->colorHighlighted = 0.0f;
	listUpgrades->selectionDefault = 1;

	azgui::Text *titleText = gui->system.CreateText(listUpgrades);
	titleText->data = TextMetadata{Rendering::CENTER, Rendering::CENTER};
	titleText->bold = true;
	titleText->fontSize = 24.0f;
	titleText->SetWidthFraction(1.0f);
	titleText->SetHeightContents();
	titleText->string = sys->ReadLocale("Upgrades");

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
		azgui::ListV *listV = gui->system.CreateListV(nullptr);
		upgradeHideable[i] = gui->system.CreateHideable(listUpgrades, listV);
		listV->SetWidthFraction(1.0f);
		listV->SetHeightContents();
		listV->margin *= 0.5f;
		listV->padding = 0.0f;
		listV->color = 0.0f;
		listV->colorHighlighted = 0.0f;

		azgui::ListH *listH = gui->system.CreateListHAsDefault(listV);
		listH->SetHeightContents();
		listH->margin = 0.0f;
		listH->padding = 0.0f;
		listH->color = 0.0f;
		listH->colorHighlighted = 0.0f;

		azgui::Text *upgradeName = gui->system.CreateText(listH);
		upgradeName->SetSizeFraction(vec2(0.35f, 1.0f));
		upgradeName->margin *= 0.5f;
		upgradeName->data = TextMetadata{Rendering::LEFT, Rendering::CENTER};
		upgradeName->fontSize = 18.0f;
		upgradeName->bold = true;
		upgradeName->string = sys->ReadLocale(upgradeNameStrings[i]);

		upgradeStatus[i] = gui->system.CreateText(listH);
		upgradeStatus[i]->SetWidthFraction(0.4f);
		upgradeStatus[i]->SetHeightContents();
		upgradeStatus[i]->margin *= 0.5f;
		upgradeStatus[i]->data = TextMetadata{Rendering::LEFT, Rendering::CENTER};
		upgradeStatus[i]->fontSize = 14.0f;
		upgradeStatus[i]->string = ToWString("0");

		upgradeButton[i] = gui->system.CreateButtonAsDefault(listH);
		upgradeButton[i]->SetSizeFraction(vec2(0.25f, 1.0f));
		upgradeButton[i]->margin *= 0.5f;
		upgradeButton[i]->color = vec4(vec3(0.0f), buttonBaseOpacity);
		azgui::Text *buttonText = upgradeButton[i]->AddDefaultText(sys->ReadLocale("Buy"));
		buttonText->fontSize = 18.0f;

		azgui::Text *upgradeDescription = gui->system.CreateText(listV);
		upgradeDescription->data = TextMetadata{Rendering::LEFT, Rendering::CENTER};
		upgradeDescription->SetWidthFraction(1.0f);
		upgradeDescription->margin = 0.0f;
		upgradeDescription->fontSize = 14.0f;
		upgradeDescription->string = sys->ReadLocale(upgradeDescriptionStrings[i]);
	}
}

inline String FloatToString(f32 in) {
	return ToString(in, 10, 2);
}

void UpgradesMenu::Update() {
	if (entities->selectedTower != -1) {
		Entities::Tower &tower = entities->towers.GetMutable(entities->selectedTower);
		hideable->hidden = false;
		vec2 towerScreenPos = entities->WorldPosToScreen(tower.physical.pos) / gui->system.scale;
		hideable->position = towerScreenPos - vec2(hideable->sizeAbsolute.x / 2.0f, 0.0f);
		// We need to use median because in the case of very large UI scaling min could be > max.
		hideable->position.x = median(hideable->position.x, gui->menuPlay.list->sizeAbsolute.x, screen->sizeAbsolute.x - hideable->sizeAbsolute.x);
		hideable->position.y = median(hideable->position.y, 0.0f, screen->sizeAbsolute.y - hideable->sizeAbsolute.y);

		towerPriorityHideable->hidden = !Entities::towerHasPriority[tower.type];
		const Entities::TowerUpgradeables &upgradeables = Entities::towerUpgradeables[tower.type];
		towerName->string = sys->ReadLocale(Entities::towerStrings[tower.type]);
		for (i32 i = 0; i < 5; i++) {
			upgradeHideable[i]->hidden = !upgradeables.data[i];
		}
		WString costString = "\n" + sys->ReadLocale("Cost:") + ' ';
		if (upgradeables.data[0]) { // Range
			i64 cost = tower.sunkCost / 2;
			f32 newRange = tower.range * 1.25f;
			bool canUpgrade = cost <= entities->money;
			upgradeStatus[0]->string =
				FloatToString(tower.range/10.0f) + "m > " + FloatToString(newRange/10.0f) + "m" +
				costString + ToString(cost);
			upgradeButton[0]->colorHighlighted = vec4(canUpgrade? colorHighlightMedium : vec3(0.8f, 0.1f, 0.1f), 1.0f);
			if (upgradeButton[0]->state.Released() && canUpgrade) {
				tower.range = newRange;
				tower.field.basis.circle.r = newRange;
				tower.sunkCost += cost;
				entities->money -= cost;
			}
		}
		if (upgradeables.data[1]) { // Firerate
			i64 cost = tower.sunkCost / 2;
			f32 newFirerate = tower.shootInterval / 1.5f;
			bool canUpgrade = cost <= entities->money && newFirerate >= 1.0f/18.1f;
			upgradeStatus[1]->string =
				FloatToString(1.0f/tower.shootInterval) + "r/s > " + FloatToString(1.0f/newFirerate) + "r/s" +
				costString + ToString(cost);
			upgradeButton[1]->colorHighlighted = vec4(canUpgrade? colorHighlightMedium : vec3(0.8f, 0.1f, 0.1f), 1.0f);
			if (upgradeButton[1]->state.Released() && canUpgrade) {
				tower.shootInterval = newFirerate;
				tower.sunkCost += cost;
				entities->money -= cost;
			}
		}
		if (upgradeables.data[2]) { // Accuracy
			i64 cost = tower.sunkCost / 5;
			Degrees32 newSpread = tower.bulletSpread.value() / 1.5f;
			bool canUpgrade = cost <= entities->money;
			upgradeStatus[2]->string =
				FloatToString(tower.bulletSpread.value()) + "° > " + FloatToString(newSpread.value()) + "°" +
				costString + ToString(cost);
			upgradeButton[2]->colorHighlighted = vec4(canUpgrade? colorHighlightMedium : vec3(0.8f, 0.1f, 0.1f), 1.0f);
			if (upgradeButton[2]->state.Released() && canUpgrade) {
				tower.bulletSpread = newSpread;
				tower.sunkCost += cost;
				entities->money -= cost;
			}
		}
		if (upgradeables.data[3]) { // Damage
			i64 cost = tower.sunkCost / 2;
			i32 newDamage = tower.damage * 3 / 2;
			bool canUpgrade = cost <= entities->money;
			upgradeStatus[3]->string =
				ToString(tower.damage) + " > " + ToString(newDamage) +
				costString + ToString(cost);
			upgradeButton[3]->colorHighlighted = vec4(canUpgrade? colorHighlightMedium : vec3(0.8f, 0.1f, 0.1f), 1.0f);
			if (upgradeButton[3]->state.Released() && canUpgrade) {
				tower.damage = newDamage;
				tower.bulletExplosionDamage *= 2;
				tower.sunkCost += cost;
				entities->money -= cost;
			}
		}
		if (upgradeables.data[4]) { // Multishot
			i64 cost = tower.sunkCost;
			i32 newBulletCount = tower.bulletCount * 2;
			if (tower.bulletCount >= 2) {
				newBulletCount = tower.bulletCount * 3 / 2;
				cost = cost * (newBulletCount - tower.bulletCount) / tower.bulletCount;
			}
			bool canUpgrade = cost <= entities->money && newBulletCount <= 60;
			upgradeStatus[4]->string =
				ToString(tower.bulletCount) + " > " + ToString(newBulletCount) +
				costString + ToString(cost);
			upgradeButton[4]->colorHighlighted = vec4(canUpgrade? colorHighlightMedium : vec3(0.8f, 0.1f, 0.1f), 1.0f);
			if (upgradeButton[4]->state.Released() && canUpgrade) {
				tower.bulletCount = newBulletCount;
				tower.sunkCost += cost;
				entities->money -= cost;
			}
		}
		selectedTowerStats->string =
			sys->ReadLocale("Kills") + ": " + ToString(tower.kills) + "\n"
			+ sys->ReadLocale("Damage") + ": " + ToString(tower.damageDone);
		if (towerPriority->changed) {
			tower.priority = (Entities::Tower::TargetPriority)(towerPriority->choice);
		}
	} else {
		hideable->hidden = true;
	}
	screen->Update(vec2(0.0f, 0.0f), !entities->focusMenu); // Hideable will handle selection culling
}

void UpgradesMenu::Draw(Rendering::DrawingContext &context) {
	Any anyContext = &context;
	screen->Draw(anyContext);
}

void PlayMenu::Initialize() {
	screen = gui->system.CreateScreen();
	
	list = gui->system.CreateListV(screen);
	list->SetWidthPixel(300.0f);
	list->SetHeightFraction(1.0f);
	list->margin = 0.0f;
	list->selectionDefault = 1;
	list->color = vec4(vec3(0.0f), backgroundOpacity);
	list->colorHighlighted = list->color;

	azgui::Text *towerHeader = gui->system.CreateText(list);
	towerHeader->data = TextMetadata{Rendering::CENTER, Rendering::TOP};
	towerHeader->string = sys->ReadLocale("Towers");

	azgui::ListH gridBase;
	gridBase.SetWidthFraction(1.0f);
	gridBase.SetHeightContents();
	gridBase.padding = vec2(0.0f);
	gridBase.margin = vec2(0.0f);
	gridBase.color = 0.0f;
	gridBase.colorHighlighted = 0.0f;
	gridBase.selectionDefault = 0;

	azgui::Button halfWidth;
	halfWidth.SetWidthFraction(0.5f);
	halfWidth.SetHeightPixel(32.0f);
	halfWidth.color = vec4(vec3(0.0f), buttonBaseOpacity);

	towerButtons.Resize(Entities::TOWER_MAX_RANGE + 1);
	for (i32 i = 0; i < towerButtons.size; i+=2) {
		azgui::ListH *grid = gui->system.CreateListHFrom(list, gridBase);
		for (i32 j = 0; j < 2; j++) {
			i32 index = i+j;
			if (index > towerButtons.size) break;
			towerButtons[index] = gui->system.CreateButtonFrom(grid, halfWidth);
			azgui::Text *buttonText = towerButtons[index]->AddDefaultText(sys->ReadLocale(Entities::towerStrings[index]));
			buttonText->fontSize = 20.0f;
			towerButtons[index]->colorHighlighted = Entities::Tower(Entities::TowerType(index)).color;
		}
		towerButtonLists.Append(grid);
	}

	towerInfo = gui->system.CreateText(list);
	towerInfo->SetHeightPixel(96.0f);
	towerInfo->SetWidthFraction(1.0f);
	towerInfo->color = vec4(1.0f);
	towerInfo->fontSize = 18.0f;
	towerInfo->string = ToWString("$MONEY");
	
	gui->system.CreateSpacer(list, 1.0f);

	azgui::Button fullWidth;
	fullWidth.SetWidthFraction(1.0f);
	fullWidth.SetHeightPixel(32.0f);
	fullWidth.color = vec4(vec3(0.0f), buttonBaseOpacity);

	azgui::ListH *waveList = gui->system.CreateListHFrom(list, gridBase);

	waveTitle = gui->system.CreateText(waveList);
	waveTitle->SetSizeFraction(vec2(0.5f, 1.0f));
	waveTitle->data = TextMetadata{Rendering::LEFT, Rendering::CENTER};
	waveTitle->colorOutline = vec4(1.0f, 0.0f, 0.5f, 1.0f);
	waveTitle->color = vec4(1.0f);
	waveTitle->outline = true;
	waveTitle->fontSize = 30.0f;
	// waveTitle->bold = true;
	waveTitle->margin.y = 0.0f;
	waveTitle->string = ToWString("Nothing");
	
	buttonStartWave = gui->system.CreateButtonAsDefaultFrom(waveList, halfWidth);
	buttonTextStartWave = buttonStartWave->AddDefaultText(sys->ReadLocale("Start Wave"));
	buttonTextStartWave->fontSize = 20.0f;
	buttonStartWave->size.y = 32.0f;
	buttonStartWave->keycodeActivators = {KC_GP_BTN_START, KC_KEY_SPACE};

	waveInfo = gui->system.CreateText(list);
	waveInfo->SetWidthFraction(1.0f);
	waveInfo->color = vec4(1.0f);
	waveInfo->fontSize = 20.0f;
	waveInfo->string = ToWString("Nothing");

	buttonMenu = gui->system.CreateButtonFrom(list, fullWidth);
	buttonMenu->AddDefaultText(sys->ReadLocale("Menu"));
	buttonMenu->keycodeActivators = {KC_GP_BTN_SELECT, KC_KEY_ESC};

	upgradesMenu.Initialize();
}

void PlayMenu::Update() {
	upgradesMenu.Update();
	WString towerInfoString = sys->ReadLocale("Money") + ": $" + ToString(entities->money);
	i32 textTower = -1;
	if (entities->placeMode) {
		textTower = entities->towerType;
	} else {
		for (i32 i = 0; i < towerButtons.size; i++) {
			if (towerButtons[i]->highlighted) {
				textTower = i;
			}
		}
	}
	{ // Make the grid work more nicely (hacky)
		i32 selection = -1;
		for (azgui::ListH *list : towerButtonLists) {
			if (list->selection >= 0) {
				selection = list->selection;
				break;
			}
		}
		if (selection != -1) {
			for (azgui::ListH *list : towerButtonLists) {
				list->selectionDefault = selection;
			}
		}
	}
	if (textTower != -1) {
		towerInfoString += "\n" + sys->ReadLocale("Cost") + ": $" + ToString(Entities::towerCosts[textTower]) + "\n" + sys->ReadLocale(Entities::towerDescriptions[textTower]);
	}
	towerInfo->string = towerInfoString;
	waveTitle->string = sys->ReadLocale("Wave") + ": " + ToString(entities->wave);
	waveInfo->string =
		sys->ReadLocale("Wave Hitpoints Left") + ": " + ToString(entities->hitpointsLeft) + "\n"
		+ sys->ReadLocale("Lives") + ": " + ToString(entities->lives);
	screen->Update(vec2(0.0f), entities->focusMenu);
	if (buttonMenu->state.Released()) {
		gui->nextMenu = Gui::Menu::MAIN;
		sys->paused = true;
		if (entities->waveActive) {
			buttonTextStartWave->string = sys->ReadLocale("Resume");
		}
	}
}

void PlayMenu::Draw(Rendering::DrawingContext &context) {
	upgradesMenu.Draw(context);
	Any anyContext = &context;
	screen->Draw(anyContext);
}

} // namespace Az2D::Gui

/*
	File: gui.cpp
	Author: Philip Haynes
*/

#include "gui.hpp"
#include "entities.hpp"

#include "Az2D/game_systems.hpp"
#include "Az2D/settings.hpp"

namespace Az2D::Gui {

using Entities::entities;
using GameSystems::sys;

Gui *gui = nullptr;

const vec3 colorBack = {1.0f, 0.4f, 0.1f};
const vec3 colorHighlightLow = {0.2f, 0.45f, 0.5f};
const vec3 colorHighlightMedium = {0.4f, 0.9f, 1.0f};
const vec3 colorHighlightHigh = {0.9f, 0.98f, 1.0f};

Gui::Gui() {
	gui = this;
}

void Gui::EventAssetsQueue() {
	GuiBasic::EventAssetsQueue();
	sys->assets.QueueFile("Cursor.png");
}

void Gui::EventAssetsAcquire() {
	GuiBasic::EventAssetsAcquire();
	texCursor = sys->assets.FindTexture("Cursor.png");
}

void Gui::EventInitialize() {
	menuMain.Initialize();
	menuSettings.Initialize();
	menuPlay.Initialize();
}

void Gui::EventSync() {
	GuiBasic::EventSync();
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

void Gui::EventDraw(Array<Rendering::DrawingContext> &contexts) {
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
	if (usingMouse) {
		sys->rendering.DrawQuad(contexts.Back(), texCursor, vec4(1.0f), sys->input.cursor, vec2(32.0f * scale), vec2(1.0f), vec2(0.5f));
	}
}

void MainMenu::Initialize() {
	ListV *listV = new ListV();
	listV->color = vec4(0.0f);
	listV->highlight = vec4(0.0f);

	Widget *spacer = new Widget();
	spacer->size.y = 0.3f;
	AddWidget(listV, spacer);

	Text *title = new Text();
	title->alignH = Rendering::CENTER;
	title->bold = true;
	title->color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	title->colorOutline = vec4(1.0f);
	title->outline = true;
	title->fontSize = 64.0f;
	title->fontIndex = guiBasic->fontIndex;
	title->string = sys->ReadLocale("AzCore Tower Defense");
	AddWidget(listV, title);

	spacer = new Widget();
	spacer->size.y = 0.4f;
	AddWidget(listV, spacer);

	ListV *buttonList = new ListV();
	buttonList->fractionWidth = false;
	buttonList->size = vec2(500.0f, 0.0f);
	buttonList->padding = vec2(16.0f);

	buttonContinue = new Button();
	buttonContinue->string = sys->ReadLocale("Continue");
	buttonContinue->size.y = 64.0f;
	buttonContinue->fractionHeight = false;
	buttonContinue->margin = vec2(16.0f);

	continueHideable = new Hideable(buttonContinue);
	continueHideable->hidden = true;
	AddWidget(buttonList, continueHideable);

	buttonNewGame = new Button();
	buttonNewGame->string = sys->ReadLocale("New Game");
	buttonNewGame->size.y = 64.0f;
	buttonNewGame->fractionHeight = false;
	buttonNewGame->margin = vec2(16.0f);
	AddWidget(buttonList, buttonNewGame);

	buttonSettings = new Button();
	buttonSettings->string = sys->ReadLocale("Settings");
	buttonSettings->size.y = 64.0f;
	buttonSettings->fractionHeight = false;
	buttonSettings->margin = vec2(16.0f);
	AddWidget(buttonList, buttonSettings);

	buttonExit = new Button();
	buttonExit->string = sys->ReadLocale("Exit");
	buttonExit->size.y = 64.0f;
	buttonExit->fractionHeight = false;
	buttonExit->margin = vec2(16.0f);
	buttonExit->highlightBG = vec4(colorBack, 0.9f);
	buttonExit->keycodeActivators = {KC_KEY_ESC};
	AddWidget(buttonList, buttonExit);

	ListH *spacingList = new ListH();
	spacingList->color = vec4(0.0f);
	spacingList->highlight = vec4(0.0f);
	spacingList->size.y = 0.0f;

	spacer = new Widget();
	spacer->size.x = 0.5f;
	AddWidget(spacingList, spacer);

	AddWidgetAsDefault(spacingList, buttonList);

	AddWidgetAsDefault(listV, spacingList);

	AddWidget(&screen, listV);
}

void MainMenu::Update() {
	screen.Update(vec2(0.0f), true);
	if (buttonContinue->state.Released()) {
		gui->nextMenu = Gui::Menu::PLAY;
	}
	if (buttonNewGame->state.Released()) {
		gui->nextMenu = Gui::Menu::PLAY;
		continueHideable->hidden = false;
	}
	if (buttonSettings->state.Released()) {
		gui->nextMenu = Gui::Menu::SETTINGS;
	}
	if (buttonExit->state.Released()) {
		sys->exit = true;
	}
}

void MainMenu::Draw(Rendering::DrawingContext &context) {
	screen.Draw(context);
}

void SettingsMenu::Initialize() {
	ListV *listV = new ListV();
	listV->color = vec4(0.0f);
	listV->highlight = vec4(0.0f);

	Widget *spacer = new Widget();
	spacer->size.y = 0.3f;
	AddWidget(listV, spacer);

	Text *title = new Text();
	title->alignH = Rendering::CENTER;
	title->bold = true;
	title->color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	title->colorOutline = vec4(1.0f);
	title->outline = true;
	title->fontSize = 64.0f;
	title->fontIndex = gui->fontIndex;
	title->string = sys->ReadLocale("Settings");
	AddWidget(listV, title);

	spacer = new Widget();
	spacer->size.y = 0.4f;
	AddWidget(listV, spacer);

	ListV *actualList = new ListV();
	actualList->fractionWidth = false;
	actualList->size.x = 500.0f;
	actualList->size.y = 0.0f;
	actualList->padding = vec2(24.0f);

	Text *settingTextTemplate = new Text();
	settingTextTemplate->fontIndex = gui->fontIndex;
	settingTextTemplate->fontSize = 20.0f;
	settingTextTemplate->fractionHeight = true;
	settingTextTemplate->size.y = 1.0f;
	settingTextTemplate->alignV = Rendering::CENTER;

	checkFullscreen = new Checkbox();
	checkFullscreen->checked = Settings::ReadBool(Settings::sFullscreen);
	
	checkVSync = new Checkbox();
	checkVSync->checked = Settings::ReadBool(Settings::sVSync);

	TextBox *textboxTemplate = new TextBox();
	textboxTemplate->fontIndex = gui->fontIndex;
	textboxTemplate->size.x = 64.0f;
	textboxTemplate->alignH = Rendering::RIGHT;
	textboxTemplate->textFilter = TextFilterDigits;
	textboxTemplate->textValidate = TextValidateNonempty;

	Slider *sliderTemplate = new Slider();
	sliderTemplate->fractionHeight = true;
	sliderTemplate->fractionWidth = false;
	sliderTemplate->size = vec2(116.0f, 1.0f);
	sliderTemplate->valueMax = 100.0f;

	textboxFramerate = new TextBox(*textboxTemplate);
	textboxFramerate->string = ToWString(ToString((i32)Settings::ReadReal(Settings::sFramerate)));


	for (i32 i = 0; i < 3; i++) {
		textboxVolumes[i] = new TextBox(*textboxTemplate);
		sliderVolumes[i] = new Slider(*sliderTemplate);
		textboxVolumes[i]->textFilter = TextFilterDecimalsPositive;
		textboxVolumes[i]->textValidate = TextValidateDecimalsPositive;
		sliderVolumes[i]->mirror = textboxVolumes[i];
	}
	f32 volumeMain = Settings::ReadReal(Settings::sVolumeMain);
	f32 volumeMusic = Settings::ReadReal(Settings::sVolumeMusic);
	f32 volumeEffects = Settings::ReadReal(Settings::sVolumeEffects);
	textboxVolumes[0]->string = ToWString(ToString(volumeMain*100.0f, 10, 1));
	textboxVolumes[1]->string = ToWString(ToString(volumeMusic*100.0f, 10, 1));
	textboxVolumes[2]->string = ToWString(ToString(volumeEffects*100.0f, 10, 1));
	sliderVolumes[0]->value = volumeMain*100.0f;
	sliderVolumes[1]->value = volumeMusic*100.0f;
	sliderVolumes[2]->value = volumeEffects*100.0f;

	ListH *settingListTemplate = new ListH();
	settingListTemplate->size.y = 0.0f;
	settingListTemplate->margin = vec2(8.0f);
	settingListTemplate->padding = vec2(0.0f);

	Array<Widget*> settingListItems = {
		checkFullscreen, nullptr,
		checkVSync, nullptr,
		textboxFramerate, nullptr,
		nullptr, nullptr,
		sliderVolumes[0], textboxVolumes[0],
		sliderVolumes[1], textboxVolumes[1],
		sliderVolumes[2], textboxVolumes[2]
	};
	const char *settingListNames[] = {
		"Fullscreen",
		"VSync",
		"Framerate",
		"Volume",
		"Main",
		"Music",
		"Effects"
	};

	for (i32 i = 0; i < settingListItems.size; i+=2) {
		if (settingListItems[i] == nullptr) {
			Text *settingText = new Text(*settingTextTemplate);
			settingText->string = sys->ReadLocale(settingListNames[i / 2]);
			settingText->alignH = Rendering::CENTER;
			settingText->fontSize = 24.0f;
			AddWidget(actualList, settingText);
		} else {
			ListH *settingList = new ListH(*settingListTemplate);
			Text *settingText = new Text(*settingTextTemplate);
			settingText->string = sys->ReadLocale(settingListNames[i / 2]);
			AddWidget(settingList, settingText);
			AddWidgetAsDefault(settingList, settingListItems[i]);
			if (settingListItems[i+1] != nullptr) {
				AddWidget(settingList, settingListItems[i+1]);
			}

			if (i == 4) {
				// Hideable Framerate
				framerateHideable = new Hideable(settingList);
				framerateHideable->hidden = Settings::ReadBool(Settings::sVSync);
				AddWidget(actualList, framerateHideable);
			} else {
				AddWidget(actualList, settingList);
			}
		}
	}

	ListH *buttonList = new ListH();
	buttonList->size.y = 0.0f;
	buttonList->margin = vec2(0.0f);
	buttonList->padding = vec2(0.0f);
	buttonList->color = vec4(0.0f);
	buttonList->highlight = vec4(0.0f);

	buttonBack = new Button();
	buttonBack->string = sys->ReadLocale("Back");
	buttonBack->size.x = 1.0f / 2.0f;
	buttonBack->size.y = 64.0f;
	buttonBack->fractionHeight = false;
	buttonBack->margin = vec2(8.0f);
	buttonBack->highlightBG = vec4(colorBack, 0.9f);
	buttonBack->keycodeActivators = {KC_GP_BTN_B, KC_KEY_ESC};
	AddWidget(buttonList, buttonBack);

	buttonApply = new Button();
	buttonApply->string = sys->ReadLocale("Apply");
	buttonApply->size.x = 1.0f / 2.0f;
	buttonApply->size.y = 64.0f;
	buttonApply->fractionHeight = false;
	buttonApply->margin = vec2(8.0f);
	AddWidgetAsDefault(buttonList, buttonApply);

	AddWidget(actualList, buttonList);

	ListH *spacingList = new ListH();
	spacingList->color = vec4(0.0f);
	spacingList->highlight = vec4(0.0f);
	spacingList->size.y = 0.0f;

	spacer = new Widget();
	spacer->size.x = 0.5f;
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
	framerateHideable->hidden = checkVSync->checked;
	screen.Update(vec2(0.0f), true);
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
		Settings::SetReal(Settings::sVolumeMain, f64(sliderVolumes[0]->value / 100.0f));
		Settings::SetReal(Settings::sVolumeMusic, f64(sliderVolumes[1]->value / 100.0f));
		Settings::SetReal(Settings::sVolumeEffects, f64(sliderVolumes[2]->value / 100.0f));
		for (i32 i = 0; i < 3; i++) {
			textboxVolumes[i]->string = ToWString(ToString(sliderVolumes[i]->value, 10, 1));
		}
	}
	if (buttonBack->state.Released()) {
		gui->nextMenu = Gui::Menu::MAIN;
	}
}

void SettingsMenu::Draw(Rendering::DrawingContext &context) {
	screen.Draw(context);
}

void UpgradesMenu::Initialize() {
	ListH *list = new ListH();
	list->fractionWidth = false;
	list->fractionHeight = false;
	list->size = 0.0f;
	list->color = vec4(vec3(0.05f), 0.8f);
	list->highlight = list->color;
	list->padding *= 0.5f;

	ListV *listStats = new ListV();
	listStats->fractionWidth = false;
	listStats->fractionHeight = false;
	listStats->size.x = 250.0f;
	listStats->size.y = 0.0f;
	listStats->margin = 0.0f;
	listStats->padding = 0.0f;
	listStats->color = 0.0f;
	listStats->highlight = 0.0f;

	Text *titleText = new Text();
	titleText->fontIndex = gui->fontIndex;
	titleText->alignH = Rendering::CENTER;
	titleText->alignV = Rendering::CENTER;
	titleText->bold = true;
	titleText->fontSize = 24.0f;
	titleText->fractionWidth = true;
	titleText->fractionHeight = false;
	titleText->size.x = 1.0f;
	titleText->size.y = 0.0f;
	titleText->string = sys->ReadLocale("Info");
	AddWidget(listStats, titleText);

	ListH *selectedTowerPriorityList = new ListH();
	selectedTowerPriorityList->fractionWidth = true;
	selectedTowerPriorityList->size.x = 1.0f;
	selectedTowerPriorityList->fractionHeight = false;
	selectedTowerPriorityList->size.y = 0.0f;
	selectedTowerPriorityList->padding = vec2(0.0f);
	selectedTowerPriorityList->margin = vec2(0.0f);
	selectedTowerPriorityList->color = 0.0f;
	selectedTowerPriorityList->highlight = 0.0f;
	Text *selectedTowerPriorityText = new Text();
	selectedTowerPriorityText->color = 1.0f;
	selectedTowerPriorityText->size.x = 0.5f;
	selectedTowerPriorityText->size.y = 1.0f;
	selectedTowerPriorityText->fractionHeight = true;
	selectedTowerPriorityText->alignV = Rendering::CENTER;
	selectedTowerPriorityText->fontIndex = gui->fontIndex;
	selectedTowerPriorityText->fontSize = 18.0f;
	selectedTowerPriorityText->string = sys->ReadLocale("Priority");
	towerPriority = new Switch();
	towerPriority->size.x = 0.5f;
	towerPriority->size.y = 0.0f;
	towerPriority->padding = 0.0f;
	for (i32 i = 0; i < 6; i++) {
		Text *priorityText = new Text();
		priorityText->selectable = true;
		priorityText->size.x = 1.0f;
		priorityText->size.y = 22.0f;
		priorityText->margin = 2.0f;
		priorityText->fractionHeight = false;
		priorityText->fontIndex = gui->fontIndex;
		priorityText->fontSize = 18.0f;
		priorityText->alignV = Rendering::CENTER;
		priorityText->string = sys->ReadLocale(Entities::Tower::priorityStrings[i]);
		AddWidget(towerPriority, priorityText);
	}
	AddWidget(selectedTowerPriorityList, selectedTowerPriorityText);
	AddWidgetAsDefault(selectedTowerPriorityList, towerPriority);
	towerPriorityHideable = new Hideable(selectedTowerPriorityList);
	AddWidgetAsDefault(listStats, towerPriorityHideable);
	selectedTowerStats = new Text();
	selectedTowerStats->size.x = 1.0f;
	selectedTowerStats->color = 1.0f;
	selectedTowerStats->fontIndex = gui->fontIndex;
	selectedTowerStats->fontSize = 18.0f;
	AddWidget(listStats, selectedTowerStats);

	ListV *listUpgrades = new ListV();
	listUpgrades->fractionWidth = false;
	listUpgrades->fractionHeight = false;
	listUpgrades->size.x = 300.0f;
	listUpgrades->size.y = 0.0f;
	listUpgrades->margin = 0.0f;
	listUpgrades->padding = 0.0f;
	listUpgrades->color = 0.0f;
	listUpgrades->highlight = 0.0f;
	listUpgrades->selectionDefault = 1;

	titleText = new Text();
	titleText->fontIndex = gui->fontIndex;
	titleText->alignH = Rendering::CENTER;
	titleText->alignV = Rendering::CENTER;
	titleText->bold = true;
	titleText->fontSize = 24.0f;
	titleText->fractionWidth = true;
	titleText->fractionHeight = false;
	titleText->size.x = 1.0f;
	titleText->size.y = 0.0f;
	titleText->string = sys->ReadLocale("Upgrades");
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
		listV->size = vec2(1.0f, 0.0f);
		listV->margin *= 0.5f;
		listV->padding = 0.0f;
		listV->color = 0.0f;
		listV->highlight = 0.0f;

		ListH *listH = new ListH();
		listH->fractionHeight = false;
		listH->size.y = 0.0f;
		listH->margin = 0.0f;
		listH->padding = 0.0f;
		listH->color = 0.0f;
		listH->highlight = 0.0f;

		Text *upgradeName = new Text();
		upgradeName->fractionWidth = true;
		upgradeName->size.x = 0.35f;
		upgradeName->fractionHeight = true;
		upgradeName->size.y = 1.0f;
		upgradeName->margin *= 0.5f;
		upgradeName->alignV = Rendering::CENTER;
		upgradeName->fontIndex = gui->fontIndex;
		upgradeName->fontSize = 18.0f;
		upgradeName->bold = true;
		upgradeName->string = sys->ReadLocale(upgradeNameStrings[i]);
		AddWidget(listH, upgradeName);

		upgradeStatus[i] = new Text();
		upgradeStatus[i]->fractionWidth = true;
		upgradeStatus[i]->size = vec2(0.4f, 0.0f);
		upgradeStatus[i]->margin *= 0.5f;
		upgradeStatus[i]->alignV = Rendering::CENTER;
		upgradeStatus[i]->fontIndex = gui->fontIndex;
		upgradeStatus[i]->fontSize = 14.0f;
		upgradeStatus[i]->string = ToWString("0");
		AddWidget(listH, upgradeStatus[i]);

		upgradeButton[i] = new Button();
		upgradeButton[i]->fractionWidth = true;
		upgradeButton[i]->fractionHeight = true;
		upgradeButton[i]->size.x = 0.25f;
		upgradeButton[i]->size.y = 1.0f;
		upgradeButton[i]->margin *= 0.5f;
		upgradeButton[i]->fontIndex = gui->fontIndex;
		upgradeButton[i]->fontSize = 18.0f;
		upgradeButton[i]->string = sys->ReadLocale("Buy");
		AddWidgetAsDefault(listH, upgradeButton[i]);

		AddWidgetAsDefault(listV, listH);

		Text *upgradeDescription = new Text();
		upgradeDescription->alignH = Rendering::CENTER;
		upgradeDescription->fractionWidth = true;
		upgradeDescription->size.x = 1.0f;
		upgradeDescription->margin = 0.0f;
		upgradeDescription->fontIndex = gui->fontIndex;
		upgradeDescription->fontSize = 14.0f;
		upgradeDescription->string = sys->ReadLocale(upgradeDescriptionStrings[i]);
		AddWidget(listV, upgradeDescription);

		upgradeHideable[i] = new Hideable(listV);
		AddWidget(listUpgrades, upgradeHideable[i]);
	}
	AddWidgetAsDefault(list, listStats);
	AddWidget(list, listUpgrades);
	hideable = new Hideable(list);
	AddWidget(&screen, hideable);
}

inline String FloatToString(f32 in) {
	return ToString(in, 10, 2);
}

void UpgradesMenu::Update() {
	if (entities->selectedTower != -1) {
		hideable->hidden = false;
		vec2 towerScreenPos = entities->WorldPosToScreen(entities->towers[entities->selectedTower].physical.pos) / gui->scale;
		hideable->position = towerScreenPos - vec2(hideable->sizeAbsolute.x / 2.0f, 0.0f);

		Entities::Tower &tower = entities->towers.GetMutable(entities->selectedTower);
		towerPriorityHideable->hidden = !Entities::towerHasPriority[tower.type];
		const Entities::TowerUpgradeables &upgradeables = Entities::towerUpgradeables[tower.type];
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
			upgradeButton[0]->highlightBG = vec4(canUpgrade? colorHighlightMedium : vec3(0.8f, 0.1f, 0.1f), 1.0f);
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
			upgradeButton[1]->highlightBG = vec4(canUpgrade? colorHighlightMedium : vec3(0.8f, 0.1f, 0.1f), 1.0f);
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
			upgradeButton[2]->highlightBG = vec4(canUpgrade? colorHighlightMedium : vec3(0.8f, 0.1f, 0.1f), 1.0f);
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
			upgradeButton[3]->highlightBG = vec4(canUpgrade? colorHighlightMedium : vec3(0.8f, 0.1f, 0.1f), 1.0f);
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
			upgradeButton[4]->highlightBG = vec4(canUpgrade? colorHighlightMedium : vec3(0.8f, 0.1f, 0.1f), 1.0f);
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
	screen.Update(vec2(0.0f), !entities->focusMenu); // Hideable will handle selection culling
}

void UpgradesMenu::Draw(Rendering::DrawingContext &context) {
	screen.Draw(context);
}

void PlayMenu::Initialize() {
	ListH *screenListH = new ListH();
	screenListH->fractionWidth = true;
	screenListH->size.x = 1.0f;
	screenListH->padding = vec2(0.0f);
	screenListH->margin = vec2(0.0f);
	screenListH->color = 0.0f;
	screenListH->highlight = 0.0f;
	screenListH->occludes = false;
	AddWidget(&screen, screenListH);
	Widget *spacer = new Widget();
	spacer->fractionWidth = true;
	spacer->size.x = 1.0f;
	AddWidget(screenListH, spacer);
	list = new ListV();
	list->fractionHeight = true;
	list->fractionWidth = false;
	list->margin = 0.0f;
	list->size = vec2(300.0f, 1.0f);
	list->selectionDefault = 1;
	AddWidgetAsDefault(screenListH, list);

	Text *towerHeader = new Text();
	towerHeader->fontIndex = gui->fontIndex;
	towerHeader->alignH = Rendering::CENTER;
	towerHeader->string = sys->ReadLocale("Towers");
	AddWidget(list, towerHeader);

	ListH *gridBase = new ListH();
	gridBase->fractionWidth = true;
	gridBase->size.x = 1.0f;
	gridBase->fractionHeight = false;
	gridBase->size.y = 0.0f;
	gridBase->padding = vec2(0.0f);
	gridBase->margin = vec2(0.0f);
	gridBase->color = 0.0f;
	gridBase->highlight = 0.0f;
	gridBase->selectionDefault = 0;

	Button *halfWidth = new Button();
	halfWidth->fractionWidth = true;
	halfWidth->size.x = 0.5f;
	halfWidth->fractionHeight = false;
	halfWidth->size.y = 32.0f;
	halfWidth->fontIndex = gui->fontIndex;
	halfWidth->fontSize = 20.0f;

	towerButtons.Resize(Entities::TOWER_MAX_RANGE + 1);
	for (i32 i = 0; i < towerButtons.size; i+=2) {
		ListH *grid = new ListH(*gridBase);
		for (i32 j = 0; j < 2; j++) {
			i32 index = i+j;
			if (index > towerButtons.size) break;
			towerButtons[index] = new Button(*halfWidth);
			towerButtons[index]->string = sys->ReadLocale(Entities::towerStrings[index]);
			towerButtons[index]->highlightBG = Entities::Tower(Entities::TowerType(index)).color;
			AddWidget(grid, towerButtons[index]);
		}
		towerButtonLists.Append(grid);
		AddWidget(list, grid);
	}

	towerInfo = new Text();
	towerInfo->size.x = 1.0f;
	towerInfo->color = vec4(1.0f);
	towerInfo->fontIndex = gui->fontIndex;
	towerInfo->fontSize = 18.0f;
	towerInfo->string = ToWString("$MONEY");
	AddWidget(list, towerInfo);

	spacer = new Widget();
	spacer->fractionHeight = true;
	spacer->size.y = 1.0f;
	AddWidget(list, spacer);

	Button *fullWidth = new Button();
	fullWidth->fractionWidth = true;
	fullWidth->size.x = 1.0f;
	fullWidth->fractionHeight = false;
	fullWidth->size.y = 32.0f;
	fullWidth->fontIndex = gui->fontIndex;

	ListH *waveList = new ListH(*gridBase);

	waveTitle = new Text();
	waveTitle->size.x = 0.5f;
	waveTitle->size.y = 1.0f;
	waveTitle->fractionHeight = true;
	waveTitle->alignV = Rendering::CENTER;
	waveTitle->colorOutline = vec4(1.0f, 0.0f, 0.5f, 1.0f);
	waveTitle->color = vec4(1.0f);
	waveTitle->outline = true;
	waveTitle->fontIndex = gui->fontIndex;
	waveTitle->fontSize = 30.0f;
	// waveTitle->bold = true;
	waveTitle->margin.y = 0.0f;
	waveTitle->string = ToWString("Nothing");
	AddWidget(waveList, waveTitle);
	buttonStartWave = new Button(*halfWidth);
	buttonStartWave->string = sys->ReadLocale("Start Wave");
	buttonStartWave->size.y = 32.0f;
	buttonStartWave->keycodeActivators = {KC_GP_BTN_START, KC_KEY_SPACE};
	AddWidgetAsDefault(waveList, buttonStartWave);

	AddWidget(list, waveList);

	waveInfo = new Text();
	waveInfo->size.x = 1.0f;
	waveInfo->color = vec4(1.0f);
	waveInfo->fontIndex = gui->fontIndex;
	waveInfo->fontSize = 20.0f;
	waveInfo->string = ToWString("Nothing");
	AddWidget(list, waveInfo);

	buttonMenu = new Button(*fullWidth);
	buttonMenu->string = sys->ReadLocale("Menu");
	buttonMenu->keycodeActivators = {KC_GP_BTN_SELECT, KC_KEY_ESC};
	AddWidget(list, buttonMenu);

	delete gridBase;
	delete halfWidth;
	delete fullWidth;

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
		towerInfoString += "\n" + sys->ReadLocale("Cost") + ": $" + ToString(Entities::towerCosts[textTower]) + "\n" + sys->ReadLocale(Entities::towerDescriptions[textTower]);
	}
	towerInfo->string = towerInfoString;
	waveTitle->string = sys->ReadLocale("Wave") + ": " + ToString(entities->wave);
	waveInfo->string =
		sys->ReadLocale("Wave Hitpoints Left") + ": " + ToString(entities->hitpointsLeft) + "\n"
		+ sys->ReadLocale("Lives") + ": " + ToString(entities->lives);
	screen.Update(vec2(0.0f), entities->focusMenu);
	if (buttonMenu->state.Released()) {
		gui->nextMenu = Gui::Menu::MAIN;
		sys->paused = true;
		if (entities->waveActive) {
			buttonStartWave->string = sys->ReadLocale("Resume");
		}
	}
}

void PlayMenu::Draw(Rendering::DrawingContext &context) {
	upgradesMenu.Draw(context);
	screen.Draw(context);
}

} // namespace Az2D::Gui

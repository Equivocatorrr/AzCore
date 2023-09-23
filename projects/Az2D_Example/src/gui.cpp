/*
	File: gui.cpp
	Author: Philip Haynes
*/

#include "gui.hpp"
#include "entities.hpp"

#include "Az2D/game_systems.hpp"
#include "Az2D/settings.hpp"
#include "Az2D/profiling.hpp"

namespace Az2D::Gui {

using namespace AzCore;

using GameSystems::sys;

Gui *gui = nullptr;

Gui::Gui() {
	gui = this;
}

const vec3 colorBack = {1.0f, 0.4f, 0.1f};
const vec3 colorHighlightLow = {0.2f, 0.45f, 0.5f};
const vec3 colorHighlightMedium = {0.4f, 0.9f, 1.0f};
const vec3 colorHighlightHigh = {0.9f, 0.98f, 1.0f};

void Gui::EventInitialize() {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Gui::Gui::EventInitialize)
	GuiBasic::EventInitialize();
	menuMain.Initialize();
	menuSettings.Initialize();
	menuPlay.Initialize();
}

void Gui::EventSync() {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Gui::Gui::EventSync)
	GuiBasic::EventSync();
	currentMenu = nextMenu;
	if (console) {
		sys->paused = true;
	} else {
		switch (currentMenu) {
		case Menu::MAIN:
			sys->paused = true;
			menuMain.Update();
			break;
		case Menu::SETTINGS:
			sys->paused = true;
			menuSettings.Update();
			break;
		case Menu::PLAY:
			sys->paused = false;
			menuPlay.Update();
			break;
		}
	}
}

void Gui::EventDraw(Array<Rendering::DrawingContext> &contexts) {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Gui::Gui::EventDraw)
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
	title->string = sys->ReadLocale("Az2D Example");
	AddWidget(listV, title);

	spacer = new Widget();
	spacer->size.y = 0.4f;
	AddWidget(listV, spacer);

	ListV *buttonList = new ListV();
	buttonList->fractionWidth = false;
	buttonList->size = vec2(500.0f, 0.0f);
	buttonList->padding = vec2(16.0f);

	buttonContinue = new Button();
	buttonContinue->size.y = 64.0f;
	buttonContinue->fractionHeight = false;
	buttonContinue->margin = vec2(16.0f);
	buttonContinue->AddDefaultText(sys->ReadLocale("Continue"));
	buttonContinue->keycodeActivators = {KC_KEY_ESC};

	continueHideable = new Hideable(buttonContinue);
	continueHideable->hidden = true;
	AddWidget(buttonList, continueHideable);

	buttonNewGame = new Button();
	buttonNewGame->size.y = 64.0f;
	buttonNewGame->fractionHeight = false;
	buttonNewGame->margin = vec2(16.0f);
	buttonNewGame->AddDefaultText(sys->ReadLocale("New Game"));
	AddWidget(buttonList, buttonNewGame);

	buttonSettings = new Button();
	buttonSettings->size.y = 64.0f;
	buttonSettings->fractionHeight = false;
	buttonSettings->margin = vec2(16.0f);
	buttonSettings->AddDefaultText(sys->ReadLocale("Settings"));
	AddWidget(buttonList, buttonSettings);

	buttonExit = new Button();
	buttonExit->size.y = 64.0f;
	buttonExit->fractionHeight = false;
	buttonExit->margin = vec2(16.0f);
	buttonExit->highlightBG = vec4(colorBack, 0.9f);
	buttonExit->AddDefaultText(sys->ReadLocale("Exit"));

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
		gui->menuSettings.Reset();
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
	textboxTemplate->size.x = 72.0f;
	textboxTemplate->alignH = Rendering::RIGHT;
	textboxTemplate->textFilter = TextFilterDigits;
	textboxTemplate->textValidate = TextValidateNonempty;

	Slider *sliderTemplate = new Slider();
	sliderTemplate->fractionHeight = true;
	sliderTemplate->fractionWidth = false;
	sliderTemplate->size = vec2(116.0f, 1.0f);
	sliderTemplate->valueMin = -60.0f;
	sliderTemplate->valueMax = 0.0f;
	sliderTemplate->valueTick = 3.0f;
	sliderTemplate->valueTickShiftMult = 1.0f / 3.0f;
	sliderTemplate->minOverride = true;
	sliderTemplate->minOverrideValue = -INFINITY;
	sliderTemplate->maxOverride = true;
	sliderTemplate->maxOverrideValue = -0.0f;

	textboxFramerate = new TextBox(*textboxTemplate);
	textboxFramerate->stringSuffix = ToWString("fps");


	for (i32 i = 0; i < 3; i++) {
		textboxVolumes[i] = new TextBox(*textboxTemplate);
		sliderVolumes[i] = new Slider(*sliderTemplate);
		textboxVolumes[i]->stringSuffix = ToWString("dB");
		textboxVolumes[i]->textFilter = TextFilterBasic;
		textboxVolumes[i]->textValidate = TextValidateDecimalsNegativeAndInfinity;
		sliderVolumes[i]->mirror = textboxVolumes[i];
	}

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
				settingListItems[i+1]->selectable = false;
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
	buttonBack->size.x = 1.0f / 2.0f;
	buttonBack->size.y = 64.0f;
	buttonBack->fractionHeight = false;
	buttonBack->margin = vec2(8.0f);
	buttonBack->highlightBG = vec4(colorBack, 0.9f);
	buttonBack->keycodeActivators = {KC_GP_BTN_B, KC_KEY_ESC};
	buttonBack->AddDefaultText(sys->ReadLocale("Back"));
	AddWidget(buttonList, buttonBack);

	buttonApply = new Button();
	buttonApply->size.x = 1.0f / 2.0f;
	buttonApply->size.y = 64.0f;
	buttonApply->fractionHeight = false;
	buttonApply->margin = vec2(8.0f);
	buttonApply->AddDefaultText(sys->ReadLocale("Apply"));
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
}

void SettingsMenu::Update() {
	framerateHideable->hidden = checkVSync->checked;
	screen.Update(vec2(0.0f), true);
	if (buttonApply->state.Released()) {
		Settings::SetBool(Settings::sFullscreen, checkFullscreen->checked);
		Settings::SetBool(Settings::sVSync, checkVSync->checked);
		u64 framerate = 60;
		if (textboxFramerate->textValidate(textboxFramerate->string)) {
			framerate = clamp(WStringToU64(textboxFramerate->string), (u64)30, (u64)600);
			sys->SetFramerate((f32)framerate);
		}
		Settings::SetReal(Settings::sFramerate, (f64)framerate);
		textboxFramerate->string = ToWString(ToString(framerate));
		Settings::SetReal(Settings::sVolumeMain, f64(az::decibelsToAmp(sliderVolumes[0]->GetActualValue())));
		Settings::SetReal(Settings::sVolumeMusic, f64(az::decibelsToAmp(sliderVolumes[1]->GetActualValue())));
		Settings::SetReal(Settings::sVolumeEffects, f64(az::decibelsToAmp(sliderVolumes[2]->GetActualValue())));
		for (i32 i = 0; i < 3; i++) {
			sliderVolumes[i]->UpdateMirror();
		}
	}
	if (buttonBack->state.Released()) {
		gui->nextMenu = Gui::Menu::MAIN;
	}
}

void SettingsMenu::Draw(Rendering::DrawingContext &context) {
	screen.Draw(context);
}

void PlayMenu::Initialize() {
	ListV *screenListV = new ListV();
	screenListV->fractionHeight = true;
	screenListV->size.y = 1.0f;
	screenListV->padding = vec2(0.0f);
	screenListV->margin = vec2(0.0f);
	screenListV->color = 0.0f;
	screenListV->highlight = 0.0f;
	screenListV->occludes = false;
	AddWidget(&screen, screenListV);
	ListH *listTop = new ListH();
	listTop->fractionHeight = false;
	listTop->fractionWidth = true;
	listTop->margin = 0.0f;
	listTop->color = 0.0f;
	listTop->highlight = 0.0f;
	listTop->size = vec2(1.0f, 80.0f);
	listTop->occludes = false;
	AddWidget(screenListV, listTop);

	Widget *spacer = new Widget();
	spacer->fractionHeight = true;
	spacer->size.y = 1.0f;
	AddWidget(screenListV, spacer);

	ListH *listBottom = new ListH();
	listBottom->fractionHeight = false;
	listBottom->fractionWidth = true;
	listBottom->color = 0.0f;
	listBottom->highlight = 0.0f;
	listBottom->margin = 0.0f;
	listBottom->size = vec2(1.0f, 80.0f);
	listBottom->occludes = false;
	AddWidgetAsDefault(screenListV, listBottom);

	buttonMenu = new Button();
	buttonMenu->fractionWidth = false;
	buttonMenu->size.x = 120.0f;
	buttonMenu->keycodeActivators = {KC_GP_BTN_START, KC_KEY_ESC};
	buttonMenu->AddDefaultText(sys->ReadLocale("Menu"));
	AddWidgetAsDefault(listBottom, buttonMenu);

	spacer = new Widget();
	spacer->fractionWidth = true;
	spacer->size.x = 1.0f;
	AddWidget(listBottom, spacer);

	buttonReset = new Button(*buttonMenu);
	buttonReset->children.Clear();
	buttonReset->keycodeActivators = {KC_GP_BTN_SELECT, KC_KEY_R};
	buttonReset->AddDefaultText(sys->ReadLocale("Reset"));
	AddWidget(listBottom, buttonReset);
}

void PlayMenu::Update() {
	screen.Update(vec2(0.0f), false);
	if (buttonMenu->state.Released()) {
		gui->nextMenu = Gui::Menu::MAIN;
		sys->paused = true;
	} else {
		sys->paused = false;
	}
}

void PlayMenu::Draw(Rendering::DrawingContext &context) {
	screen.Draw(context);
}

} // namespace Int
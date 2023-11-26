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
	AZCORE_PROFILING_SCOPED_TIMER(Az2D::Gui::Gui::EventInitialize)
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
}

void MainMenu::Initialize() {
	screen = gui->system.CreateScreen();
	azgui::ListV *listV = gui->system.CreateListV(screen);
	listV->color = vec4(0.0f);
	listV->colorHighlighted = vec4(0.0f);

	azgui::Spacer *spacer = gui->system.CreateSpacer(listV);
	spacer->SetHeightFraction(0.3f);

	azgui::Text *title = gui->system.CreateText(listV);
	title->data = TextMetadata{Rendering::CENTER, Rendering::TOP};
	title->bold = true;
	title->color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	title->colorOutline = vec4(1.0f);
	title->outline = true;
	title->fontSize = 64.0f;
	title->string = sys->ReadLocale("Az2D Example");

	spacer = gui->system.CreateSpacer(listV);
	spacer->SetHeightFraction(0.4f);
	
	azgui::ListH *spacingList = gui->system.CreateListH(listV);
	spacingList->color = vec4(0.0f);
	spacingList->colorHighlighted = vec4(0.0f);
	spacingList->SetHeightContents();

	spacer = gui->system.CreateSpacer(spacingList);
	spacer->SetWidthFraction(0.5f);

	azgui::ListV *buttonList = gui->system.CreateListV(spacingList);
	buttonList->SetWidthPixel(500.0f);
	buttonList->SetHeightContents();
	buttonList->padding = vec2(16.0f);

	buttonContinue = gui->system.CreateButton(nullptr);
	buttonContinue->size.y = 64.0f;
	buttonContinue->fractionHeight = false;
	buttonContinue->margin = vec2(16.0f);
	buttonContinue->AddDefaultText(sys->ReadLocale("Continue"));
	buttonContinue->keycodeActivators = {KC_KEY_ESC};

	continueHideable = gui->system.CreateHideable(buttonList, buttonContinue);
	continueHideable->hidden = true;

	buttonNewGame = gui->system.CreateButton(buttonList);
	buttonNewGame->SetHeightPixel(64.0f);
	buttonNewGame->margin = vec2(16.0f);
	buttonNewGame->AddDefaultText(sys->ReadLocale("New Game"));

	buttonSettings = gui->system.CreateButton(buttonList);
	buttonSettings->SetHeightPixel(64.0f) ;
	buttonSettings->margin = vec2(16.0f);
	buttonSettings->AddDefaultText(sys->ReadLocale("Settings"));

	buttonExit = gui->system.CreateButton(buttonList);
	buttonExit->SetHeightPixel(64.0f);
	buttonExit->margin = vec2(16.0f);
	buttonExit->colorHighlighted = vec4(colorBack, 0.9f);
	buttonExit->AddDefaultText(sys->ReadLocale("Exit"));
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
	gui->currentContext = &context;
	screen->Draw();
}

void SettingsMenu::Initialize() {
	screen = gui->system.CreateScreen();
	azgui::ListV *listV = gui->system.CreateListV(screen);
	listV->color = vec4(0.0f);
	listV->colorHighlighted = vec4(0.0f);

	azgui::Spacer *spacer = gui->system.CreateSpacer(listV);
	spacer->SetHeightFraction(0.3f);

	azgui::Text *title = gui->system.CreateText(listV);
	title->data = TextMetadata{Rendering::CENTER, Rendering::TOP};
	title->bold = true;
	title->color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	title->colorOutline = vec4(1.0f);
	title->outline = true;
	title->fontSize = 64.0f;
	title->string = sys->ReadLocale("Settings");

	spacer = gui->system.CreateSpacer(listV);
	spacer->SetHeightFraction(0.4f);
	
	azgui::ListH *spacingList = gui->system.CreateListH(listV);
	spacingList->color = vec4(0.0f);
	spacingList->colorHighlighted = vec4(0.0f);
	spacingList->SetHeightContents();

	spacer = gui->system.CreateSpacer(spacingList);
	spacer->SetWidthFraction(0.5f);

	azgui::ListV *actualList = gui->system.CreateListV(spacingList);
	actualList->SetWidthPixel(500.0f);
	actualList->SetHeightContents();
	actualList->padding = vec2(24.0f);

	azgui::Text settingTextTemplate;
	settingTextTemplate.fontSize = 20.0f;
	settingTextTemplate.SetHeightFraction(1.0f);
	settingTextTemplate.data = TextMetadata{Rendering::LEFT, Rendering::CENTER};

	checkFullscreen = new azgui::Checkbox();
	checkFullscreen->checked = Settings::ReadBool(Settings::sFullscreen);

	checkVSync = new azgui::Checkbox();
	checkVSync->checked = Settings::ReadBool(Settings::sVSync);

	azgui::Textbox textboxTemplate;
	textboxTemplate.SetWidthPixel(72.0f);
	textboxTemplate.SetHeightFraction(1.0f);
	textboxTemplate.data = TextMetadata{Rendering::RIGHT, Rendering::CENTER};
	textboxTemplate.textFilter = azgui::TextFilterDigits;
	textboxTemplate.textValidate = azgui::TextValidateNonempty;

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

	textboxFramerate = new azgui::Textbox(textboxTemplate);
	textboxFramerate->stringSuffix = ToWString("fps");


	for (i32 i = 0; i < 3; i++) {
		textboxVolumes[i] = new azgui::Textbox(textboxTemplate);
		sliderVolumes[i] = new azgui::Slider(sliderTemplate);
		textboxVolumes[i]->stringSuffix = ToWString("dB");
		textboxVolumes[i]->textFilter = azgui::TextFilterBasic;
		textboxVolumes[i]->textValidate = azgui::TextValidateDecimalsNegativeAndInfinity;
		sliderVolumes[i]->mirror = textboxVolumes[i];
	}

	azgui::ListH settingListTemplate;
	settingListTemplate.SetHeightContents();
	settingListTemplate.margin = vec2(8.0f);
	settingListTemplate.padding = vec2(0.0f);

	Array<azgui::Widget*> settingListItems = {
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
			azgui::Text *settingText = gui->system.CreateTextFrom(actualList, settingTextTemplate);
			settingText->string = sys->ReadLocale(settingListNames[i / 2]);
			settingText->data = TextMetadata{Rendering::CENTER, Rendering::CENTER};
			settingText->fontSize = 24.0f;
		} else {
			azgui::ListH *settingList = new azgui::ListH(settingListTemplate);
			azgui::Text *settingText = new azgui::Text(settingTextTemplate);
			settingText->string = sys->ReadLocale(settingListNames[i / 2]);
			gui->system.AddWidget(settingList, settingText);
			gui->system.AddWidgetAsDefault(settingList, settingListItems[i]);
			if (settingListItems[i+1] != nullptr) {
				settingListItems[i+1]->selectable = false;
				gui->system.AddWidget(settingList, settingListItems[i+1]);
			}

			if (i == 4) {
				// Hideable Framerate
				framerateHideable = gui->system.CreateHideable(actualList, settingList);
				framerateHideable->hidden = Settings::ReadBool(Settings::sVSync);
			} else {
				gui->system.AddWidget(actualList, settingList);
			}
		}
	}

	azgui::ListH *buttonList = gui->system.CreateListH(actualList);
	buttonList->SetHeightContents();
	buttonList->margin = vec2(0.0f);
	buttonList->padding = vec2(0.0f);
	buttonList->color = vec4(0.0f);
	buttonList->colorHighlighted = vec4(0.0f);
	
	azgui::Button buttonTemplate;
	buttonTemplate.SetWidthFraction(1.0f / 2.0f);
	buttonTemplate.SetHeightPixel(64.0f);
	buttonTemplate.margin = vec2(8.0f);

	buttonBack = gui->system.CreateButtonFrom(buttonList, buttonTemplate);
	buttonBack->colorHighlighted = vec4(colorBack, 0.9f);
	buttonBack->keycodeActivators = {KC_GP_BTN_B, KC_KEY_ESC};
	buttonBack->AddDefaultText(sys->ReadLocale("Back"));

	buttonApply = gui->system.CreateButtonFrom(buttonList, buttonTemplate);
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
	screen->Update(vec2(0.0f), true);
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
	gui->currentContext = &context;
	screen->Draw();
}

void PlayMenu::Initialize() {
	screen = gui->system.CreateScreen();
	azgui::ListV *screenListV = gui->system.CreateListV(screen);
	screenListV->SetHeightFraction(1.0f);
	screenListV->padding = vec2(0.0f);
	screenListV->margin = vec2(0.0f);
	screenListV->color = 0.0f;
	screenListV->colorHighlighted = 0.0f;
	screenListV->occludes = false;
	
	azgui::ListH *listTop = gui->system.CreateListH(screenListV);
	listTop->SetWidthFraction(1.0f);
	listTop->SetHeightPixel(80.0f);
	listTop->margin = 0.0f;
	listTop->color = 0.0f;
	listTop->colorHighlighted = 0.0f;
	listTop->occludes = false;

	azgui::Spacer *spacer = gui->system.CreateSpacer(screenListV);
	spacer->SetHeightFraction(1.0f);

	azgui::ListH *listBottom = gui->system.CreateListH(screenListV);
	listBottom->SetWidthFraction(1.0f);
	listBottom->SetHeightPixel(80.0f);
	listBottom->color = 0.0f;
	listBottom->colorHighlighted = 0.0f;
	listBottom->margin = 0.0f;
	listBottom->occludes = false;

	buttonMenu = gui->system.CreateButton(listBottom);
	buttonMenu->SetWidthPixel(120.0f);
	buttonMenu->keycodeActivators = {KC_GP_BTN_START, KC_KEY_ESC};
	buttonMenu->AddDefaultText(sys->ReadLocale("Menu"));

	spacer = gui->system.CreateSpacer(listBottom);
	spacer->SetWidthFraction(1.0f);

	buttonReset = gui->system.CreateButton(listBottom);
	buttonReset->SetWidthPixel(120.0f);
	buttonReset->keycodeActivators = {KC_GP_BTN_SELECT, KC_KEY_R};
	buttonReset->AddDefaultText(sys->ReadLocale("Reset"));
}

void PlayMenu::Update() {
	screen->Update(vec2(0.0f), false);
	if (buttonMenu->state.Released()) {
		gui->nextMenu = Gui::Menu::MAIN;
		sys->paused = true;
	} else {
		sys->paused = false;
	}
}

void PlayMenu::Draw(Rendering::DrawingContext &context) {
	gui->currentContext = &context;
	screen->Draw();
}

} // namespace Int
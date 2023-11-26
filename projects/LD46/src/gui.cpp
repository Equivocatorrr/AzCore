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
using Entities::entities;

Gui *gui = nullptr;

Gui::Gui() {
	gui = this;
}

const vec3 colorBack = {1.0f, 0.4f, 0.1f};
const vec3 colorHighlightLow = {0.2f, 0.45f, 0.5f};
const vec3 colorHighlightMedium = {0.4f, 0.9f, 1.0f};
const vec3 colorHighlightHigh = {0.9f, 0.98f, 1.0f};

void Gui::EventAssetsQueue() {
	GuiBasic::EventAssetsQueue();
	sys->assets.QueueFile("beep short.ogg");
	sys->assets.QueueFile("dramatic beep.ogg");
	sys->assets.QueueFile("phone buzz.ogg");

	sys->assets.QueueFile("Intro/Intro-1.png");
	sys->assets.QueueFile("Intro/Intro-2.png");
	sys->assets.QueueFile("Intro/Intro-3.png");
	sys->assets.QueueFile("Intro/Intro-4.png");
	sys->assets.QueueFile("Intro/Intro-5.png");

	sys->assets.QueueFile("Outro/Outro1.png");
	sys->assets.QueueFile("Outro/Outro2.png");
	sys->assets.QueueFile("Outro/Outro3.png");
	sys->assets.QueueFile("Outro/Outro4.png");
	sys->assets.QueueFile("Outro/Outro5.png");
	sys->assets.QueueFile("Outro/Outro6.png");

	sys->assets.QueueFile("Outro/Credits-Equivocator.png");
	sys->assets.QueueFile("Outro/Credits-Flubz.png");
}

void Gui::EventAssetsAcquire() {
	GuiBasic::EventAssetsAcquire();
	texIntro[0] = sys->assets.FindTexture("Intro/Intro-1.png");
	texIntro[1] = sys->assets.FindTexture("Intro/Intro-2.png");
	texIntro[2] = sys->assets.FindTexture("Intro/Intro-3.png");
	texIntro[3] = sys->assets.FindTexture("Intro/Intro-4.png");
	texIntro[4] = sys->assets.FindTexture("Intro/Intro-5.png");

	texOutro[0] = sys->assets.FindTexture("Outro/Outro1.png");
	texOutro[1] = sys->assets.FindTexture("Outro/Outro2.png");
	texOutro[2] = sys->assets.FindTexture("Outro/Outro3.png");
	texOutro[3] = sys->assets.FindTexture("Outro/Outro4.png");
	texOutro[4] = sys->assets.FindTexture("Outro/Outro5.png");
	texOutro[5] = sys->assets.FindTexture("Outro/Outro6.png");

	texCreditsEquivocator = sys->assets.FindTexture("Outro/Credits-Equivocator.png");
	texCreditsFlubz = sys->assets.FindTexture("Outro/Credits-Flubz.png");

	sndBeepShort.Create("beep short.ogg");
	sndBeepShort.SetGain(0.5f);
	sndBeepLong.Create("dramatic beep.ogg");
	sndBeepLong.SetGain(0.5f);
	sndPhoneBuzz.Create("phone buzz.ogg");
	sndPhoneBuzz.SetGain(0.5f);
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
	menuEditor.Initialize();
	menuCutscene.Initialize();
}

void Gui::EventSync() {
	AZCORE_PROFILING_SCOPED_TIMER(Az2D::Gui::Gui::EventSync)
	GuiBasic::EventSync();
	menuCurrent = menuNext;
	if (console) {
		sys->paused = true;
	} else {
		switch (menuCurrent) {
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
		case Menu::EDITOR:
			sys->paused = false;
			menuEditor.Update();
			break;
		case Menu::INTRO:
			sys->paused = false;
			menuCutscene.intro = true;
			menuCutscene.Update();
			break;
		case Menu::OUTTRO:
			sys->paused = false;
			menuCutscene.intro = false;
			menuCutscene.Update();
			break;
		}
	}
}

void Gui::EventDraw(Array<Rendering::DrawingContext> &contexts) {
	AZCORE_PROFILING_SCOPED_TIMER(Az2D::Gui::Gui::EventDraw)
	switch (menuCurrent) {
	case Menu::MAIN:
		menuMain.Draw(contexts.Back());
		break;
	case Menu::SETTINGS:
		menuSettings.Draw(contexts.Back());
		break;
	case Menu::PLAY:
		menuPlay.Draw(contexts.Back());
		break;
	case Menu::EDITOR:
		menuEditor.Draw(contexts.Back());
		break;
	case Menu::INTRO:
	case Menu::OUTTRO:
		menuCutscene.Draw(contexts.Back());
		break;
	}
	GuiBasic::EventDraw(contexts);
}

void MainMenu::Initialize() {
	screen = gui->system.CreateScreen();
	azgui::ListV *listV = gui->system.CreateListV(screen);
	listV->color = vec4(0.0f);
	listV->colorHighlighted = vec4(0.0f);

	azgui::Spacer *spacer = gui->system.CreateSpacer(listV, 0.3f);

	azgui::Text *title = gui->system.CreateText(listV);
	title->data = TextMetadata{Rendering::CENTER, Rendering::TOP};
	title->bold = true;
	title->color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	title->colorOutline = vec4(1.0f);
	title->outline = true;
	title->fontSize = 64.0f;
	title->string = sys->ReadLocale("Torch Runner");

	spacer = gui->system.CreateSpacer(listV, 0.4f);
	
	azgui::ListH *spacingList = gui->system.CreateListHAsDefault(listV);
	spacingList->color = vec4(0.0f);
	spacingList->colorHighlighted = vec4(0.0f);
	spacingList->SetHeightContents();

	spacer = gui->system.CreateSpacer(spacingList, 0.5f);

	azgui::ListV *buttonList = gui->system.CreateListVAsDefault(spacingList);
	buttonList->SetWidthPixel(500.0f);
	buttonList->SetHeightContents();
	buttonList->padding = vec2(16.0f);

	buttonContinue = gui->system.CreateButton(nullptr);
	continueHideable = gui->system.CreateHideable(buttonList, buttonContinue);
	continueHideable->hidden = true;
	buttonContinue->AddDefaultText(sys->ReadLocale("Continue"));
	buttonContinue->SetHeightPixel(64.0f);
	buttonContinue->margin = vec2(16.0f);
	buttonContinue->keycodeActivators = {KC_KEY_ESC};

	buttonNewGame = gui->system.CreateButton(buttonList);
	buttonNewGame->AddDefaultText(sys->ReadLocale("New Game"));
	buttonNewGame->SetHeightPixel(64.0f);
	buttonNewGame->margin = vec2(16.0f);

	buttonLevelEditor = gui->system.CreateButton(buttonList);
	buttonLevelEditor->AddDefaultText(sys->ReadLocale("Level Editor"));
	buttonLevelEditor->SetHeightPixel(64.0f);
	buttonLevelEditor->margin = vec2(16.0f);

	buttonSettings = gui->system.CreateButton(buttonList);
	buttonSettings->AddDefaultText(sys->ReadLocale("Settings"));
	buttonSettings->SetHeightPixel(64.0f);
	buttonSettings->margin = vec2(16.0f);

	buttonExit = gui->system.CreateButton(buttonList);
	buttonExit->AddDefaultText(sys->ReadLocale("Exit"));
	buttonExit->SetHeightPixel(64.0f);
	buttonExit->margin = vec2(16.0f);
	buttonExit->colorHighlighted = vec4(colorBack, 0.9f);
}

void MainMenu::Update() {
	screen->Update(vec2(0.0f), true);
	if (buttonContinue->state.Released()) {
		gui->menuNext = Gui::Menu::PLAY;
	}
	if (buttonNewGame->state.Released()) {
		gui->menuNext = Gui::Menu::INTRO;
		gui->menuCutscene.intro = true;
		gui->menuCutscene.Begin();
		continueHideable->hidden = false;
	}
	if (buttonLevelEditor->state.Released()) {
		gui->menuNext = Gui::Menu::EDITOR;
	}
	if (buttonSettings->state.Released()) {
		gui->menuNext = Gui::Menu::SETTINGS;
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

	azgui::Spacer *spacer = gui->system.CreateSpacer(listV, 0.3f);

	azgui::Text *title = gui->system.CreateText(listV);
	title->data = TextMetadata{Rendering::CENTER, Rendering::TOP};
	title->bold = true;
	title->color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	title->colorOutline = vec4(1.0f);
	title->outline = true;
	title->fontSize = 64.0f;
	title->string = sys->ReadLocale("Settings");

	spacer = gui->system.CreateSpacer(listV, 0.4f);
	
	azgui::ListH *spacingList = gui->system.CreateListHAsDefault(listV);
	spacingList->color = vec4(0.0f);
	spacingList->colorHighlighted = vec4(0.0f);
	spacingList->SetHeightContents();

	spacer = gui->system.CreateSpacer(spacingList, 0.5f);

	azgui::ListV *actualList = gui->system.CreateListVAsDefault(spacingList);
	actualList->SetWidthPixel(500.0f);
	actualList->SetHeightContents();
	actualList->padding = vec2(24.0f);

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
	textboxTemplate.data = TextMetadata{Rendering::RIGHT, Rendering::CENTER};
	textboxTemplate.textFilter = azgui::TextFilterDigits;
	textboxTemplate.textValidate = azgui::TextValidateNonempty;

	azgui::Slider sliderTemplate;
	sliderTemplate.SetWidthPixel(116.0f);
	sliderTemplate.SetHeightFraction(1.0f);
	sliderTemplate.valueMax = 100.0f;

	textboxFramerate = gui->system.CreateTextboxFrom(nullptr, textboxTemplate);
	textboxFramerate->string = ToWString(ToString((i32)Settings::ReadReal(Settings::sFramerate)));

	for (i32 i = 0; i < 3; i++) {
		textboxVolumes[i] = gui->system.CreateTextboxFrom(nullptr, textboxTemplate);
		sliderVolumes[i] = gui->system.CreateSliderFrom(nullptr, sliderTemplate);
		textboxVolumes[i]->textFilter = azgui::TextFilterDecimalsPositive;
		textboxVolumes[i]->textValidate = azgui::TextValidateDecimalsPositive;
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
			azgui::ListH *settingList = gui->system.CreateListHFrom(nullptr, settingListTemplate);
			if (i == 4) {
				// Hideable Framerate
				framerateHideable = gui->system.CreateHideable(actualList, settingList);
				framerateHideable->hidden = Settings::ReadBool(Settings::sVSync);
			} else {
				gui->system.AddWidget(actualList, settingList);
			}
			azgui::Text *settingText = gui->system.CreateTextFrom(settingList, settingTextTemplate);
			settingText->string = sys->ReadLocale(settingListNames[i / 2]);
			gui->system.AddWidgetAsDefault(settingList, settingListItems[i]);
			if (settingListItems[i+1] != nullptr) {
				// Allowing us to use the keyboard and gamepad to control the slider instead
				settingListItems[i+1]->selectable = false;
				gui->system.AddWidget(settingList, settingListItems[i+1]);
			}
		}
	}

	azgui::ListH *buttonList = gui->system.CreateListH(actualList);
	buttonList->SetHeightContents();
	buttonList->margin = vec2(0.0f);
	buttonList->padding = vec2(0.0f);
	buttonList->color = vec4(0.0f);
	buttonList->colorHighlighted = vec4(0.0f);

	buttonBack = gui->system.CreateButton(buttonList);
	buttonBack->AddDefaultText(sys->ReadLocale("Back"));
	buttonBack->SetWidthFraction(1.0f / 2.0f);
	buttonBack->SetHeightPixel(64.0f);
	buttonBack->margin = vec2(8.0f);
	buttonBack->colorHighlighted = vec4(colorBack, 0.9f);
	buttonBack->keycodeActivators = {KC_GP_BTN_B, KC_KEY_ESC};

	buttonApply = gui->system.CreateButtonAsDefault(buttonList);
	buttonApply->AddDefaultText(sys->ReadLocale("Apply"));
	buttonApply->SetWidthFraction(1.0f / 2.0f);
	buttonApply->SetHeightPixel(64.0f);
	buttonApply->margin = vec2(8.0f);
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
		Settings::SetReal(Settings::sVolumeMain, f64(sliderVolumes[0]->value / 100.0f));
		Settings::SetReal(Settings::sVolumeMusic, f64(sliderVolumes[1]->value / 100.0f));
		Settings::SetReal(Settings::sVolumeEffects, f64(sliderVolumes[2]->value / 100.0f));
		for (i32 i = 0; i < 3; i++) {
			textboxVolumes[i]->string = ToWString(ToString(sliderVolumes[i]->value, 10, 1));
		}
	}
	if (buttonBack->state.Released()) {
		gui->menuNext = Gui::Menu::MAIN;
	}
}

void SettingsMenu::Draw(Rendering::DrawingContext &context) {
	gui->currentContext = &context;
	screen->Draw();
}

inline String FloatToString(f32 in) {
	return ToString(in, 10, 2);
}

void CutsceneMenu::Initialize() {
	screen = gui->system.CreateScreen();
	azgui::ListH *screenListH = gui->system.CreateListH(screen);
	screenListH->margin = 0.0f;
	screenListH->padding = 0.0f;
	screenListH->color = vec4(vec3(0.0f), 1.0f);
	screenListH->colorHighlighted = screenListH->color;

	azgui::Spacer *spacer = gui->system.CreateSpacer(screenListH, 0.5f);

	azgui::ListV *listV = gui->system.CreateListVAsDefault(screenListH);
	listV->margin = 0.0f;
	listV->padding = 0.0f;
	listV->color = 0.0f;
	listV->colorHighlighted = 0.0f;
	listV->SetWidthContents();

	spacer = gui->system.CreateSpacer(listV, 0.5f);

	image = gui->system.CreateImage(listV);
	image->SetSizePixel(vec2(416.0f, 416.0f));
	image->margin = vec2(224.0f, 32.0f);

	text = gui->system.CreateText(listV);
	text->data = TextMetadata{Rendering::CENTER, Rendering::CENTER};
	text->SetSizePixel(vec2(800.0f, 100.0f));
	text->margin = 32.0f;
	text->string = sys->ReadLocale("This is the intro cutscene!");
	
	buttonSkip = gui->system.CreateButton(listV);
	buttonSkip->SetSizePixel(vec2(128.0f, 64.0f));
	buttonSkip->margin.x = 368.0f;
	buttonSkip->AddDefaultText(sys->ReadLocale("Skip"));
	buttonSkip->keycodeActivators = {KC_KEY_ESC, KC_KEY_SPACE};
	buttonSkip->color = 0.0f;

	introFrames = {
		{"", &gui->sndBeepShort, 0.5f, 0.5f, 0.0f, gui->texIntro[0], true},
		{"", &gui->sndBeepShort, 0.0f, 1.0f, 0.0f, gui->texIntro[1], true},
		{"", &gui->sndBeepShort, 0.0f, 1.0f, 0.0f, gui->texIntro[2], true},
		{"", &gui->sndBeepLong, 0.0f, 1.0f, 1.0f, gui->texIntro[3], true},
		{"It is time.", nullptr, 1.0f, 1.0f, 1.0f, 0, false},
		{"Light the beacon.", nullptr, 0.5f, 2.0f, 0.5f, gui->texIntro[4], true}
	};

	outtroFrames = {
		{"", nullptr, 0.5f, 1.0f, 0.0f, gui->texOutro[0], true},
		{"", &gui->sndPhoneBuzz, 0.0f, 1.0f, 0.5f, gui->texOutro[1], true},
		{"", nullptr, 0.5f, 1.5f, 0.0f, gui->texOutro[2], true},
		{"h- Huh?", nullptr, 0.0f, 2.0f, 0.0f, gui->texOutro[3], true},
		{"what's-", nullptr, 0.0f, 2.0f, 0.0f, gui->texOutro[4], true},
		{"Oh #&^$ ma P I Z Z A", nullptr, 0.0f, 2.0f, 1.0f, gui->texOutro[5], true},
		{"", nullptr, 0.2f, 0.1f, 0.2f, 0, false},
		{"Programming and Sound:\nEquivocator", &entities->jump1Sources[0], 0.5f, 2.0f, 0.5f, gui->texCreditsEquivocator, true},
		{"Art and Sound:\nFlubz", &entities->jump2Sources[0], 0.5f, 2.0f, 0.5f, gui->texCreditsFlubz, true},
		{"Thanks for playing!", nullptr, 0.5f, 2.0f, 0.5f, 0, false},
	};
}

void CutsceneMenu::Update() {
	screen->Update(vec2(0.0f), true);
	const Array<Frame> &frames = intro ? introFrames : outtroFrames;
	if (currentFrame < 0) {
		currentFrame = 0;
		frameTimer = 0.0f;
	}
	if (currentFrame >= frames.size || buttonSkip->state.Released()) {
		gui->menuNext = intro ? Gui::Menu::PLAY : Gui::Menu::MAIN;
		if (!intro) {
			entities->Reset();
			entities->camPos = -1.0f;
			entities->camZoom = 10000.0f;
		}
		return;
	}
	const Frame &frame = frames[currentFrame];
	if (frameTimer == 0.0f) {
		image->data = ImageMetadata{frame.image};
		text->string = sys->ReadLocale(frame.text);
		if (frame.sound) {
			frame.sound->Play();
		}
	}
	frameTimer += sys->timestep;
	if (frameTimer >= (frame.fadein + frame.duration + frame.fadeout)) {
		currentFrame++;
		frameTimer = 0.0f;
	} else if (frameTimer < frame.fadein) {
		// Do the fade
		f32 progress = clamp01(frameTimer / frame.fadein);
		if (frame.useImage) {
			image->color.a = progress;
		} else {
			image->color.a = 0.0f;
		}
		text->color.a = progress;
	} else if (frameTimer < (frame.fadein + frame.duration)) {
		// Stay the same I guess
		if (frame.useImage) {
			image->color.a = 1.0f;
		} else {
			image->color.a = 0.0f;
		}
		text->color.a = 1.0f;
	} else {
		// fade out
		f32 progress = clamp01((frameTimer - frame.fadein - frame.duration) / frame.fadeout);
		if (frame.useImage) {
			image->color.a = 1.0f - progress;
		} else {
			image->color.a = 0.0f;
		}
		text->color.a = 1.0f - progress;
	}
}

void CutsceneMenu::Draw(Rendering::DrawingContext &context) {
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

	azgui::Spacer *spacer = gui->system.CreateSpacer(screenListV, 1.0f);

	azgui::ListH *listBottom = gui->system.CreateListH(screenListV);
	listBottom->SetWidthFraction(1.0f);
	listBottom->SetHeightPixel(80.0f);
	listBottom->color = 0.0f;
	listBottom->colorHighlighted = 0.0f;
	listBottom->margin = 0.0f;

	buttonMenu = gui->system.CreateButton(listBottom);
	buttonMenu->SetWidthPixel(120.0f);
	buttonMenu->AddDefaultText(sys->ReadLocale("Menu"));
	buttonMenu->keycodeActivators = {KC_GP_BTN_START, KC_KEY_ESC};

	spacer = gui->system.CreateSpacer(listBottom, 1.0f);

	buttonReset = gui->system.CreateButton(listBottom);
	buttonReset->SetWidthPixel(120.0f);
	buttonReset->AddDefaultText(sys->ReadLocale("Reset"));
	buttonReset->keycodeActivators = {KC_GP_BTN_SELECT, KC_KEY_R};
}

void PlayMenu::Update() {
	screen->Update(vec2(0.0f), false);
	if (buttonMenu->state.Released()) {
		gui->menuNext = Gui::Menu::MAIN;
		sys->paused = true;
	} else {
		sys->paused = false;
	}
}

void PlayMenu::Draw(Rendering::DrawingContext &context) {
	gui->currentContext = &context;
	screen->Draw();
}

const u8 EditorMenu::blockTypes[5] = {
	Az2D::Entities::World::Block::BLOCK_PLAYER,
	Az2D::Entities::World::Block::BLOCK_WALL,
	Az2D::Entities::World::Block::BLOCK_WATER_TOP,
	Az2D::Entities::World::Block::BLOCK_GOAL,
	Az2D::Entities::World::Block::BLOCK_SPRINKLER,
};

void EditorMenu::Initialize() {
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
	listTop->SetHeightContents();
	listTop->margin = 0.0f;
	listTop->color = 0.0f;
	listTop->colorHighlighted = 0.0f;

	switchBlock = gui->system.CreateSwitchAsDefault(listTop);
	switchBlock->SetWidthPixel(128.0f);
	switchBlock->SetHeightContents();
	switchBlock->padding = 0.0f;
	switchBlock->selectable = false;
	switchBlock->inheritSelectable = false;

	const char *blockNames[] = {
		"Player",
		"Wall",
		"Water",
		"Beacon",
		"Sprinkler"
	};

	for (i32 i = 0; i < (i32)sizeof(blockTypes); i++) {
		azgui::Text *text = gui->system.CreateText(switchBlock);
		text->selectable = true;
		text->SetWidthFraction(1.0f);
		text->SetHeightPixel(28.0f);
		text->margin = 2.0f;
		text->fontSize = 24.0f;
		text->data = TextMetadata{Rendering::LEFT, Rendering::CENTER};
		text->string = sys->ReadLocale(blockNames[i]);
	}

	azgui::Spacer *spacer = gui->system.CreateSpacer(screenListV, 0.5f);

	azgui::ListH *listMiddle = gui->system.CreateListH(screenListV);
	listMiddle->SetHeightContents();
	listMiddle->SetWidthFraction(1.0f);
	listMiddle->margin = 0.0f;
	listMiddle->padding = 0.0f;
	listMiddle->color = 0.0f;
	listMiddle->colorHighlighted = 0.0f;
	listMiddle->occludes = false;

	spacer = gui->system.CreateSpacer(listMiddle, 0.5f);

	azgui::ListV *listDialogs = gui->system.CreateListV(listMiddle);
	listDialogs->SetWidthPixel(480.0f);
	listDialogs->SetHeightContents();
	listDialogs->color = 0.0f;
	listDialogs->colorHighlighted = 0.0f;
	listDialogs->margin = 0.0f;
	listDialogs->padding = 0.0f;
	listDialogs->occludes = false;

	spacer = gui->system.CreateSpacer(screenListV, 0.5f);

	azgui::ListH *listBottom = gui->system.CreateListH(screenListV);
	listBottom->SetHeightPixel(80.0f);
	listBottom->SetWidthFraction(1.0f);
	listBottom->color = 0.0f;
	listBottom->colorHighlighted = 0.0f;
	listBottom->margin = 0.0f;
	
	azgui::Button buttonTemplate;
	buttonTemplate.SetWidthPixel(120.0f);
	buttonTemplate.selectable = false;

	buttonMenu = gui->system.CreateButtonFrom(listBottom, buttonTemplate);
	buttonMenu->AddDefaultText(sys->ReadLocale("Menu"));
	buttonMenu->keycodeActivators = {KC_GP_BTN_START, KC_KEY_ESC};

	spacer = gui->system.CreateSpacer(listBottom, 1.0f);

	buttonNew = gui->system.CreateButtonFrom(listBottom, buttonTemplate);
	buttonNew->AddDefaultText(sys->ReadLocale("New"));
	buttonNew->keycodeActivators = {KC_GP_BTN_SELECT, KC_KEY_N};

	buttonLoad = gui->system.CreateButtonFrom(listBottom, buttonTemplate);
	buttonLoad->AddDefaultText(sys->ReadLocale("Load"));
	buttonLoad->keycodeActivators = {KC_GP_BTN_Y, KC_KEY_L};

	buttonSave = gui->system.CreateButtonFrom(listBottom, buttonTemplate);
	buttonSave->AddDefaultText(sys->ReadLocale("Save"));
	buttonSave->keycodeActivators = {KC_GP_BTN_X, KC_KEY_S};

	// Dialogs

	buttonCancel = gui->system.CreateButtonFrom(nullptr, buttonTemplate);
	buttonCancel->AddDefaultText(sys->ReadLocale("Cancel"));
	buttonCancel->keycodeActivators = {KC_GP_BTN_B};
	buttonConfirm = gui->system.CreateButtonFrom(nullptr, buttonTemplate);
	buttonConfirm->AddDefaultText(sys->ReadLocale("Confirm"));
	buttonConfirm->keycodeActivators = {};

	{ // Resize
		azgui::ListV *dialog = gui->system.CreateListV(nullptr);
		hideableResize = gui->system.CreateHideable(listDialogs, dialog);
		hideableResize->hidden = true;
		dialog->SetHeightContents();
		dialog->margin = 0.0f;
		dialog->padding = 0.0f;

		azgui::Text *header = gui->system.CreateText(dialog);
		header->bold = true;
		header->fontSize = 24.0f;
		header->string = sys->ReadLocale("ResizeText");

		azgui::ListH *textboxes = gui->system.CreateListH(dialog);
		textboxes->SetHeightPixel(48.0f);
		textboxes->padding = 0.0f;
		textboxes->color = 0.0f;
		textboxes->colorHighlighted = 0.0f;

		azgui::Text *widthText = gui->system.CreateText(textboxes);
		widthText->SetWidthFraction(0.5f);
		widthText->fontSize = 24.0f;
		widthText->string = sys->ReadLocale("Width:");

		textboxWidth = gui->system.CreateTextbox(textboxes);
		textboxWidth->SetWidthPixel(80.0f);
		textboxWidth->SetHeightFraction(1.0f);
		textboxWidth->fontSize = 24.0f;
		textboxWidth->data = TextMetadata{Rendering::RIGHT, Rendering::CENTER};
		textboxWidth->string = ToWString("64");
		textboxWidth->textFilter = azgui::TextFilterDigits;
		textboxWidth->textValidate = azgui::TextValidateNonempty;

		azgui::Text *heightText = gui->system.CreateText(textboxes);
		heightText->SetWidthFraction(0.5f);
		heightText->fontSize = 24.0f;
		heightText->string = sys->ReadLocale("Height:");

		textboxHeight = gui->system.CreateTextboxFrom(textboxes, *textboxWidth);
		textboxHeight->string = ToWString("32");

		azgui::ListH *buttons = gui->system.CreateListH(dialog);
		buttons->SetHeightPixel(80.0f);
		buttons->padding = 0.0f;
		buttons->color = 0.0f;
		buttons->colorHighlighted = 0.0f;

		gui->system.AddWidgetAsDefault(buttons, buttonCancel);
		spacer = gui->system.CreateSpacer(buttons, 1.0f);
		gui->system.AddWidget(buttons, buttonConfirm);
	}

	{ // Save
		azgui::ListV *dialog = gui->system.CreateListV(nullptr);
		hideableSave = gui->system.CreateHideable(listDialogs, dialog);
		hideableSave->hidden = true;
		dialog->SetHeightContents();
		dialog->margin = 0.0f;
		dialog->padding = 0.0f;

		azgui::Text *header = gui->system.CreateText(dialog);
		header->bold = true;
		header->fontSize = 24.0f;
		header->string = sys->ReadLocale("SaveText");

		textboxFilename = gui->system.CreateTextbox(dialog);
		textboxFilename->SetWidthFraction(1.0f);
		textboxFilename->SetHeightPixel(32.0f);
		textboxFilename->margin *= 2.0f;
		textboxFilename->fontSize = 24.0f;
		textboxFilename->data = TextMetadata{Rendering::CENTER, Rendering::CENTER};
		textboxFilename->string = ToWString("My Level");
		textboxFilename->textValidate = azgui::TextValidateNonempty;

		azgui::ListH *buttons = gui->system.CreateListH(dialog);
		buttons->SetHeightPixel(80.0f);
		buttons->padding = 0.0f;
		buttons->color = 0.0f;
		buttons->colorHighlighted = 0.0f;

		gui->system.AddWidgetAsDefault(buttons, buttonCancel);
		spacer = gui->system.CreateSpacer(buttons, 1.0f);
		gui->system.AddWidget(buttons, buttonConfirm);
	}

	{ // Load
		azgui::ListV *dialog = gui->system.CreateListV(nullptr);
		hideableLoad = gui->system.CreateHideable(listDialogs, dialog);
		hideableLoad->hidden = true;
		dialog->SetHeightContents();
		dialog->margin = 0.0f;
		dialog->padding = 0.0f;

		azgui::Text *header = gui->system.CreateText(dialog);
		header->bold = true;
		header->fontSize = 24.0f;
		header->string = sys->ReadLocale("LoadText");

		gui->system.AddWidget(dialog, textboxFilename);

		azgui::ListH *buttons = gui->system.CreateListH(dialog);
		buttons->SetHeightPixel(80.0f);
		buttons->padding = 0.0f;
		buttons->color = 0.0f;
		buttons->colorHighlighted = 0.0f;

		gui->system.AddWidgetAsDefault(buttons, buttonCancel);
		spacer = gui->system.CreateSpacer(buttons, 1.0f);
		gui->system.AddWidget(buttons, buttonConfirm);
	}
}

void EditorMenu::Update() {
	screen->Update(vec2(0.0f), true);

	if (buttonMenu->state.Released()) {
		gui->menuNext = Gui::Menu::MAIN;
		sys->paused = true;
	}
	if (buttonNew->state.Released()) {
		hideableResize->hidden = false;
		hideableSave->hidden = true;
		hideableLoad->hidden = true;
	}
	if (buttonSave->state.Released()) {
		hideableSave->hidden = false;
		hideableResize->hidden = true;
		hideableLoad->hidden = true;
	}
	if (buttonLoad->state.Released()) {
		hideableLoad->hidden = false;
		hideableSave->hidden = true;
		hideableResize->hidden = true;
	}
	if (buttonCancel->state.Released()) {
		buttonCancel->state.Set(false, false, false);
		hideableResize->hidden = true;
		hideableLoad->hidden = true;
		hideableSave->hidden = true;
	}
	if (buttonConfirm->state.Released()) {
		buttonConfirm->state.Set(false, false, false);
		bool succeeded = false;
		if (!hideableResize->hidden) {
			if (textboxWidth->textValidate(textboxWidth->string) && textboxHeight->textValidate(textboxHeight->string)) {
				i32 width = (i32)WStringToU64(textboxWidth->string);
				i32 height = (i32)WStringToU64(textboxHeight->string);
				entities->world.Resize(vec2i(width, height));
				entities->camPos = vec2(entities->world.size) * 16.0f;
				succeeded = true;
			}
		} else if (!hideableSave->hidden) {
			if (textboxFilename->textValidate(textboxFilename->string)) {
				String filename(textboxFilename->string.size);
				for (i32 i = 0; i < filename.size; i++) {
					filename[i] = textboxFilename->string[i];
				}
				succeeded = entities->world.Save(filename);
			}
		} else if (!hideableLoad->hidden) {
			if (textboxFilename->textValidate(textboxFilename->string)) {
				String filename(textboxFilename->string.size);
				for (i32 i = 0; i < filename.size; i++) {
					filename[i] = textboxFilename->string[i];
				}
				succeeded = entities->world.Load(filename);
			}
		}
		if (succeeded) {
			hideableResize->hidden = true;
			hideableLoad->hidden = true;
			hideableSave->hidden = true;
		}
	}
}

void EditorMenu::Draw(Rendering::DrawingContext &context) {
	gui->currentContext = &context;
	screen->Draw();
}

} // namespace Az2D::Gui

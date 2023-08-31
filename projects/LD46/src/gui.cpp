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
	menuMain.Initialize();
	menuSettings.Initialize();
	menuPlay.Initialize();
	menuEditor.Initialize();
	menuCutscene.Initialize();
}

void Gui::EventSync() {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Gui::Gui::EventSync)
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
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Gui::Gui::EventDraw)
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
	title->string = sys->ReadLocale("Torch Runner");
	AddWidget(listV, title);

	spacer = new Widget();
	spacer->size.y = 0.4f;
	AddWidget(listV, spacer);

	ListV *buttonList = new ListV();
	buttonList->fractionWidth = false;
	buttonList->size = vec2(500.0f, 0.0f);
	buttonList->padding = vec2(16.0f);

	buttonContinue = new Button();
	buttonContinue->AddDefaultText(sys->ReadLocale("Continue"));
	buttonContinue->size.y = 64.0f;
	buttonContinue->fractionHeight = false;
	buttonContinue->margin = vec2(16.0f);

	continueHideable = new Hideable(buttonContinue);
	continueHideable->hidden = true;
	AddWidget(buttonList, continueHideable);

	buttonNewGame = new Button();
	buttonNewGame->AddDefaultText(sys->ReadLocale("New Game"));
	buttonNewGame->size.y = 64.0f;
	buttonNewGame->fractionHeight = false;
	buttonNewGame->margin = vec2(16.0f);
	AddWidget(buttonList, buttonNewGame);

	buttonLevelEditor = new Button();
	buttonLevelEditor->AddDefaultText(sys->ReadLocale("Level Editor"));
	buttonLevelEditor->size.y = 64.0f;
	buttonLevelEditor->fractionHeight = false;
	buttonLevelEditor->margin = vec2(16.0f);
	AddWidget(buttonList, buttonLevelEditor);

	buttonSettings = new Button();
	buttonSettings->AddDefaultText(sys->ReadLocale("Settings"));
	buttonSettings->size.y = 64.0f;
	buttonSettings->fractionHeight = false;
	buttonSettings->margin = vec2(16.0f);
	AddWidget(buttonList, buttonSettings);

	buttonExit = new Button();
	buttonExit->AddDefaultText(sys->ReadLocale("Exit"));
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
	title->fontIndex = guiBasic->fontIndex;
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
	settingTextTemplate->fontIndex = guiBasic->fontIndex;
	settingTextTemplate->fontSize = 20.0f;
	settingTextTemplate->fractionHeight = true;
	settingTextTemplate->size.y = 1.0f;
	settingTextTemplate->alignV = Rendering::CENTER;

	checkFullscreen = new Checkbox();
	checkFullscreen->checked = Settings::ReadBool(Settings::sFullscreen);

	checkVSync = new Checkbox();
	checkVSync->checked = Settings::ReadBool(Settings::sVSync);

	TextBox *textboxTemplate = new TextBox();
	textboxTemplate->fontIndex = guiBasic->fontIndex;
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
				// Allowing us to use the keyboard and gamepad to control the slider instead
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
	buttonBack->AddDefaultText(sys->ReadLocale("Back"));
	buttonBack->size.x = 1.0f / 2.0f;
	buttonBack->size.y = 64.0f;
	buttonBack->fractionHeight = false;
	buttonBack->margin = vec2(8.0f);
	buttonBack->highlightBG = vec4(colorBack, 0.9f);
	buttonBack->keycodeActivators = {KC_GP_BTN_B, KC_KEY_ESC};
	AddWidget(buttonList, buttonBack);

	buttonApply = new Button();
	buttonApply->AddDefaultText(sys->ReadLocale("Apply"));
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
		gui->menuNext = Gui::Menu::MAIN;
	}
}

void SettingsMenu::Draw(Rendering::DrawingContext &context) {
	screen.Draw(context);
}

inline String FloatToString(f32 in) {
	return ToString(in, 10, 2);
}

void CutsceneMenu::Initialize() {
	ListH *screenListH = new ListH();
	screenListH->margin = 0.0f;
	screenListH->padding = 0.0f;
	screenListH->color = vec4(vec3(0.0f), 1.0f);
	screenListH->highlight = screenListH->color;
	AddWidget(&screen, screenListH);

	Widget *spacer = new Widget();
	spacer->size.x = 0.5f;
	AddWidget(screenListH, spacer);

	ListV *listV = new ListV();
	listV->margin = 0.0f;
	listV->padding = 0.0f;
	listV->color = 0.0f;
	listV->highlight = 0.0f;
	listV->size.x = 0.0f;
	listV->fractionWidth = false;
	AddWidget(screenListH, listV);

	spacer = new Widget();
	spacer->size.y = 0.5f;
	AddWidget(listV, spacer);

	image = new Image();
	image->size = vec2(416.0f, 416.0f);
	image->fractionWidth = false;
	image->fractionHeight = false;
	image->margin = vec2(224.0f, 32.0f);
	AddWidget(listV, image);

	text = new Text();
	text->alignH = Rendering::CENTER;
	text->alignV = Rendering::CENTER;
	text->size = vec2(800.0f, 100.0f);
	text->fractionWidth = false;
	text->fractionHeight = false;
	text->margin = 32.0f;
	text->string = sys->ReadLocale("This is the intro cutscene!");
	AddWidget(listV, text);

	buttonSkip = new Button();
	buttonSkip->fractionHeight = false;
	buttonSkip->fractionWidth = false;
	buttonSkip->size = vec2(128.0f, 64.0f);
	buttonSkip->margin.x = 368.0f;
	buttonSkip->AddDefaultText(sys->ReadLocale("Skip"));
	buttonSkip->keycodeActivators = {KC_KEY_ESC, KC_KEY_SPACE};
	buttonSkip->colorBG = 0.0f;
	AddWidget(listV, buttonSkip);

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
	screen.Update(vec2(0.0f), true);
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
		image->texIndex = frame.image;
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
	AddWidgetAsDefault(screenListV, listBottom);

	buttonMenu = new Button();
	buttonMenu->fractionWidth = false;
	buttonMenu->size.x = 120.0f;
	buttonMenu->AddDefaultText(sys->ReadLocale("Menu"));
	buttonMenu->keycodeActivators = {KC_GP_BTN_START, KC_KEY_ESC};
	AddWidgetAsDefault(listBottom, buttonMenu);

	spacer = new Widget();
	spacer->fractionWidth = true;
	spacer->size.x = 1.0f;
	AddWidget(listBottom, spacer);

	buttonReset = new Button(*buttonMenu);
	buttonReset->children.Clear();
	buttonReset->AddDefaultText(sys->ReadLocale("Reset"));
	buttonReset->keycodeActivators = {KC_GP_BTN_SELECT, KC_KEY_R};
	AddWidget(listBottom, buttonReset);
}

void PlayMenu::Update() {
	screen.Update(vec2(0.0f), false);
	if (buttonMenu->state.Released()) {
		gui->menuNext = Gui::Menu::MAIN;
		sys->paused = true;
	} else {
		sys->paused = false;
	}
}

void PlayMenu::Draw(Rendering::DrawingContext &context) {
	screen.Draw(context);
}

const u8 EditorMenu::blockTypes[5] = {
	Az2D::Entities::World::Block::BLOCK_PLAYER,
	Az2D::Entities::World::Block::BLOCK_WALL,
	Az2D::Entities::World::Block::BLOCK_WATER_TOP,
	Az2D::Entities::World::Block::BLOCK_GOAL,
	Az2D::Entities::World::Block::BLOCK_SPRINKLER,
};

void EditorMenu::Initialize() {
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
	listTop->size = vec2(1.0f, 0.0f);
	AddWidget(screenListV, listTop);

	switchBlock = new Switch();
	switchBlock->fractionWidth = false;
	switchBlock->size.x = 128.0f;
	switchBlock->size.y = 0.0f;
	switchBlock->padding = 0.0f;

	const char *blockNames[] = {
		"Player",
		"Wall",
		"Water",
		"Beacon",
		"Sprinkler"
	};

	for (i32 i = 0; i < (i32)sizeof(blockTypes); i++) {
		Text *text = new Text();
		text->selectable = true;
		text->size.x = 1.0f;
		text->fractionHeight = false;
		text->size.y = 28.0f;
		text->margin = 2.0f;
		text->fontIndex = guiBasic->fontIndex;
		text->fontSize = 24.0f;
		text->alignV = Rendering::CENTER;
		text->string = sys->ReadLocale(blockNames[i]);
		AddWidget(switchBlock, text);
	}
	AddWidgetAsDefault(listTop, switchBlock);

	Widget *spacer = new Widget();
	spacer->size.y = 0.5f;
	AddWidget(screenListV, spacer);

	ListH *listMiddle = new ListH();
	listMiddle->fractionHeight = false;
	listMiddle->fractionWidth = true;
	listMiddle->margin = 0.0f;
	listMiddle->padding = 0.0f;
	listMiddle->color = 0.0f;
	listMiddle->highlight = 0.0f;
	listMiddle->size = vec2(1.0f, 0.0f);
	listMiddle->occludes = false;
	AddWidget(screenListV, listMiddle);

	spacer = new Widget();
	spacer->size.x = 0.5f;
	spacer->size.y = 0.0f;
	spacer->margin = 0.0f;
	AddWidget(listMiddle, spacer);

	ListV *listDialogs = new ListV();
	listDialogs->fractionWidth = false;
	listDialogs->fractionHeight = false;
	listDialogs->size = vec2(480.0f, 0.0f);
	listDialogs->color = 0.0f;
	listDialogs->margin = 0.0f;
	listDialogs->padding = 0.0f;
	listDialogs->highlight = 0.0f;
	listDialogs->occludes = false;
	AddWidget(listMiddle, listDialogs);

	spacer = new Widget();
	spacer->size.y = 0.5f;
	AddWidget(screenListV, spacer);

	ListH *listBottom = new ListH();
	listBottom->fractionHeight = false;
	listBottom->fractionWidth = true;
	listBottom->color = 0.0f;
	listBottom->highlight = 0.0f;
	listBottom->margin = 0.0f;
	listBottom->size = vec2(1.0f, 80.0f);
	AddWidgetAsDefault(screenListV, listBottom);

	buttonMenu = new Button();
	buttonMenu->fractionWidth = false;
	buttonMenu->size.x = 120.0f;
	buttonMenu->AddDefaultText(sys->ReadLocale("Menu"));
	buttonMenu->keycodeActivators = {KC_GP_BTN_START, KC_KEY_ESC};
	AddWidgetAsDefault(listBottom, buttonMenu);

	spacer = new Widget();
	spacer->size.x = 1.0f;
	AddWidget(listBottom, spacer);

	buttonNew = new Button(*buttonMenu);
	buttonNew->children.Clear();
	buttonNew->AddDefaultText(sys->ReadLocale("New"));
	buttonNew->keycodeActivators = {KC_GP_BTN_SELECT, KC_KEY_N};
	AddWidget(listBottom, buttonNew);

	buttonLoad = new Button(*buttonMenu);
	buttonLoad->children.Clear();
	buttonLoad->AddDefaultText(sys->ReadLocale("Load"));
	buttonLoad->keycodeActivators = {KC_GP_BTN_Y, KC_KEY_L};
	AddWidget(listBottom, buttonLoad);

	buttonSave = new Button(*buttonMenu);
	buttonSave->children.Clear();
	buttonSave->AddDefaultText(sys->ReadLocale("Save"));
	buttonSave->keycodeActivators = {KC_GP_BTN_X, KC_KEY_S};
	AddWidget(listBottom, buttonSave);

	// Dialogs

	buttonCancel = new Button(*buttonMenu);
	buttonCancel->children.Clear();
	buttonCancel->AddDefaultText(sys->ReadLocale("Cancel"));
	buttonCancel->keycodeActivators = {KC_GP_BTN_B};
	buttonConfirm = new Button(*buttonMenu);
	buttonConfirm->children.Clear();
	buttonConfirm->AddDefaultText(sys->ReadLocale("Confirm"));
	buttonConfirm->keycodeActivators = {};

	{ // Resize
		ListV *dialog = new ListV();
		dialog->fractionHeight = false;
		dialog->size.y = 0.0f;
		dialog->margin = 0.0f;
		dialog->padding = 0.0f;

		Text *header = new Text();
		header->bold = true;
		header->fontIndex = guiBasic->fontIndex;
		header->fontSize = 24.0f;
		header->string = sys->ReadLocale("ResizeText");
		AddWidget(dialog, header);

		ListH *textboxes = new ListH();
		textboxes->fractionHeight = false;
		textboxes->size.y = 48.0f;
		textboxes->padding = 0.0f;
		textboxes->color = 0.0f;
		textboxes->highlight = 0.0f;
		AddWidgetAsDefault(dialog, textboxes);

		Text *widthText = new Text();
		widthText->size.x = 0.5f;
		widthText->fontIndex = guiBasic->fontIndex;
		widthText->fontSize = 24.0f;
		widthText->string = sys->ReadLocale("Width:");
		AddWidget(textboxes, widthText);

		textboxWidth = new TextBox();
		textboxWidth->fractionHeight = true;
		textboxWidth->size.x = 80.0f;
		textboxWidth->size.y = 1.0f;
		textboxWidth->fontIndex = guiBasic->fontIndex;
		textboxWidth->fontSize = 24.0f;
		textboxWidth->alignH = Rendering::RIGHT;
		textboxWidth->string = ToWString("64");
		textboxWidth->textFilter = TextFilterDigits;
		textboxWidth->textValidate = TextValidateNonempty;
		AddWidget(textboxes, textboxWidth);

		Text *heightText = new Text();
		heightText->size.x = 0.5f;
		heightText->fontIndex = guiBasic->fontIndex;
		heightText->fontSize = 24.0f;
		heightText->string = sys->ReadLocale("Height:");
		AddWidget(textboxes, heightText);

		textboxHeight = new TextBox(*textboxWidth);
		textboxHeight->string = ToWString("32");
		AddWidget(textboxes, textboxHeight);

		ListH *buttons = new ListH();
		buttons->fractionHeight = false;
		buttons->size.y = 80.0f;
		buttons->padding = 0.0f;
		buttons->color = 0.0f;
		buttons->highlight = 0.0f;
		AddWidget(dialog, buttons);

		AddWidgetAsDefault(buttons, buttonCancel);
		spacer = new Widget();
		AddWidget(buttons, spacer);
		AddWidget(buttons, buttonConfirm);
		hideableResize = new Hideable(dialog);
		hideableResize->hidden = true;

		AddWidget(listDialogs, hideableResize);
	}

	{ // Save
		ListV *dialog = new ListV();
		dialog->fractionHeight = false;
		dialog->size.y = 0.0f;
		dialog->margin = 0.0f;
		dialog->padding = 0.0f;

		Text *header = new Text();
		header->bold = true;
		header->fontIndex = guiBasic->fontIndex;
		header->fontSize = 24.0f;
		header->string = sys->ReadLocale("SaveText");
		AddWidget(dialog, header);

		textboxFilename = new TextBox();
		textboxFilename->fractionWidth = true;
		textboxFilename->fractionHeight = false;
		textboxFilename->size.x = 1.0f;
		textboxFilename->size.y = 32.0f;
		textboxFilename->margin *= 2.0f;
		textboxFilename->fontIndex = guiBasic->fontIndex;
		textboxFilename->fontSize = 24.0f;
		textboxFilename->alignH = Rendering::CENTER;
		textboxFilename->string = ToWString("My Level");
		textboxFilename->textValidate = TextValidateNonempty;
		AddWidget(dialog, textboxFilename);

		ListH *buttons = new ListH();
		buttons->fractionHeight = false;
		buttons->size.y = 80.0f;
		buttons->padding = 0.0f;
		buttons->color = 0.0f;
		buttons->highlight = 0.0f;
		AddWidget(dialog, buttons);

		AddWidgetAsDefault(buttons, buttonCancel);
		spacer = new Widget();
		AddWidget(buttons, spacer);
		AddWidget(buttons, buttonConfirm);
		hideableSave = new Hideable(dialog);
		hideableSave->hidden = true;

		AddWidget(listDialogs, hideableSave);
	}

	{ // Load
		ListV *dialog = new ListV();
		dialog->fractionHeight = false;
		dialog->size.y = 0.0f;
		dialog->margin = 0.0f;
		dialog->padding = 0.0f;

		Text *header = new Text();
		header->bold = true;
		header->fontIndex = guiBasic->fontIndex;
		header->fontSize = 24.0f;
		header->string = sys->ReadLocale("LoadText");
		AddWidget(dialog, header);

		AddWidget(dialog, textboxFilename);

		ListH *buttons = new ListH();
		buttons->fractionHeight = false;
		buttons->size.y = 80.0f;
		buttons->padding = 0.0f;
		buttons->color = 0.0f;
		buttons->highlight = 0.0f;
		AddWidget(dialog, buttons);

		AddWidgetAsDefault(buttons, buttonCancel);
		spacer = new Widget();
		AddWidget(buttons, spacer);
		AddWidget(buttons, buttonConfirm);
		hideableLoad = new Hideable(dialog);
		hideableLoad->hidden = true;

		AddWidget(listDialogs, hideableLoad);
	}
}

void EditorMenu::Update() {
	screen.Update(vec2(0.0f), true);

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
	screen.Draw(context);
}

} // namespace Az2D::Gui

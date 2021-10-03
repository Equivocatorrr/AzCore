/*
	File: gui.cpp
	Author: Philip Haynes
*/

#include "gui.hpp"
#include "globals.hpp"

namespace Int {

const vec3 colorBack = {1.0f, 0.4f, 0.1f};
const vec3 colorHighlightLow = {0.2f, 0.45f, 0.5f};
const vec3 colorHighlightMedium = {0.4f, 0.9f, 1.0f};
const vec3 colorHighlightHigh = {0.9f, 0.98f, 1.0f};

Gui::~Gui() {
	for (Widget* widget : allWidgets) {
		delete widget;
	}
}

void Gui::EventAssetInit() {
	globals->assets.QueueFile("DroidSans.ttf");
	// globals->assets.QueueFile("FiraSans-Book.otf");
	// globals->assets.QueueFile("LiberationSerif-Regular.ttf");
	// globals->assets.QueueFile("OpenSans-Regular.ttf");
	// globals->assets.QueueFile("Literata[wght].ttf");
	globals->assets.QueueFile("click in 1.ogg");
	globals->assets.QueueFile("click in 2.ogg");
	globals->assets.QueueFile("click in 3.ogg");
	globals->assets.QueueFile("click in 4.ogg");
	globals->assets.QueueFile("click out 1.ogg");
	globals->assets.QueueFile("click out 2.ogg");
	globals->assets.QueueFile("click out 3.ogg");
	globals->assets.QueueFile("click out 4.ogg");
	globals->assets.QueueFile("click soft 1.ogg");
	globals->assets.QueueFile("click soft 2.ogg");
	globals->assets.QueueFile("Pop High.ogg");
	globals->assets.QueueFile("Pop Low.ogg");
	globals->assets.QueueFile("Cursor.png");

	globals->assets.QueueFile("beep short.ogg");
	globals->assets.QueueFile("dramatic beep.ogg");
	globals->assets.QueueFile("phone buzz.ogg");

	globals->assets.QueueFile("Intro/Intro-1.png");
	globals->assets.QueueFile("Intro/Intro-2.png");
	globals->assets.QueueFile("Intro/Intro-3.png");
	globals->assets.QueueFile("Intro/Intro-4.png");
	globals->assets.QueueFile("Intro/Intro-5.png");

	globals->assets.QueueFile("Outro/Outro1.png");
	globals->assets.QueueFile("Outro/Outro2.png");
	globals->assets.QueueFile("Outro/Outro3.png");
	globals->assets.QueueFile("Outro/Outro4.png");
	globals->assets.QueueFile("Outro/Outro5.png");
	globals->assets.QueueFile("Outro/Outro6.png");

	globals->assets.QueueFile("Outro/Credits-Equivocator.png");
	globals->assets.QueueFile("Outro/Credits-Flubz.png");
}

void Gui::EventAssetAcquire() {
	fontIndex = globals->assets.FindMapping("DroidSans.ttf");
	// fontIndex = globals->assets.FindMapping("FiraSans-Book.otf");
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
		sndClickInSources[i].SetGain(0.15f);
		sndClickInSources[i].SetPitch(1.2f);
		sndClickOutSources[i].SetGain(0.15f);
		sndClickOutSources[i].SetPitch(1.2f);
	}
	sndClickSoftSources[0].Create("click soft 1.ogg");
	sndClickSoftSources[1].Create("click soft 2.ogg");
	sndClickSoftSources[0].SetGain(0.01f);
	sndClickSoftSources[1].SetGain(0.01f);
	sndClickSoftSources[0].SetPitch(1.2f);
	sndClickSoftSources[1].SetPitch(1.2f);
	sndClickSoft.sources = {
		&sndClickSoftSources[0],
		&sndClickSoftSources[1]
	};
	sndPopHigh.Create("Pop High.ogg");
	sndPopLow.Create("Pop Low.ogg");
	sndPopHigh.SetGain(0.1f);
	sndPopLow.SetGain(0.1f);
	font = &globals->assets.fonts[fontIndex];

	cursorIndex = globals->assets.FindMapping("Cursor.png");

	introIndex[0] = globals->assets.FindMapping("Intro/Intro-1.png");
	introIndex[1] = globals->assets.FindMapping("Intro/Intro-2.png");
	introIndex[2] = globals->assets.FindMapping("Intro/Intro-3.png");
	introIndex[3] = globals->assets.FindMapping("Intro/Intro-4.png");
	introIndex[4] = globals->assets.FindMapping("Intro/Intro-5.png");

	outtroIndex[0] = globals->assets.FindMapping("Outro/Outro1.png");
	outtroIndex[1] = globals->assets.FindMapping("Outro/Outro2.png");
	outtroIndex[2] = globals->assets.FindMapping("Outro/Outro3.png");
	outtroIndex[3] = globals->assets.FindMapping("Outro/Outro4.png");
	outtroIndex[4] = globals->assets.FindMapping("Outro/Outro5.png");
	outtroIndex[5] = globals->assets.FindMapping("Outro/Outro6.png");

	creditsEquivocator = globals->assets.FindMapping("Outro/Credits-Equivocator.png");
	creditsFlubz = globals->assets.FindMapping("Outro/Credits-Flubz.png");

	sndBeepShort.Create("beep short.ogg");
	sndBeepShort.SetGain(0.5f);
	sndBeepLong.Create("dramatic beep.ogg");
	sndBeepLong.SetGain(0.5f);
	sndPhoneBuzz.Create("phone buzz.ogg");
	sndPhoneBuzz.SetGain(0.5f);
}

void Gui::EventInitialize() {
	mainMenu.Initialize();
	settingsMenu.Initialize();
	playMenu.Initialize();
	editorMenu.Initialize();
	cutsceneMenu.Initialize();
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
	case MENU_EDITOR:
		editorMenu.Update();
		break;
	case MENU_INTRO:
		cutsceneMenu.intro = true;
		cutsceneMenu.Update();
		break;
	case MENU_OUTTRO:
		cutsceneMenu.intro = false;
		cutsceneMenu.Update();
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
	case MENU_EDITOR:
		editorMenu.Draw(contexts.Back());
		break;
	case MENU_INTRO:
	case MENU_OUTTRO:
		cutsceneMenu.Draw(contexts.Back());
		break;
	}
	if (usingMouse) {
		globals->rendering.DrawQuad(contexts.Back(), cursorIndex, vec4(1.0f), globals->input.cursor, vec2(32.0f * scale), vec2(1.0f), vec2(0.5f));
	}
}

void AddWidget(Widget *parent, Widget *newWidget, bool deeper = false) {
	newWidget->depth = parent->depth + (deeper ? 1 : 0);
	if (newWidget->selectable) {
		parent->selectable = true;
	}
	parent->children.Append(newWidget);
	if (!globals->gui.allWidgets.Exists(newWidget)) {
		globals->gui.allWidgets.Emplace(newWidget);
	}
}

void AddWidget(Widget *parent, Switch *newWidget) {
	newWidget->depth = parent->depth + 1;
	newWidget->parentDepth = parent->depth;
	if (newWidget->selectable) {
		parent->selectable = true;
	}
	parent->children.Append(newWidget);
	if (!globals->gui.allWidgets.Exists(newWidget)) {
		globals->gui.allWidgets.Emplace(newWidget);
	}
}

void AddWidgetAsDefault(List *parent, Widget *newWidget, bool deeper = false) {
	newWidget->depth = parent->depth + (deeper ? 1 : 0);
	if (newWidget->selectable) {
		parent->selectable = true;
	}
	parent->selectionDefault = parent->children.size;
	parent->children.Append(newWidget);
	if (!globals->gui.allWidgets.Exists(newWidget)) {
		globals->gui.allWidgets.Emplace(newWidget);
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
	if (!globals->gui.allWidgets.Exists(newWidget)) {
		globals->gui.allWidgets.Emplace(newWidget);
	}
}

//
//	  Menu implementations
//

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
	title->fontIndex = globals->gui.fontIndex;
	title->string = globals->ReadLocale("Torch Runner");
	AddWidget(listV, title);

	spacer = new Widget();
	spacer->size.y = 0.4f;
	AddWidget(listV, spacer);

	ListV *buttonList = new ListV();
	buttonList->fractionWidth = false;
	buttonList->size = vec2(500.0f, 0.0f);
	buttonList->padding = vec2(16.0f);

	buttonContinue = new Button();
	buttonContinue->string = globals->ReadLocale("Continue");
	buttonContinue->size.y = 64.0f;
	buttonContinue->fractionHeight = false;
	buttonContinue->margin = vec2(16.0f);

	continueHideable = new Hideable(buttonContinue);
	continueHideable->hidden = true;
	AddWidget(buttonList, continueHideable);

	buttonNewGame = new Button();
	buttonNewGame->string = globals->ReadLocale("New Game");
	buttonNewGame->size.y = 64.0f;
	buttonNewGame->fractionHeight = false;
	buttonNewGame->margin = vec2(16.0f);
	AddWidget(buttonList, buttonNewGame);

	buttonLevelEditor = new Button();
	buttonLevelEditor->string = globals->ReadLocale("Level Editor");
	buttonLevelEditor->size.y = 64.0f;
	buttonLevelEditor->fractionHeight = false;
	buttonLevelEditor->margin = vec2(16.0f);
	AddWidget(buttonList, buttonLevelEditor);

	buttonSettings = new Button();
	buttonSettings->string = globals->ReadLocale("Settings");
	buttonSettings->size.y = 64.0f;
	buttonSettings->fractionHeight = false;
	buttonSettings->margin = vec2(16.0f);
	AddWidget(buttonList, buttonSettings);

	buttonExit = new Button();
	buttonExit->string = globals->ReadLocale("Exit");
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
		globals->gui.nextMenu = MENU_PLAY;
	}
	if (buttonNewGame->state.Released()) {
		globals->gui.nextMenu = MENU_INTRO;
		globals->gui.cutsceneMenu.intro = true;
		globals->gui.cutsceneMenu.Begin();
		continueHideable->hidden = false;
	}
	if (buttonLevelEditor->state.Released()) {
		globals->gui.nextMenu = MENU_EDITOR;
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
	title->fontIndex = globals->gui.fontIndex;
	title->string = globals->ReadLocale("Settings");
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
	settingTextTemplate->fontIndex = globals->gui.fontIndex;
	settingTextTemplate->fontSize = 20.0f;
	settingTextTemplate->fractionHeight = true;
	settingTextTemplate->size.y = 1.0f;
	settingTextTemplate->alignV = Rendering::CENTER;

	checkFullscreen = new Checkbox();
	checkFullscreen->checked = globals->fullscreen;

	TextBox *textboxTemplate = new TextBox();
	textboxTemplate->fontIndex = globals->gui.fontIndex;
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
	textboxFramerate->string = ToWString(ToString((i32)globals->framerate));


	for (i32 i = 0; i < 3; i++) {
		textboxVolumes[i] = new TextBox(*textboxTemplate);
		sliderVolumes[i] = new Slider(*sliderTemplate);
		textboxVolumes[i]->textFilter = TextFilterDecimalsPositive;
		textboxVolumes[i]->textValidate = TextValidateDecimalsPositive;
		sliderVolumes[i]->mirror = textboxVolumes[i];
	}
	textboxVolumes[0]->string = ToWString(ToString(globals->volumeMain*100.0f, 10, 1));
	textboxVolumes[1]->string = ToWString(ToString(globals->volumeMusic*100.0f, 10, 1));
	textboxVolumes[2]->string = ToWString(ToString(globals->volumeEffects*100.0f, 10, 1));
	sliderVolumes[0]->value = globals->volumeMain*100.0f;
	sliderVolumes[1]->value = globals->volumeMusic*100.0f;
	sliderVolumes[2]->value = globals->volumeEffects*100.0f;

	ListH *settingListTemplate = new ListH();
	settingListTemplate->size.y = 0.0f;
	settingListTemplate->margin = vec2(8.0f);
	settingListTemplate->padding = vec2(0.0f);

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
			settingText->fontSize = 24.0f;
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
	buttonList->size.y = 0.0f;
	buttonList->margin = vec2(0.0f);
	buttonList->padding = vec2(0.0f);
	buttonList->color = vec4(0.0f);
	buttonList->highlight = vec4(0.0f);

	buttonBack = new Button();
	buttonBack->string = globals->ReadLocale("Back");
	buttonBack->size.x = 1.0f / 2.0f;
	buttonBack->size.y = 64.0f;
	buttonBack->fractionHeight = false;
	buttonBack->margin = vec2(8.0f);
	buttonBack->highlightBG = vec4(colorBack, 0.9f);
	buttonBack->keycodeActivators = {KC_GP_BTN_B, KC_KEY_ESC};
	AddWidget(buttonList, buttonBack);

	buttonApply = new Button();
	buttonApply->string = globals->ReadLocale("Apply");
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
	screen.Update(vec2(0.0f), true);
	if (buttonApply->state.Released()) {
		globals->window.Fullscreen(checkFullscreen->checked);
		globals->fullscreen = checkFullscreen->checked;
		u64 framerate = 60;
		if (textboxFramerate->textValidate(textboxFramerate->string)) {
			framerate = clamp(WStringToU64(textboxFramerate->string), (u64)30, (u64)300);
			globals->Framerate((f32)framerate);
		}
		textboxFramerate->string = ToWString(ToString(framerate));
		globals->volumeMain = sliderVolumes[0]->value / 100.0f;
		globals->volumeMusic = sliderVolumes[1]->value / 100.0f;
		globals->volumeEffects = sliderVolumes[2]->value / 100.0f;
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
	text->string = globals->ReadLocale("This is the intro cutscene!");
	AddWidget(listV, text);

	buttonSkip = new Button();
	buttonSkip->fractionHeight = false;
	buttonSkip->fractionWidth = false;
	buttonSkip->size = vec2(128.0f, 64.0f);
	buttonSkip->margin.x = 368.0f;
	buttonSkip->string = globals->ReadLocale("Skip");
	buttonSkip->keycodeActivators = {KC_KEY_ESC, KC_KEY_SPACE};
	buttonSkip->colorBG = 0.0f;
	AddWidget(listV, buttonSkip);

	introFrames = {
		{"", &globals->gui.sndBeepShort, 0.5f, 0.5f, 0.0f, globals->gui.introIndex[0], true},
		{"", &globals->gui.sndBeepShort, 0.0f, 1.0f, 0.0f, globals->gui.introIndex[1], true},
		{"", &globals->gui.sndBeepShort, 0.0f, 1.0f, 0.0f, globals->gui.introIndex[2], true},
		{"", &globals->gui.sndBeepLong, 0.0f, 1.0f, 1.0f, globals->gui.introIndex[3], true},
		{"It is time.", nullptr, 1.0f, 1.0f, 1.0f, 0, false},
		{"Light the beacon.", nullptr, 0.5f, 2.0f, 0.5f, globals->gui.introIndex[4], true}
	};

	outtroFrames = {
		{"", nullptr, 0.5f, 1.0f, 0.0f, globals->gui.outtroIndex[0], true},
		{"", &globals->gui.sndPhoneBuzz, 0.0f, 1.0f, 0.5f, globals->gui.outtroIndex[1], true},
		{"", nullptr, 0.5f, 1.5f, 0.0f, globals->gui.outtroIndex[2], true},
		{"h- Huh?", nullptr, 0.0f, 2.0f, 0.0f, globals->gui.outtroIndex[3], true},
		{"what's-", nullptr, 0.0f, 2.0f, 0.0f, globals->gui.outtroIndex[4], true},
		{"Oh #&^$ ma P I Z Z A", nullptr, 0.0f, 2.0f, 1.0f, globals->gui.outtroIndex[5], true},
		{"", nullptr, 0.2f, 0.1f, 0.2f, 0, false},
		{"Programming and Sound:\nEquivocator", &globals->entities.jump1Sources[0], 0.5f, 2.0f, 0.5f, globals->gui.creditsEquivocator, true},
		{"Art and Sound:\nFlubz", &globals->entities.jump2Sources[0], 0.5f, 2.0f, 0.5f, globals->gui.creditsFlubz, true},
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
		globals->gui.nextMenu = intro ? MENU_PLAY : MENU_MAIN;
		if (!intro) {
			globals->entities.Reset();
			globals->entities.camPos = -1.0f;
			globals->entities.camZoom = 10000.0f;
		}
		return;
	}
	const Frame &frame = frames[currentFrame];
	if (frameTimer == 0.0f) {
		image->texIndex = frame.image;
		text->string = globals->ReadLocale(frame.text);
		if (frame.sound) {
			frame.sound->Play();
		}
	}
	frameTimer += globals->objects.timestep;
	if (frameTimer >= (frame.fadein + frame.duration + frame.fadeout)) {
		currentFrame++;
		frameTimer = 0.0f;
	} else if (frameTimer < frame.fadein) {
		// Do the fade
		f32 progress = clamp(frameTimer / frame.fadein, 0.0f, 1.0f);
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
		f32 progress = clamp((frameTimer - frame.fadein - frame.duration) / frame.fadeout, 0.0f, 1.0f);
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
	buttonMenu->fontIndex = globals->gui.fontIndex;
	buttonMenu->fractionWidth = false;
	buttonMenu->size.x = 120.0f;
	buttonMenu->string = globals->ReadLocale("Menu");
	buttonMenu->keycodeActivators = {KC_GP_BTN_START, KC_KEY_ESC};
	AddWidgetAsDefault(listBottom, buttonMenu);

	spacer = new Widget();
	spacer->fractionWidth = true;
	spacer->size.x = 1.0f;
	AddWidget(listBottom, spacer);

	buttonReset = new Button(*buttonMenu);
	buttonReset->string = globals->ReadLocale("Reset");
	buttonReset->keycodeActivators = {KC_GP_BTN_SELECT, KC_KEY_R};
	AddWidget(listBottom, buttonReset);
}

void PlayMenu::Update() {
	screen.Update(vec2(0.0f), false);
	if (buttonMenu->state.Released()) {
		globals->gui.nextMenu = MenuEnum::MENU_MAIN;
		globals->objects.paused = true;
	} else {
		globals->objects.paused = false;
	}
}

void PlayMenu::Draw(Rendering::DrawingContext &context) {
	screen.Draw(context);
}

const u8 EditorMenu::blockTypes[5] = {
	Entities::World::Block::BLOCK_PLAYER,
	Entities::World::Block::BLOCK_WALL,
	Entities::World::Block::BLOCK_WATER_TOP,
	Entities::World::Block::BLOCK_GOAL,
	Entities::World::Block::BLOCK_SPRINKLER,
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
		text->fontIndex = globals->gui.fontIndex;
		text->fontSize = 24.0f;
		text->alignV = Rendering::CENTER;
		text->string = globals->ReadLocale(blockNames[i]);
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
	listMiddle->color = 0.0f;
	listMiddle->highlight = 0.0f;
	listMiddle->size = vec2(1.0f, 0.0f);
	AddWidget(screenListV, listMiddle);

	spacer = new Widget();
	spacer->size.x = 0.5f;
	AddWidget(listMiddle, spacer);

	ListV *listDialogs = new ListV();
	listDialogs->fractionWidth = false;
	listDialogs->fractionHeight = false;
	listDialogs->size = vec2(480.0f, 0.0f);
	listDialogs->color = 0.0f;
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
	buttonMenu->fontIndex = globals->gui.fontIndex;
	buttonMenu->fractionWidth = false;
	buttonMenu->size.x = 120.0f;
	buttonMenu->string = globals->ReadLocale("Menu");
	buttonMenu->keycodeActivators = {KC_GP_BTN_START, KC_KEY_ESC};
	AddWidgetAsDefault(listBottom, buttonMenu);

	spacer = new Widget();
	spacer->size.x = 1.0f;
	AddWidget(listBottom, spacer);

	buttonNew = new Button(*buttonMenu);
	buttonNew->string = globals->ReadLocale("New");
	buttonNew->keycodeActivators = {KC_GP_BTN_SELECT, KC_KEY_N};
	AddWidget(listBottom, buttonNew);

	buttonLoad = new Button(*buttonMenu);
	buttonLoad->string = globals->ReadLocale("Load");
	buttonLoad->keycodeActivators = {KC_GP_BTN_Y, KC_KEY_L};
	AddWidget(listBottom, buttonLoad);

	buttonSave = new Button(*buttonMenu);
	buttonSave->string = globals->ReadLocale("Save");
	buttonSave->keycodeActivators = {KC_GP_BTN_X, KC_KEY_S};
	AddWidget(listBottom, buttonSave);

	// Dialogs

	buttonCancel = new Button(*buttonMenu);
	buttonCancel->string = globals->ReadLocale("Cancel");
	buttonCancel->keycodeActivators = {KC_GP_BTN_B};
	buttonConfirm = new Button(*buttonMenu);
	buttonConfirm->string = globals->ReadLocale("Confirm");
	buttonConfirm->keycodeActivators = {};

	{ // Resize
		ListV *dialog = new ListV();
		dialog->fractionHeight = false;
		dialog->size.y = 0.0f;

		Text *header = new Text();
		header->bold = true;
		header->fontIndex = globals->gui.fontIndex;
		header->fontSize = 24.0f;
		header->string = globals->ReadLocale("ResizeText");
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
		widthText->fontIndex = globals->gui.fontIndex;
		widthText->fontSize = 24.0f;
		widthText->string = globals->ReadLocale("Width:");
		AddWidget(textboxes, widthText);

		textboxWidth = new TextBox();
		textboxWidth->fractionHeight = true;
		textboxWidth->size.x = 80.0f;
		textboxWidth->size.y = 1.0f;
		textboxWidth->fontIndex = globals->gui.fontIndex;
		textboxWidth->fontSize = 24.0f;
		textboxWidth->alignH = Rendering::RIGHT;
		textboxWidth->string = ToWString("64");
		textboxWidth->textFilter = TextFilterDigits;
		textboxWidth->textValidate = TextValidateNonempty;
		AddWidget(textboxes, textboxWidth);

		Text *heightText = new Text();
		heightText->size.x = 0.5f;
		heightText->fontIndex = globals->gui.fontIndex;
		heightText->fontSize = 24.0f;
		heightText->string = globals->ReadLocale("Height:");
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

		Text *header = new Text();
		header->bold = true;
		header->fontIndex = globals->gui.fontIndex;
		header->fontSize = 24.0f;
		header->string = globals->ReadLocale("SaveText");
		AddWidget(dialog, header);

		textboxFilename = new TextBox();
		textboxFilename->fractionWidth = true;
		textboxFilename->fractionHeight = false;
		textboxFilename->size.x = 1.0f;
		textboxFilename->size.y = 32.0f;
		textboxFilename->margin *= 2.0f;
		textboxFilename->fontIndex = globals->gui.fontIndex;
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

		Text *header = new Text();
		header->bold = true;
		header->fontIndex = globals->gui.fontIndex;
		header->fontSize = 24.0f;
		header->string = globals->ReadLocale("LoadText");
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
		globals->gui.nextMenu = MENU_MAIN;
		globals->objects.paused = true;
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
				globals->entities.world.Resize(vec2i(width, height));
				globals->entities.camPos = vec2(globals->entities.world.size) * 16.0f;
				succeeded = true;
			}
		} else if (!hideableSave->hidden) {
			if (textboxFilename->textValidate(textboxFilename->string)) {
				String filename(textboxFilename->string.size);
				for (i32 i = 0; i < filename.size; i++) {
					filename[i] = textboxFilename->string[i];
				}
				succeeded = globals->entities.world.Save(filename);
			}
		} else if (!hideableLoad->hidden) {
			if (textboxFilename->textValidate(textboxFilename->string)) {
				String filename(textboxFilename->string.size);
				for (i32 i = 0; i < filename.size; i++) {
					filename[i] = textboxFilename->string[i];
				}
				succeeded = globals->entities.world.Load(filename);
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

//
//	  Widget implementations beyond this point
//

Widget::Widget() : children(), margin(8.0f), size(1.0f), fractionWidth(true), fractionHeight(true), minSize(0.0f), maxSize(-1.0f), position(0.0f), sizeAbsolute(0.0f), positionAbsolute(0.0f), depth(0), selectable(false), highlighted(false), occludes(false) {}

void Widget::UpdateSize(vec2 container) {
	sizeAbsolute = vec2(0.0f);
	if (size.x > 0.0f) {
		sizeAbsolute.x = fractionWidth ? (container.x * size.x - margin.x * 2.0f) : size.x;
	} else {
		sizeAbsolute.x = 0.0f;
	}
	if (size.y > 0.0f) {
		sizeAbsolute.y = fractionHeight ? (container.y * size.y - margin.y * 2.0f) : size.y;
	} else {
		sizeAbsolute.y = 0.0f;
	}
	LimitSize();
}

void Widget::LimitSize() {
	if (maxSize.x >= 0.0f) {
		sizeAbsolute.x = median(minSize.x, sizeAbsolute.x, maxSize.x);
	} else {
		sizeAbsolute.x = max(minSize.x, sizeAbsolute.x);
	}
	if (maxSize.y >= 0.0f) {
		sizeAbsolute.y = median(minSize.y, sizeAbsolute.y, maxSize.y);
	} else {
		sizeAbsolute.y = max(minSize.y, sizeAbsolute.y);
	}
}

void Widget::PushScissor(Rendering::DrawingContext &context) const {
	if (sizeAbsolute.x != 0.0f && sizeAbsolute.y != 0.0f) {
		vec2i topLeft = vec2i(
			i32(positionAbsolute.x * globals->gui.scale),
			i32(positionAbsolute.y * globals->gui.scale)
		);
		vec2i botRight = vec2i(
			(i32)ceil((positionAbsolute.x + sizeAbsolute.x) * globals->gui.scale),
			(i32)ceil((positionAbsolute.y + sizeAbsolute.y) * globals->gui.scale)
		);
		globals->rendering.PushScissor(context, topLeft, botRight);
	}
}

void Widget::PopScissor(Rendering::DrawingContext &context) const {
	if (sizeAbsolute.x != 0.0f && sizeAbsolute.y != 0.0f) {
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
		//	 mouse = globals->entities.WorldPosToScreen(globals->entities.mouse);
		// } else {
			mouse = -1.0f;
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
	margin = vec2(0.0f);
}

void Screen::Update(vec2 pos, bool selected) {
	UpdateSize(globals->rendering.screenSize / globals->gui.scale);
	Widget::Update(pos + position, selected);
	// if (selected) {
		FindMouseoverDepth(0);
	// }
}

void Screen::UpdateSize(vec2 container) {
	sizeAbsolute = container - margin * 2.0f;
	for (Widget* child : children) {
		child->UpdateSize(sizeAbsolute);
	}
}

List::List() : padding(8.0f), color(0.05f, 0.05f, 0.05f, 0.9f), highlight(0.05f, 0.05f, 0.05f, 0.9f), select(0.2f, 0.2f, 0.2f, 0.0f), selection(-2), selectionDefault(-1) { occludes = true; }

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
	if ((highlighted ? highlight.a : color.a) > 0.0f) {
		globals->rendering.DrawQuad(context, Rendering::texBlank, highlighted ? highlight : color, positionAbsolute * globals->gui.scale, vec2(1.0f), sizeAbsolute * globals->gui.scale);
	}
	if (selection >= 0 && select.a > 0.0f) {
		vec2 selectionPos = children[selection]->positionAbsolute;
		vec2 selectionSize = children[selection]->sizeAbsolute;
		globals->rendering.DrawQuad(context, Rendering::texBlank, select, selectionPos * globals->gui.scale, vec2(1.0f), selectionSize * globals->gui.scale);
	}
	PushScissor(context);
	Widget::Draw(context);
	PopScissor(context);
}

void ListV::UpdateSize(vec2 container) {
	sizeAbsolute = vec2(0.0f);
	if (size.x > 0.0f) {
		sizeAbsolute.x = fractionWidth ? (container.x * size.x - margin.x * 2.0f) : size.x;
	} else {
		sizeAbsolute.x = padding.x * 2.0f;
	}
	if (size.y > 0.0f) {
		sizeAbsolute.y = fractionHeight ? (container.y * size.y - margin.y * 2.0f) : size.y;
	} else {
		sizeAbsolute.y = padding.y * 2.0f;
	}
	LimitSize();
	vec2 sizeForInheritance = sizeAbsolute - padding * 2.0f;
	if (size.x == 0.0f) {
		for (Widget* child : children) {
			child->UpdateSize(sizeForInheritance);
			vec2 childSize = child->GetSize();
			sizeAbsolute.x = max(sizeAbsolute.x, childSize.x + padding.x * 2.0f);
		}
	}
	sizeForInheritance = sizeAbsolute - padding * 2.0f;
	for (Widget* child : children) {
		if (child->size.y == 0.0f) {
			child->UpdateSize(sizeForInheritance);
			sizeForInheritance.y -= child->GetSize().y;
		} else {
			if (!child->fractionHeight) {
				sizeForInheritance.y -= child->size.y + child->margin.y * 2.0f;
			}
		}
	}
	for (Widget* child : children) {
		child->UpdateSize(sizeForInheritance);
		vec2 childSize = child->GetSize();
		if (size.x == 0.0f) {
			sizeAbsolute.x = max(sizeAbsolute.x, childSize.x + padding.x * 2.0f);
		}
		if (size.y == 0.0f) {
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
	color = vec4(0.0f, 0.0f, 0.0f, 0.9f);
	highlight = vec4(0.1f, 0.1f, 0.1f, 0.9f);
	occludes = true;
}

void ListH::UpdateSize(vec2 container) {
	sizeAbsolute = vec2(0.0f);
	if (size.x > 0.0f) {
		sizeAbsolute.x = fractionWidth ? (container.x * size.x - margin.x * 2.0f) : size.x;
	} else {
		sizeAbsolute.x = padding.x * 2.0f;
	}
	if (size.y > 0.0f) {
		sizeAbsolute.y = fractionHeight ? (container.y * size.y - margin.y * 2.0f) : size.y;
	} else {
		sizeAbsolute.y = padding.y * 2.0f;
	}
	LimitSize();
	vec2 sizeForInheritance = sizeAbsolute - padding * 2.0f;
	if (size.y == 0.0f) {
		for (Widget* child : children) {
			child->UpdateSize(sizeForInheritance);
			vec2 childSize = child->GetSize();
			sizeAbsolute.y = max(sizeAbsolute.y, childSize.y + padding.y * 2.0f);
		}
		sizeForInheritance = sizeAbsolute - padding * 2.0f;
	}
	for (Widget* child : children) {
		if (child->size.x == 0.0f) {
			child->UpdateSize(sizeForInheritance);
			sizeForInheritance.x -= child->GetSize().x;
		} else {
			if (!child->fractionWidth) {
				sizeForInheritance.x -= child->size.x + child->margin.x * 2.0f;
			}
		}
	}
	for (Widget* child : children) {
		child->UpdateSize(sizeForInheritance);
		vec2 childSize = child->GetSize();
		if (size.x == 0.0f) {
			sizeAbsolute.x += childSize.x;
		}
		if (size.y == 0.0f) {
			sizeAbsolute.y = max(sizeAbsolute.y, childSize.y + padding.y * 2.0f);
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
	color = vec4(vec3(0.2f), 0.9f);
	highlight = vec4(colorHighlightMedium, 0.9f);
	select = vec4(colorHighlightMedium, 0.9f);
}

void Switch::UpdateSize(vec2 container) {
	if (open) {
		ListV::UpdateSize(container);
	} else {
		sizeAbsolute = vec2(0.0f);
		if (size.x > 0.0f) {
			sizeAbsolute.x = fractionWidth ? (container.x * size.x - margin.x * 2.0f) : size.x;
		} else {
			sizeAbsolute.x = padding.x * 2.0f;
		}
		if (size.y > 0.0f) {
			sizeAbsolute.y = fractionHeight ? (container.y * size.y - margin.y * 2.0f) : size.y;
		} else {
			sizeAbsolute.y = padding.y * 2.0f;
		}
		LimitSize();
		Widget *child = children[choice];
		vec2 sizeForInheritance = sizeAbsolute - padding * 2.0f;
		if (size.x == 0.0f) {
			child->UpdateSize(sizeForInheritance);
			vec2 childSize = child->GetSize();
			sizeAbsolute.x = max(sizeAbsolute.x, childSize.x + padding.x * 2.0f);
		}
		sizeForInheritance = sizeAbsolute - padding * 2.0f;
		if (child->size.y == 0.0f) {
			child->UpdateSize(sizeForInheritance);
			sizeForInheritance.y -= child->GetSize().y;
		} else {
			if (!child->fractionHeight) {
				sizeForInheritance.y -= child->size.y + child->margin.y * 2.0f;
			}
		}
		child->UpdateSize(sizeForInheritance);
		vec2 childSize = child->GetSize();
		if (size.x == 0.0f) {
			sizeAbsolute.x = max(sizeAbsolute.x, childSize.x + padding.x * 2.0f);
		}
		if (size.y == 0.0f) {
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
	if (color.a > 0.0f) {
		globals->rendering.DrawQuad(context, Rendering::texBlank, (highlighted && !open) ? highlight : color, positionAbsolute * globals->gui.scale, vec2(1.0f), sizeAbsolute * globals->gui.scale);
	}
	PushScissor(context);
	if (open) {
		if (selection >= 0 && select.a > 0.0f) {
			Widget *child = children[selection];
			vec2 selectionPos = child->positionAbsolute - child->margin;
			vec2 selectionSize = child->sizeAbsolute + child->margin * 2.0f;
			globals->rendering.DrawQuad(context, Rendering::texBlank, select, selectionPos * globals->gui.scale, vec2(1.0f), selectionSize * globals->gui.scale);
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

Text::Text() : stringFormatted(), string(), padding(0.1f), fontSize(32.0f), fontIndex(1), bold(false), paddingEM(true), alignH(Rendering::LEFT), alignV(Rendering::TOP), color(vec3(1.0f), 1.0f), colorOutline(vec3(0.0f), 1.0f), highlight(vec3(0.0f), 1.0f), highlightOutline(vec3(1.0f), 1.0f), outline(false) {
	size.y = 0.0f;
}

void Text::UpdateSize(vec2 container) {
	if (size.x > 0.0f) {
		sizeAbsolute.x = fractionWidth ? (container.x * size.x - margin.x * 2.0f) : size.x;
	} else {
		sizeAbsolute.x = globals->rendering.StringWidth(stringFormatted, fontIndex) * fontSize + padding.x * (paddingEM ? fontSize * 2.0f : 2.0f);
	}
	if (size.y > 0.0f) {
		sizeAbsolute.y = fractionHeight ? (container.y * size.y - margin.y * 2.0f) : size.y;
	} else {
		sizeAbsolute.y = Rendering::StringHeight(stringFormatted) * fontSize + padding.y * (paddingEM ? fontSize * 2.0f : 2.0f);
	}
	LimitSize();
}

void Text::Update(vec2 pos, bool selected) {
	if (size.x != 0.0f) {
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
	vec2 textArea = (sizeAbsolute - paddingAbsolute * 2.0f) * globals->gui.scale;
	if (alignH == Rendering::CENTER) {
		drawPos.x += textArea.x * 0.5f;
	} else if (alignH == Rendering::RIGHT) {
		drawPos.x += textArea.x;
	}
	if (alignV == Rendering::CENTER) {
		drawPos.y += textArea.y * 0.5f;
	} else if (alignV == Rendering::BOTTOM) {
		drawPos.y += textArea.y;
	}
	f32 bounds = bold ? 0.425f : 0.525f;
	if (outline) {
		globals->rendering.DrawText(context, stringFormatted, fontIndex, highlighted? highlightOutline : colorOutline, drawPos, scale, alignH, alignV, textArea.x, 0.1f, bounds - 0.2f);
	}
	globals->rendering.DrawText(context, stringFormatted, fontIndex, highlighted? highlight : color, drawPos, scale, alignH, alignV, textArea.x, 0.0f, bounds);
	PopScissor(context);
}

Image::Image() : texIndex(0), color(vec4(1.0f)) { occludes = true; }

void Image::Draw(Rendering::DrawingContext &context) const {
	globals->rendering.DrawQuad(context, texIndex, color, positionAbsolute * globals->gui.scale, vec2(1.0f), sizeAbsolute * globals->gui.scale);
}

Button::Button() : string(), colorBG(vec3(0.15f), 0.9f), highlightBG(colorHighlightMedium, 0.9f), colorText(vec3(1.0f), 1.0f), highlightText(vec3(0.0f), 1.0f), fontIndex(1), fontSize(28.0f), state(), keycodeActivators() {
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
	state.Tick(0.0f);
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
		scale = 0.9f;
	} else {
		scale = 1.0f;
	}
	scale *= globals->gui.scale;
	vec2 drawPos = (positionAbsolute + sizeAbsolute * 0.5f) * globals->gui.scale;
	globals->rendering.DrawQuad(context, Rendering::texBlank, highlighted ? highlightBG : colorBG, drawPos, vec2(1.0f), sizeAbsolute * scale, vec2(0.5f));
	globals->rendering.DrawText(context, string, fontIndex,  highlighted ? highlightText : colorText, drawPos, vec2(fontSize * scale), Rendering::CENTER, Rendering::CENTER, sizeAbsolute.x * globals->gui.scale);
	PopScissor(context);
}

Checkbox::Checkbox() : colorOff(vec3(0.15f), 0.9f), highlightOff(colorHighlightLow, 0.9f), colorOn(colorHighlightMedium, 1.0f), highlightOn(colorHighlightHigh, 1.0f), transition(0.0f), checked(false) {
	selectable = true;
	size = vec2(48.0f, 24.0f);
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
		transition = decay(transition, 1.0f, 0.05f, globals->objects.timestep);
	} else {
		transition = decay(transition, 0.0f, 0.05f, globals->objects.timestep);
	}
}

void Checkbox::Draw(Rendering::DrawingContext &context) const {
	const vec4 &colorOnActual = highlighted ? highlightOn : colorOn;
	const vec4 &colorOffActual = highlighted ? highlightOff : colorOff;
	vec4 colorActual = lerp(colorOffActual, colorOnActual, transition);
	vec2 switchPos = (positionAbsolute + sizeAbsolute * vec2(lerp(0.0625f, 0.5625f, transition), 0.125f)) * globals->gui.scale;
	globals->rendering.DrawQuad(context, Rendering::texBlank, colorActual, positionAbsolute * globals->gui.scale, vec2(1.0f), sizeAbsolute * globals->gui.scale);
	globals->rendering.DrawQuad(context, Rendering::texBlank, vec4(vec3(0.0f), 0.8f), switchPos, vec2(1.0f), (sizeAbsolute * vec2(0.375f, 0.75f)) * globals->gui.scale);
}

TextBox::TextBox() : string(), colorBG(vec3(0.15f), 0.9f), highlightBG(vec3(0.2f), 0.9f), errorBG(0.1f, 0.0f, 0.0f, 0.9f), colorText(vec3(1.0f), 1.0f), highlightText(vec3(1.0f), 1.0f), errorText(1.0f, 0.5f, 0.5f, 1.0f), padding(2.0f), cursor(0), fontIndex(1), fontSize(17.39f), cursorBlinkTimer(0.0f), alignH(Rendering::LEFT), textFilter(TextFilterBasic), textValidate(TextValidateAll), entry(false), multiline(false) {
	// selectable = true;
	occludes = true;
	fractionWidth = false;
	fractionHeight = false;
	size.x = 200.0f;
	size.y = 0.0f;
	minSize.y = 24.0f;
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
	vec2 cursorPos = 0.0f;
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
	globals->rendering.LineCursorStartAndSpaceScale(cursorPos.x, spaceScale, fontSize, spaceWidth, fontIndex, lineString, sizeAbsolute.x - padding.x * 2.0f, alignH);
	cursorPos.x += positionAbsolute.x + padding.x;
	if (alignH == Rendering::CENTER) {
		cursorPos.x += sizeAbsolute.x * 0.5f - padding.x;
	} else if (alignH == Rendering::RIGHT) {
		cursorPos.x += sizeAbsolute.x - padding.x * 2.0f;
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
			advanceCarry = spaceWidth * 0.5f;
		} else {
			advanceCarry = globals->assets.CharacterWidth(c, fontIndex) * fontSize * globals->gui.scale * 0.5f;
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
	vec2 cursorPos = 0.0f;
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
	globals->rendering.LineCursorStartAndSpaceScale(cursorPos.x, spaceScale, fontSize, spaceWidth, fontIndex, lineString, sizeAbsolute.x - padding.x * 2.0f, alignH);
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
		cursorPos.x += sizeAbsolute.x * 0.5f - padding.x;
	} else if (alignH == Rendering::RIGHT) {
		cursorPos.x += sizeAbsolute.x - padding.x * 2.0f;
	}
	cursorPos += positionAbsolute + padding;
	cursorPos *= globals->gui.scale;
	return cursorPos;
}

void TextBox::UpdateSize(vec2 container) {
	if (size.x > 0.0f) {
		sizeAbsolute.x = fractionWidth ? (container.x * size.x - margin.x * 2.0f) : size.x;
	} else {
		sizeAbsolute.x = globals->rendering.StringWidth(stringFormatted, fontIndex) * fontSize + padding.x * 2.0f;
	}
	if (size.y > 0.0f) {
		sizeAbsolute.y = fractionHeight ? (container.y * size.y - margin.y * 2.0f) : size.y;
	} else {
		sizeAbsolute.y = Rendering::StringHeight(stringFormatted) * fontSize + padding.y * 2.0f;
	}
	LimitSize();
}

void TextBox::Update(vec2 pos, bool selected) {
	if (entry) {
		cursorBlinkTimer += globals->objects.timestep;
		if (cursorBlinkTimer > 1.0f) {
			cursorBlinkTimer -= 1.0f;
		}
		highlighted = true;
		if (globals->input.AnyKey.Pressed()) {
			for (i32 i = 0; i < globals->input.typingString.size; i++) {
				const char32 c = globals->input.typingString[i];
				if (textFilter(c)) {
					string.Insert(cursor, c);
					cursorBlinkTimer = 0.0f;
					cursor++;
				}
			}
		}
		globals->input.typingString.Clear();
		if (globals->input.Pressed(KC_KEY_BACKSPACE)) {
			if (cursor <= string.size && cursor > 0) {
				string.Erase(cursor-1);
				cursorBlinkTimer = 0.0f;
				cursor--;
			}
		}
		if (globals->input.Pressed(KC_KEY_DELETE)) {
			if (cursor < string.size) {
				string.Erase(cursor);
				cursorBlinkTimer = 0.0f;
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
			cursorBlinkTimer = 0.0f;
		}
		if (globals->input.Pressed(KC_KEY_END)) {
			if (globals->input.Down(KC_KEY_LEFTCTRL) || globals->input.Down(KC_KEY_RIGHTCTRL) || !multiline) {
				cursor = string.size;
			} else {
				for (; cursor < string.size; cursor++) {
					if (string[cursor] == '\n') break;
				}
			}
			cursorBlinkTimer = 0.0f;
		}
		if (multiline) {
			if (globals->input.Pressed(KC_KEY_ENTER)) {
				string.Insert(cursor, '\n');
				cursor++;
				cursorBlinkTimer = 0.0f;
			}
			if (globals->input.Pressed(KC_KEY_UP)) {
				vec2 cursorPos = PositionFromCursor();
				cursorPos.y -= fontSize * globals->gui.scale * Rendering::lineHeight * 0.5f;
				CursorFromPosition(cursorPos);
				cursorBlinkTimer = 0.0f;
			}
			if (globals->input.Pressed(KC_KEY_DOWN)) {
				vec2 cursorPos = PositionFromCursor();
				cursorPos.y += fontSize * globals->gui.scale * Rendering::lineHeight * 1.5f;
				CursorFromPosition(cursorPos);
				cursorBlinkTimer = 0.0f;
			}
		}
		if (globals->input.Pressed(KC_KEY_LEFT)) {
			cursorBlinkTimer = 0.0f;
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
			cursorBlinkTimer = 0.0f;
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
	if (size.x != 0.0f && multiline) {
		stringFormatted = globals->rendering.StringAddNewlines(string, fontIndex, (sizeAbsolute.x - padding.x * 2.0f) / fontSize);
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
			cursorBlinkTimer = 0.0f;
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
	vec2 textArea = (sizeAbsolute - padding * 2.0f) * globals->gui.scale;
	if (alignH == Rendering::CENTER) {
		drawPosText.x += textArea.x * 0.5f;
	} else if (alignH == Rendering::RIGHT) {
		drawPosText.x += textArea.x;
	}
	vec2 drawPos = positionAbsolute * globals->gui.scale;
	globals->rendering.DrawQuad(context, Rendering::texBlank, bg, drawPos, vec2(1.0f), sizeAbsolute * globals->gui.scale);
	globals->rendering.DrawText(context, stringFormatted, fontIndex, text, drawPosText, scale, alignH, Rendering::TOP, textArea.x);
	if (cursorBlinkTimer < 0.5f && entry) {
		vec2 cursorPos = PositionFromCursor();
		cursorPos.y -= fontSize * globals->gui.scale * 0.1f;
		globals->rendering.DrawQuad(context, Rendering::texBlank, text, cursorPos, vec2(1.0f), vec2(1.0f, fontSize * globals->gui.scale * Rendering::lineHeight));
	}
	PopScissor(context);
}

Slider::Slider() :
value(1.0f),                    valueMin(0.0f),
valueMax(1.0f),                 mirror(nullptr),
colorBG(vec3(0.15f), 0.9f),     colorSlider(colorHighlightMedium, 1.0f),
highlightBG(vec3(0.2f), 0.9f),  highlightSlider(colorHighlightHigh, 1.0f),
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
		f32 sliderX = map(value, valueMin, valueMax, 0.0f, sizeAbsolute.x - 16.0f);
		if (mouseX < sliderX) {
			mousePos = -1;
		} else if (mouseX > sliderX+16.0f) {
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
	f32 scale = (valueMax - valueMin) / (sizeAbsolute.x - 16.0f);
	if (grabbed) {
		f32 moved = f32(globals->input.cursor.x - globals->input.cursorPrevious.x) / globals->gui.scale * scale;
		if (globals->objects.Down(KC_KEY_LEFTSHIFT)) {
			moved /= 10.0f;
		}
		if (moved != 0.0f) updated = true;
		value = clamp(value + moved, valueMin, valueMax);
	}
	if (!globals->objects.Down(KC_KEY_LEFTSHIFT)) {
		scale *= 10.0f;
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
	globals->rendering.DrawQuad(context, Rendering::texBlank, bg, drawPos, vec2(1.0f), sizeAbsolute * globals->gui.scale);
	drawPos.x += map(value, valueMin, valueMax, 2.0f, sizeAbsolute.x - 16.0f) * globals->gui.scale;
	drawPos.y += 2.0f * globals->gui.scale;
	globals->rendering.DrawQuad(context, Rendering::texBlank, slider, drawPos, vec2(1.0f), vec2(12.0f, sizeAbsolute.y - 4.0f) * globals->gui.scale);
}

Hideable::Hideable(Widget *child) : hidden(false), hiddenPrev(false) {
	margin = 0.0f;
	AddWidget(this, child);
	size = child->size; // We need to inherit this for Lists to work properly
	fractionWidth = child->fractionWidth;
	fractionHeight = child->fractionHeight;
	occludes = child->occludes;
}

void Hideable::UpdateSize(vec2 container) {
	if (hidden) {
		sizeAbsolute = 0.0f;
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

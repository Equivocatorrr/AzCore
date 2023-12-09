/*
	File: guiBasic.cpp
	Author: Philip Haynes
*/

#include "gui_basics.hpp"
#include "game_systems.hpp"
#include "settings.hpp"
#include "AzCore/Profiling.hpp"
#include "console_commands.hpp"
#include "AzCore/Math/Color.hpp"

namespace Az2D::Gui {

using namespace AzCore;
using GameSystems::sys;

GuiBasic *guiBasic = nullptr;

GuiBasic::GuiBasic() {
	guiBasic = this;
}

void GuiBasic::EventAssetsRequest() {
	fontIndex = sys->assets.RequestFont(defaultFontFilename);
	for (SoundDef& def : sndClickInDefs) {
		def.soundIndex = sys->assets.RequestSound(def.filename);
	}
	for (SoundDef& def : sndClickOutDefs) {
		def.soundIndex = sys->assets.RequestSound(def.filename);
	}
	for (SoundDef& def : sndClickSoftDefs) {
		def.soundIndex = sys->assets.RequestSound(def.filename);
	}
	sndCheckboxOnDef.soundIndex = sys->assets.RequestSound(sndCheckboxOnDef.filename);
	sndCheckboxOffDef.soundIndex = sys->assets.RequestSound(sndCheckboxOffDef.filename);
}

void AcquireSounds(Array<GuiBasic::SoundDef> &defs, Array<Sound::Source> &sources, Sound::MultiSource &multiSource) {
	sources.Resize(defs.size);
	multiSource.sources.Reserve(defs.size);
	for (i32 i = 0; i < sources.size; i++) {
		GuiBasic::SoundDef &def = defs[i];
		Sound::Source &source = sources[i];
		source.Create(def.soundIndex);
		source.SetGain(def.gain);
		source.SetPitch(def.pitch);
		multiSource.sources.Append(&source);
	}
}

void AcquireSound(GuiBasic::SoundDef &def, Sound::Source &source) {
	source.Create(def.soundIndex);
	source.SetGain(def.gain);
	source.SetPitch(def.pitch);
}

void GuiBasic::EventAssetsAvailable() {
	AcquireSounds(sndClickInDefs, sndClickInSources, sndClickIn);
	AcquireSounds(sndClickOutDefs, sndClickOutSources, sndClickOut);
	AcquireSounds(sndClickSoftDefs, sndClickSoftSources, sndClickSoft);
	AcquireSound(sndCheckboxOnDef, sndCheckboxOn);
	AcquireSound(sndCheckboxOffDef, sndCheckboxOff);
	font = &sys->assets.fonts[fontIndex];
}

String FramerateSetter(void *userdata, String name, String argument) {
	f32 real;
	if (!StringToF32(argument, &real)) {
		return Stringify(name, " expected a real number value");
	}
	real = clamp(real, 10.0f, 1000.0f);
	Az2D::Settings::Name settingName = name;
	Az2D::Settings::SetReal(settingName, real);
	sys->SetFramerate(real);
	return Stringify("set ", name, " to ", real);
}

// Set the drawable region
void SetScissor(Any &dataGlobal, Any &dataWidget, Any &dataDrawCall, vec2 position, vec2 size) {
	(void)dataGlobal;
	(void)dataWidget;
	sys->rendering.SetScissor(dataDrawCall.Get<Rendering::DrawingContext>(), position, size);
}

void DrawQuad(Any &dataGlobal, Any &dataWidget, Any &dataDrawCall, vec2 position, vec2 size, vec4 color) {
	(void)dataGlobal;
	(void)dataWidget;
	Rendering::Material material(color);
	sys->rendering.DrawQuad(dataDrawCall.Get<Rendering::DrawingContext>(), position, size, vec2(1.0f), vec2(0.0f), 0.0f, Rendering::PIPELINE_BASIC_2D, material);
}

void DrawImage(Any &dataGlobal, Any &dataWidget, Any &dataDrawCall, vec2 position, vec2 size, vec4 color) {
	(void)dataGlobal;
	ImageMetadata metadata;
	if (dataWidget.IsType<ImageMetadata>()) {
		metadata = dataWidget.Get<ImageMetadata>();
	}
	Rendering::Material material;
	material.color = color;
	sys->rendering.DrawQuad(dataDrawCall.Get<Rendering::DrawingContext>(), position, size, vec2(1.0f), vec2(0.0f), 0.0f, metadata.pipeline, material, Rendering::TexIndices(metadata.texIndex));
}

void DrawText(Any &dataGlobal, Any &dataWidget, Any &dataDrawCall, vec2 position, vec2 area, vec2 fontSize, const WString &text, vec4 color, vec4 colorOutline, bool bold) {
	GuiBasic &guiBasic = dataGlobal.Get<GuiBasic>();
	TextMetadata metadata;
	if (dataWidget.IsType<TextMetadata>()) {
		metadata = dataWidget.Get<TextMetadata>();
	}
	if (metadata.fontIndex == -1) metadata.fontIndex = guiBasic.fontIndex;
	if (metadata.alignH == Rendering::CENTER) {
		position.x += area.x * 0.5f;
	} else if (metadata.alignH == Rendering::RIGHT) {
		position.x += area.x;
	}
	if (metadata.alignV == Rendering::CENTER) {
		position.y += area.y * 0.5f;
	} else if (metadata.alignV == Rendering::BOTTOM) {
		position.y += area.y;
	}
	f32 bounds = bold ? 0.425f : 0.525f;
	if (colorOutline.a > 0.0f) {
		sys->rendering.DrawText(dataDrawCall.Get<Rendering::DrawingContext>(), text, metadata.fontIndex, colorOutline, position, fontSize, metadata.alignH, metadata.alignV, area.x, 0.05f, bounds - 0.325f - clamp((1.0f - (colorOutline.r + colorOutline.g + colorOutline.b) / 3.0f) * 2.0f, 0.0f, 2.0f)/fontSize.y);
	}
	bounds -= clamp((1.0f - (color.r + color.g + color.b) / 3.0f) * 2.0f, 0.0f, 2.0f) / fontSize.y;
	sys->rendering.DrawText(dataDrawCall.Get<Rendering::DrawingContext>(), text, metadata.fontIndex, color, position, fontSize, metadata.alignH, metadata.alignV, area.x, 0.0f, bounds);
}

// Units are in the font's EM square
// Multiply this by the font size for the actual dimensions
vec2 GetTextDimensions(Any &dataGlobal, Any &dataWidget, const WString &string) {
	GuiBasic &guiBasic = dataGlobal.Get<GuiBasic>();
	Assets::FontIndex fontIndex = -1;
	if (dataWidget.IsType<TextMetadata>()) {
		fontIndex = dataWidget.Get<TextMetadata>().fontIndex;
	}
	if (fontIndex == -1) fontIndex = guiBasic.fontIndex;
	return sys->rendering.StringSize(string, fontIndex);
}

// Units are in the font's EM square
// Divide the actual width by the font size for the EM size
WString ApplyTextWrapping(Any &dataGlobal, Any &dataWidget, const WString &string, f32 maxWidth) {
	GuiBasic &guiBasic = dataGlobal.Get<GuiBasic>();
	Assets::FontIndex fontIndex = -1;
	if (dataWidget.IsType<TextMetadata>()) {
		fontIndex = dataWidget.Get<TextMetadata>().fontIndex;
	}
	if (fontIndex == -1) fontIndex = guiBasic.fontIndex;
	return sys->rendering.StringAddNewlines(string, fontIndex, maxWidth);
}

// Returns the index into the text to place the cursor based on pickerPosition. It should aim to find the cursor position closest to the left of the character halfway between lines (a UV of {0, 0.5}).
i32 GetCursorFromPositionInText(Any &dataGlobal, Any &dataWidget, vec2 position, vec2 area, vec2 fontSize, const SimpleRange<char32> text, vec2 pickerPosition) {
	GuiBasic &guiBasic = dataGlobal.Get<GuiBasic>();
	TextMetadata metadata;
	if (dataWidget.IsType<TextMetadata>()) {
		metadata = dataWidget.Get<TextMetadata>();
	}
	if (metadata.fontIndex == -1) metadata.fontIndex = guiBasic.fontIndex;
	
	vec2 cursorPos = 0.0f;
	f32 spaceScale, spaceWidth, tabWidth;
	spaceWidth = sys->assets.CharacterWidth(' ', metadata.fontIndex) * fontSize.x;
	tabWidth = sys->assets.CharacterWidth((char32)'_', metadata.fontIndex) * fontSize.x * 4.0f;
	const char32 *lineString = text.str;
	i32 cursor = 0;
	// Find which line we're on first
	cursorPos.y += fontSize.y * Rendering::lineHeight + position.y;
	if (cursorPos.y <= pickerPosition.y / guiBasic.system.scale) {
		for (; cursor < text.size; cursor++) {
			const char32 &c = text[cursor];
			if (c == '\n') {
				lineString = &c+1;
				cursorPos.y += fontSize.y * Rendering::lineHeight;
				if (cursorPos.y > pickerPosition.y / guiBasic.system.scale) {
					cursor++;
					break;
				}
			}
		}
	}
	// Find where we are in that line
	sys->rendering.LineCursorStartAndSpaceScale(cursorPos.x, spaceScale, fontSize.x, spaceWidth, metadata.fontIndex, lineString, area.x, metadata.alignH);
	cursorPos.x += position.x;
	if (metadata.alignH == Rendering::CENTER) {
		cursorPos.x += area.x * 0.5f;
	} else if (metadata.alignH == Rendering::RIGHT) {
		cursorPos.x += area.x;
	}
	cursorPos *= guiBasic.system.scale;
	spaceWidth *= spaceScale * guiBasic.system.scale;
	for (; cursor < text.size; cursor++) {
		const char32 &c = text[cursor];
		f32 halfAdvance;
		if (c == '\n') {
			break;
		} else if (c == '\t') {
			halfAdvance = (ceil((cursorPos.x-position.x)/tabWidth+0.05f) * tabWidth - (cursorPos.x-position.x)) * 0.5f;
		} else if (c == ' ') {
			halfAdvance = spaceWidth * 0.5f;
		} else {
			halfAdvance = sys->assets.CharacterWidth(c, metadata.fontIndex) * fontSize.x * guiBasic.system.scale * 0.5f;
		}
		cursorPos.x += halfAdvance;
		if (cursorPos.x > pickerPosition.x) {
			break;
		}
		cursorPos.x += halfAdvance;
	}
	return cursor;
}

// Returns the absolute position of a UV within the character at cursor where a UV of {0, 0} is the top left, and {1, 1} is the bottom right.
vec2 GetPositionFromCursorInText(Any &dataGlobal, Any &dataWidget, vec2 position, vec2 area, vec2 fontSize, const SimpleRange<char32> text, i32 cursor, vec2 charUV) {
	GuiBasic &guiBasic = dataGlobal.Get<GuiBasic>();
	TextMetadata metadata;
	if (dataWidget.IsType<TextMetadata>()) {
		metadata = dataWidget.Get<TextMetadata>();
	}
	if (metadata.fontIndex == -1) metadata.fontIndex = guiBasic.fontIndex;
	
	vec2 cursorPos = 0.0f;
	f32 spaceScale, spaceWidth, tabWidth;
	spaceWidth = sys->assets.CharacterWidth(' ', metadata.fontIndex) * fontSize.x;
	tabWidth = sys->assets.CharacterWidth((char32)'_', metadata.fontIndex) * fontSize.x * 4.0f;
	// Get our line start and vertical position
	const char32 *lineString = text.str;
	i32 lineStart = 0;
	for (i32 i = 0; i < cursor; i++) {
		const char32 &c = text[i];
		if (c == '\n') {
			cursorPos.y += fontSize.y * Rendering::lineHeight;
			lineString = &c+1;
			lineStart = i+1;
		}
	}
	// Get our horizontal position
	sys->rendering.LineCursorStartAndSpaceScale(cursorPos.x, spaceScale, fontSize.x, spaceWidth, metadata.fontIndex, lineString, area.x, metadata.alignH);
	spaceWidth *= spaceScale;
	for (i32 i = lineStart; i < cursor; i++) {
		const char32 &c = text[i];
		if (c == '\n') {
			break;
		} if (c == '\t') {
			cursorPos.x = ceil((cursorPos.x)/tabWidth+0.05f) * tabWidth;
			continue;
		}
		if (c == ' ') {
			cursorPos.x += spaceWidth;
		} else {
			cursorPos.x += sys->assets.CharacterWidth(c, metadata.fontIndex) * fontSize.x;
		}
	}
	if (metadata.alignH == Rendering::CENTER) {
		cursorPos.x += area.x * 0.5f;
	} else if (metadata.alignH == Rendering::RIGHT) {
		cursorPos.x += area.x;
	}
	cursorPos += position;
	cursorPos *= guiBasic.system.scale;
	return cursorPos;
}

// Returns the height of one line for the given fontSize for the given widget.
f32 GetLineHeight(Any &dataGlobal, Any &dataWidget, f32 fontSize) {
	return Rendering::lineHeight * fontSize;
}

bool KeycodePressed(Any &dataGlobal, Any &dataWidget, u8 keycode) {
	return sys->Pressed(keycode);
}

bool KeycodeRepeated(Any &dataGlobal, Any &dataWidget, u8 keycode) {
	return sys->Repeated(keycode);
}

bool KeycodeDown(Any &dataGlobal, Any &dataWidget, u8 keycode) {
	return sys->Down(keycode);
}

bool KeycodeReleased(Any &dataGlobal, Any &dataWidget, u8 keycode) {
	return sys->Released(keycode);
}

WString ConsumeTypingString(Any &dataGlobal, Any &dataWidget) {
	WString result = ToWString(sys->input.typingString);
	sys->input.typingString.ClearSoft();
	return result;
}

void OnButtonPressed(Any &dataGlobal, Any &dataWidget) {
	GuiBasic &guiBasic = dataGlobal.Get<GuiBasic>();
	guiBasic.sndClickIn.Play();
}

void OnButtonRepeated(Any &dataGlobal, Any &dataWidget) {
	// Do nothing
}

void OnButtonReleased(Any &dataGlobal, Any &dataWidget) {
	GuiBasic &guiBasic = dataGlobal.Get<GuiBasic>();
	guiBasic.sndClickOut.Play();
}

void OnButtonHighlighted(Any &dataGlobal, Any &dataWidget) {
	GuiBasic &guiBasic = dataGlobal.Get<GuiBasic>();
	guiBasic.sndClickSoft.Play();
}

void OnCheckboxTurnedOn(Any &dataGlobal, Any &dataWidget) {
	GuiBasic &guiBasic = dataGlobal.Get<GuiBasic>();
	guiBasic.sndCheckboxOn.Play();
}

void OnCheckboxTurnedOff(Any &dataGlobal, Any &dataWidget) {
	GuiBasic &guiBasic = dataGlobal.Get<GuiBasic>();
	guiBasic.sndCheckboxOff.Play();
}


void GuiBasic::EventInitialize() {
	system.data = this;
	
	system.functions.SetScissor = SetScissor;
	system.functions.DrawQuad = DrawQuad;
	system.functions.DrawImage = DrawImage;
	system.functions.DrawText = DrawText;
	system.functions.GetTextDimensions = GetTextDimensions;
	system.functions.ApplyTextWrapping = ApplyTextWrapping;
	system.functions.GetCursorFromPositionInText = GetCursorFromPositionInText;
	system.functions.GetPositionFromCursorInText = GetPositionFromCursorInText;
	system.functions.GetLineHeight = GetLineHeight;
	
	system.functions.KeycodePressed = KeycodePressed;
	system.functions.KeycodeRepeated = KeycodeRepeated;
	system.functions.KeycodeDown = KeycodeDown;
	system.functions.KeycodeReleased = KeycodeReleased;
	
	system.functions.ConsumeTypingString = ConsumeTypingString;
	
	system.functions.OnButtonPressed     = OnButtonPressed;
	// system.functions.OnButtonRepeated    = OnButtonRepeated;
	system.functions.OnButtonReleased    = OnButtonReleased;
	system.functions.OnButtonHighlighted = OnButtonHighlighted;
	system.functions.OnCheckboxTurnedOn  = OnCheckboxTurnedOn;
	system.functions.OnCheckboxTurnedOff = OnCheckboxTurnedOff;
	
	devConsole.Initialize();
	Dev::AddGlobalVariable(Az2D::Settings::sDebugInfo.GetString(), "Whether to display frame rate and time information.", nullptr, Dev::defaultBoolSettingsGetter, Dev::defaultBoolSettingsSetter);
	Dev::AddGlobalVariable(Az2D::Settings::sFullscreen.GetString(), "Whether the window should be fullscreen.", nullptr, Dev::defaultBoolSettingsGetter, Dev::defaultBoolSettingsSetter);
	Dev::AddGlobalVariable(Az2D::Settings::sVSync.GetString(), "Whether to enable vertical sync.", nullptr, Dev::defaultBoolSettingsGetter, Dev::defaultBoolSettingsSetter);
	Dev::AddGlobalVariable(Az2D::Settings::sFramerate.GetString(), "Target framerate when vsync is disabled.", nullptr, Dev::defaultRealSettingsGetter, FramerateSetter);
	Dev::AddGlobalVariable(Az2D::Settings::sGuiScale.GetString(), "A scaling factor for all GUIs.", &system.scale, Dev::defaultRealSettingsGetter, Dev::defaultRealSettingsSetter);
}

void GuiBasic::EventSync() {
	system.Update(sys->input.cursor, sys->rendering.screenSize, sys->timestep);
	if (sys->Pressed(KC_KEY_GRAVE)) {
		console = !console;
		sys->input.typingString.Clear();
		if (console) {
			devConsole.textboxInput->entry = true;
		}
	}
	if (console) {
		devConsole.Update();
	}
}

void GuiBasic::EventDraw(Array<Rendering::DrawingContext> &contexts) {
	if (console) {
		devConsole.Draw(contexts.Back());
	}
}

void DevConsole::Initialize() {
	system = &guiBasic->system;
	screen = system->CreateScreen();
	az::GuiGeneric::ListV *listV = system->CreateListV(screen);
	listV->color = ColorFromARGB(0xee0a1a1a);
	listV->colorHighlighted = listV->color;
	listV->SetSizeFraction(vec2(1.0f, 0.3f));
	listV->padding = 4.0f;
	listV->margin = 0.0f;
	listV->scrollableY = false;
	
	az::GuiGeneric::ListV *outputListV = system->CreateListV(listV);
	outputListV->color = vec4(0.0f);
	outputListV->colorHighlighted = vec4(0.0f);
	outputListV->SetSizeFraction(vec2(1.0f));
	outputListV->padding = 0.0f;
	outputListV->margin = 4.0f;
	
	consoleOutput = system->CreateText(outputListV);
	consoleOutputMeta->alignV = Rendering::BOTTOM;
	consoleOutput->data = consoleOutputMeta.RawPtr();
	consoleOutput->fontSize = 16.0f;
	consoleOutput->margin = 0.0f;
	
	textboxInput = system->CreateTextbox(listV);
	textboxInput->minSize.y = 24.0f;
	textboxInput->SetWidthFraction(1.0f);
	textboxInput->colorBG = ColorFromARGB(0xff1a4033);
	textboxInput->colorBGHighlighted = ColorFromARGB(0xff245947);
	textboxInput->fontSize = 16.0f;
	textboxInput->margin = 4.0f;
	textboxInput->multiline = true;
}

void DevConsole::Update() {
	if (Az2D::Settings::ReadBool(Az2D::Settings::sDebugInfo)) {
		screen->margin.y = 20.0f;
	} else {
		screen->margin.y = 0.0f;
	}
	if (sys->Pressed(KC_KEY_ENTER) && !(sys->Down(KC_KEY_LEFTSHIFT) || sys->Down(KC_KEY_RIGHTSHIFT))) {
		for (i32 i = CONSOLE_COMMAND_OUTPUT_LINES_CAP-1; i > 0; i--) {
			outputLines[i] = std::move(outputLines[i-1]);
		}
		recentCommand = -1;
		// This clears the texbox string
		outputLines[0] = ToWString(Dev::HandleCommand(FromWString(textboxInput->string)));
		previousCommands[nextCommand] = textboxInput->string;
		if (textboxInput->string.size != 0) {
			nextCommand++;
			textboxInput->string.ClearSoft();
			textboxInput->cursor = 0;
		}
		sys->input.typingString.ClearSoft();
		numCommandsInHistory++;
		if (nextCommand == CONSOLE_COMMAND_HISTORY_CAP) {
			nextCommand = 0;
			numCommandsInHistory = CONSOLE_COMMAND_HISTORY_CAP;
		}
		consoleOutput->string.ClearSoft();
		for (i32 i = CONSOLE_COMMAND_OUTPUT_LINES_CAP-1; i >= 0; i--) {
			if (outputLines[i].size == 0) continue;
			consoleOutput->string.Append(outputLines[i]);
			consoleOutput->string.Append('\n');
		}
		if (consoleOutput->string.size > 0) consoleOutput->string.size--;
	} else {
		if (numCommandsInHistory > 0) {
			bool changeCommand = false;
			if ((recentCommand != -1 || textboxInput->string.size == 0) && (recentCommand == -1 || textboxInput->string == previousCommands[recentCommand])) {
				if (sys->Pressed(KC_KEY_UP)) {
					if (recentCommand == -1) {
						recentCommand = nextCommand-1;
					} else {
						if (recentCommand == 0) {
							recentCommand = numCommandsInHistory - 1;
						} else {
							recentCommand -= 1;
						}
					}
					changeCommand = true;
				} else if (sys->Pressed(KC_KEY_DOWN)) {
					if (recentCommand >= numCommandsInHistory-1) {
						textboxInput->string.ClearSoft();
						textboxInput->cursor = 0;
						recentCommand = -1;
					} else {
						recentCommand += 1;
						changeCommand = true;
					}
				}
			}
			if (changeCommand) {
				textboxInput->string = previousCommands[recentCommand];
				textboxInput->cursor = textboxInput->string.size;
			}
		}
		screen->Update(vec2(0.0f), false);
	}
}

void DevConsole::Draw(Rendering::DrawingContext &context) {
	Any anyContext = &context;
	screen->Draw(anyContext);
}

} // namespace Az2D::Gui

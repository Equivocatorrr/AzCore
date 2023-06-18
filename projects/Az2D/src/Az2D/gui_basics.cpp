/*
	File: guiBasic.cpp
	Author: Philip Haynes
*/

#include "gui_basics.hpp"
#include "game_systems.hpp"
#include "profiling.hpp"

namespace Az2D::Gui {

using namespace AzCore;
using GameSystems::sys;

GuiBasic *guiBasic = nullptr;

GuiBasic::GuiBasic() {
	guiBasic = this;
}

void AddWidget(Widget *parent, Widget *newWidget, bool deeper) {
	newWidget->depth = parent->depth + (deeper ? 1 : 0);
	if (newWidget->selectable) {
		parent->selectable = true;
	}
	parent->children.Append(newWidget);
	if (!guiBasic->allWidgets.Exists(newWidget)) {
		guiBasic->allWidgets.Emplace(newWidget);
	}
}

void AddWidget(Widget *parent, Switch *newWidget) {
	newWidget->depth = parent->depth + 1;
	newWidget->parentDepth = parent->depth;
	if (newWidget->selectable) {
		parent->selectable = true;
	}
	parent->children.Append(newWidget);
	if (!guiBasic->allWidgets.Exists(newWidget)) {
		guiBasic->allWidgets.Emplace(newWidget);
	}
}

void AddWidgetAsDefault(List *parent, Widget *newWidget, bool deeper) {
	newWidget->depth = parent->depth + (deeper ? 1 : 0);
	if (newWidget->selectable) {
		parent->selectable = true;
	}
	parent->selectionDefault = parent->children.size;
	parent->children.Append(newWidget);
	if (!guiBasic->allWidgets.Exists(newWidget)) {
		guiBasic->allWidgets.Emplace(newWidget);
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
	if (!guiBasic->allWidgets.Exists(newWidget)) {
		guiBasic->allWidgets.Emplace(newWidget);
	}
}

GuiBasic::~GuiBasic() {
	for (Widget* widget : allWidgets) {
		delete widget;
	}
}

void GuiBasic::EventAssetsQueue() {
	sys->assets.QueueFile(defaultFontFilename);
	for (SoundDef& def : sndClickInDefs) {
		sys->assets.QueueFile(def.filename);
	}
	for (SoundDef& def : sndClickOutDefs) {
		sys->assets.QueueFile(def.filename);
	}
	for (SoundDef& def : sndClickSoftDefs) {
		sys->assets.QueueFile(def.filename);
	}
	sys->assets.QueueFile(sndCheckboxOnDef.filename);
	sys->assets.QueueFile(sndCheckboxOffDef.filename);
}

void AcquireSounds(Array<GuiBasic::SoundDef> &defs, Array<Sound::Source> &sources, Sound::MultiSource &multiSource) {
	sources.Resize(defs.size);
	multiSource.sources.Reserve(defs.size);
	for (i32 i = 0; i < sources.size; i++) {
		GuiBasic::SoundDef &def = defs[i];
		Sound::Source &source = sources[i];
		source.Create(def.filename);
		source.SetGain(def.gain);
		source.SetPitch(def.pitch);
		multiSource.sources.Append(&source);
	}
}

void AcquireSound(GuiBasic::SoundDef &def, Sound::Source &source) {
	source.Create(def.filename);
	source.SetGain(def.gain);
	source.SetPitch(def.pitch);
}

void GuiBasic::EventAssetsAcquire() {
	fontIndex = sys->assets.FindFont(defaultFontFilename);
	AcquireSounds(sndClickInDefs, sndClickInSources, sndClickIn);
	AcquireSounds(sndClickOutDefs, sndClickOutSources, sndClickOut);
	AcquireSounds(sndClickSoftDefs, sndClickSoftSources, sndClickSoft);
	AcquireSound(sndCheckboxOnDef, sndCheckboxOn);
	AcquireSound(sndCheckboxOffDef, sndCheckboxOff);
	font = &sys->assets.fonts[fontIndex];
}

void GuiBasic::EventSync() {
	mouseoverWidget = nullptr;
	mouseoverDepth = -1;
	if (sys->input.cursor != sys->input.cursorPrevious) {
		usingMouse = true;
		usingGamepad = false;
		usingArrows = false;
	} else if (sys->rawInput.AnyGP.Pressed()) {
		usingGamepad = true;
		usingMouse = false;
		usingArrows = false;
	} else if (sys->Pressed(KC_KEY_UP) || sys->Pressed(KC_KEY_DOWN) || sys->Pressed(KC_KEY_LEFT) || sys->Pressed(KC_KEY_RIGHT)) {
		usingMouse = false;
		usingGamepad = false;
		usingArrows = true;
	}
}


Widget::Widget() : children(), margin(8.0f), size(1.0f), fractionWidth(true), fractionHeight(true), minSize(0.0f), maxSize(-1.0f), position(0.0f), sizeAbsolute(0.0f), positionAbsolute(0.0f), depth(0), selectable(false), highlighted(false), occludes(false), mouseover(false) {}

void Widget::UpdateSize(vec2 container, f32 _scale) {
	scale = _scale;
	sizeAbsolute = vec2(0.0f);
	vec2 totalMargin = margin * 2.0f * scale;
	if (size.x > 0.0f) {
		sizeAbsolute.x = fractionWidth ? (container.x * size.x - totalMargin.x) : size.x * scale;
	} else {
		sizeAbsolute.x = 0.0f;
	}
	if (size.y > 0.0f) {
		sizeAbsolute.y = fractionHeight ? (container.y * size.y - totalMargin.y) : size.y * scale;
	} else {
		sizeAbsolute.y = 0.0f;
	}
	LimitSize();
}

void Widget::LimitSize() {
	if (maxSize.x >= 0.0f) {
		sizeAbsolute.x = median(minSize.x * scale, sizeAbsolute.x, maxSize.x * scale);
	} else {
		sizeAbsolute.x = max(minSize.x * scale, sizeAbsolute.x);
	}
	if (maxSize.y >= 0.0f) {
		sizeAbsolute.y = median(minSize.y * scale, sizeAbsolute.y, maxSize.y * scale);
	} else {
		sizeAbsolute.y = max(minSize.y * scale, sizeAbsolute.y);
	}
}

void Widget::PushScissor(Rendering::DrawingContext &context) const {
	if (sizeAbsolute.x != 0.0f && sizeAbsolute.y != 0.0f) {
		vec2i topLeft = vec2i(
			i32(positionAbsolute.x * guiBasic->scale),
			i32(positionAbsolute.y * guiBasic->scale)
		);
		vec2i botRight = vec2i(
			(i32)ceil((positionAbsolute.x + sizeAbsolute.x) * guiBasic->scale),
			(i32)ceil((positionAbsolute.y + sizeAbsolute.y) * guiBasic->scale)
		);
		sys->rendering.PushScissor(context, topLeft, botRight);
	}
}

void Widget::PopScissor(Rendering::DrawingContext &context) const {
	if (sizeAbsolute.x != 0.0f && sizeAbsolute.y != 0.0f) {
		sys->rendering.PopScissor(context);
	}
}

void Widget::Update(vec2 pos, bool selected) {
	pos += (margin + position) * scale;
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
	if (guiBasic->usingMouse) {
		mouse = vec2(sys->input.cursor) / guiBasic->scale;
	} else {
		mouse = -1.0f;
	}
	return mouse.x == median(positionAbsolute.x, mouse.x, positionAbsolute.x + sizeAbsolute.x)
		&& mouse.y == median(positionAbsolute.y, mouse.y, positionAbsolute.y + sizeAbsolute.y);
}

void Widget::FindMouseoverDepth(i32 actualDepth) {
	if (actualDepth <= guiBasic->mouseoverDepth) return;
	if (MouseOver()) {
		if (occludes) {
			guiBasic->mouseoverDepth = actualDepth;
			guiBasic->mouseoverWidget = this;
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
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Gui::Screen::Update)
	UpdateSize(sys->rendering.screenSize / guiBasic->scale, 1.0f);
	Widget::Update(pos, selected);
	// if (selected) {
		FindMouseoverDepth(0);
	// }
}

void Screen::UpdateSize(vec2 container, f32 _scale) {
	AZ2D_PROFILING_SCOPED_TIMER(Az2D::Gui::Screen::UpdateSize)
	scale = _scale;
	sizeAbsolute = container - margin * 2.0f * scale;
	for (Widget* child : children) {
		child->UpdateSize(sizeAbsolute, scale);
	}
}

List::List() : padding(8.0f), color(0.05f, 0.05f, 0.05f, 0.9f), highlight(0.05f, 0.05f, 0.05f, 0.9f), select(0.2f, 0.2f, 0.2f, 0.0f), selection(-2), selectionDefault(-1) { occludes = true; }

bool List::UpdateSelection(bool selected, StaticArray<u8, 4> keyCodeSelect, StaticArray<u8, 4> keyCodeBack, StaticArray<u8, 4> keyCodeIncrement, StaticArray<u8, 4> keyCodeDecrement) {
	highlighted = selected;
	if (selected) {
		bool select = false;
		for (u8 kc : keyCodeSelect) {
			if (sys->Released(kc)) select = true;
		}
		bool back = false;
		for (u8 kc : keyCodeBack) {
			if (sys->Released(kc)) {
				back = true;
				if (guiBasic->controlDepth > depth) {
					sys->ConsumeInput(kc);
				}
			}
		}
		bool increment = false;
		for (u8 kc : keyCodeIncrement) {
			if (sys->Repeated(kc)) increment = true;
		}
		bool decrement = false;
		for (u8 kc : keyCodeDecrement) {
			if (sys->Repeated(kc)) decrement = true;
		}
		if (guiBasic->controlDepth == depth) {
			if (selection >= 0 && selection < children.size && select) {
				guiBasic->controlDepth = children[selection]->depth;
			}
			if (increment) {
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
			} else if (decrement) {
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
		} else if (guiBasic->controlDepth == depth+1 && back) {
			guiBasic->controlDepth = depth;
		}
		if (guiBasic->controlDepth > depth) {
			highlighted = false;
		}
	} else {
		selection = -2;
	}
	if (guiBasic->controlDepth == depth && selected) {
		bool mouseSelect = false;
		if (guiBasic->usingMouse /* && sys->input.cursor != sys->input.cursorPrevious */) {
			if (MouseOver()) {
				mouseSelect = true;
			}
			selection = -1;
		} else if (selection < 0 && !guiBasic->usingMouse && (sys->rawInput.AnyGP.state != 0 || sys->input.AnyKey.state != 0)) {
			selection = selectionDefault;
		}
		return mouseSelect;
	}
	return false;
}

void List::Draw(Rendering::DrawingContext &context) const {
	if ((highlighted ? highlight.a : color.a) > 0.0f) {
		sys->rendering.DrawQuad(context, positionAbsolute * guiBasic->scale, sizeAbsolute * guiBasic->scale, 1.0f, 0.0f, 0.0f, Rendering::PIPELINE_BASIC_2D, highlighted ? highlight : color);
	}
	if (selection >= 0 && select.a > 0.0f) {
		vec2 selectionPos = children[selection]->positionAbsolute;
		vec2 selectionSize = children[selection]->sizeAbsolute;
		sys->rendering.DrawQuad(context, selectionPos * guiBasic->scale, selectionSize * guiBasic->scale, 1.0f, 0.0f, 0.0f, Rendering::PIPELINE_BASIC_2D, select);
	}
	PushScissor(context);
	Widget::Draw(context);
	PopScissor(context);
}

void ListV::UpdateSize(vec2 container, f32 _scale) {
	scale = _scale;
	sizeAbsolute = vec2(0.0f);
	vec2 totalMargin = margin * 2.0f * scale;
	vec2 totalPadding = padding * 2.0f * scale;
	if (size.x > 0.0f) {
		sizeAbsolute.x = fractionWidth ? (container.x * size.x - totalMargin.x) : size.x * scale;
	} else {
		sizeAbsolute.x = totalPadding.x;
	}
	if (size.y > 0.0f) {
		sizeAbsolute.y = fractionHeight ? (container.y * size.y - totalMargin.y) : size.y * scale;
	} else {
		sizeAbsolute.y = totalPadding.y;
	}
	LimitSize();
	vec2 sizeForInheritance = sizeAbsolute - totalPadding;
	if (size.x == 0.0f) {
		for (Widget* child : children) {
			child->UpdateSize(sizeForInheritance, scale);
			vec2 childSize = child->GetSize();
			sizeAbsolute.x = max(sizeAbsolute.x, childSize.x + totalPadding.x);
		}
	}
	sizeForInheritance = sizeAbsolute - totalPadding;
	for (Widget* child : children) {
		if (child->size.y == 0.0f) {
			child->UpdateSize(sizeForInheritance, scale);
			sizeForInheritance.y -= child->GetSize().y;
		} else {
			if (!child->fractionHeight) {
				sizeForInheritance.y -= (child->size.y + child->margin.y * 2.0f) * child->scale;
			}
		}
	}
	for (Widget* child : children) {
		child->UpdateSize(sizeForInheritance, scale);
		vec2 childSize = child->GetSize();
		if (size.x == 0.0f) {
			sizeAbsolute.x = max(sizeAbsolute.x, childSize.x + totalPadding.x);
		}
		if (size.y == 0.0f) {
			sizeAbsolute.y += childSize.y;
		}
	}
	LimitSize();
}

void ListV::Update(vec2 pos, bool selected) {
	pos += (margin + position) * scale;
	positionAbsolute = pos;
	const bool mouseSelect = UpdateSelection(selected, {KC_GP_BTN_A, KC_KEY_ENTER}, {KC_GP_BTN_B, KC_KEY_ESC}, {KC_GP_AXIS_LS_DOWN, KC_KEY_DOWN}, {KC_GP_AXIS_LS_UP, KC_KEY_UP});
	pos += padding * scale;
	if (mouseSelect) {
		f32 childY = pos.y;
		for (selection = 0; selection < children.size; selection++) {
			Widget *child = children[selection];
			if (!child->Selectable()) {
				childY += child->GetSize().y;
				continue;
			}
			child->positionAbsolute.x = pos.x + child->margin.x * child->scale;
			child->positionAbsolute.y = childY + child->margin.y * child->scale;
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

void ListH::UpdateSize(vec2 container, f32 _scale) {
	scale = _scale;
	sizeAbsolute = vec2(0.0f);
	vec2 totalMargin = margin * 2.0f * scale;
	vec2 totalPadding = padding * 2.0f * scale;
	if (size.x > 0.0f) {
		sizeAbsolute.x = fractionWidth ? (container.x * size.x - totalMargin.x) : size.x * scale;
	} else {
		sizeAbsolute.x = totalPadding.x;
	}
	if (size.y > 0.0f) {
		sizeAbsolute.y = fractionHeight ? (container.y * size.y - totalMargin.y) : size.y * scale;
	} else {
		sizeAbsolute.y = totalPadding.y;
	}
	LimitSize();
	vec2 sizeForInheritance = sizeAbsolute - totalPadding;
	if (size.y == 0.0f) {
		for (Widget* child : children) {
			child->UpdateSize(sizeForInheritance, scale);
			vec2 childSize = child->GetSize();
			sizeAbsolute.y = max(sizeAbsolute.y, childSize.y + totalPadding.y);
		}
		sizeForInheritance = sizeAbsolute - totalPadding;
	}
	for (Widget* child : children) {
		if (child->size.x == 0.0f) {
			child->UpdateSize(sizeForInheritance, scale);
			sizeForInheritance.x -= child->GetSize().x;
		} else {
			if (!child->fractionWidth) {
				sizeForInheritance.x -= (child->size.x + child->margin.x * 2.0f) * child->scale;
			}
		}
	}
	for (Widget* child : children) {
		child->UpdateSize(sizeForInheritance, scale);
		vec2 childSize = child->GetSize();
		if (size.x == 0.0f) {
			sizeAbsolute.x += childSize.x;
		}
		if (size.y == 0.0f) {
			sizeAbsolute.y = max(sizeAbsolute.y, childSize.y + totalPadding.y);
		}
	}
	LimitSize();
}

void ListH::Update(vec2 pos, bool selected) {
	pos += (margin + position) * scale;
	positionAbsolute = pos;
	const bool mouseSelect = UpdateSelection(selected, {KC_GP_BTN_A, KC_KEY_ENTER}, {KC_GP_BTN_B, KC_KEY_ESC}, {KC_GP_AXIS_LS_RIGHT, KC_KEY_RIGHT}, {KC_GP_AXIS_LS_LEFT, KC_KEY_LEFT});
	pos += padding * scale;
	if (mouseSelect) {
		f32 childX = pos.x;
		for (selection = 0; selection < children.size; selection++) {
			Widget *child = children[selection];
			if (child->Selectable()) {
				child->positionAbsolute.x = childX + child->margin.x * child->scale;
				child->positionAbsolute.y = pos.y + child->margin.y * child->scale;
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

void Switch::UpdateSize(vec2 container, f32 _scale) {
	scale = _scale;
	if (open) {
		ListV::UpdateSize(container, scale);
	} else {
		sizeAbsolute = vec2(0.0f);
		vec2 totalMargin = margin * 2.0f * scale;
		vec2 totalPadding = padding * 2.0f * scale;
		if (size.x > 0.0f) {
			sizeAbsolute.x = fractionWidth ? (container.x * size.x - totalMargin.x) : size.x * scale;
		} else {
			sizeAbsolute.x = totalPadding.x;
		}
		if (size.y > 0.0f) {
			sizeAbsolute.y = fractionHeight ? (container.y * size.y - totalMargin.y) : size.y * scale;
		} else {
			sizeAbsolute.y = totalPadding.y;
		}
		LimitSize();
		Widget *child = children[choice];
		vec2 sizeForInheritance = sizeAbsolute - totalPadding;
		if (size.x == 0.0f) {
			child->UpdateSize(sizeForInheritance, scale);
			vec2 childSize = child->GetSize();
			sizeAbsolute.x = max(sizeAbsolute.x, childSize.x + totalPadding.x);
		}
		sizeForInheritance = sizeAbsolute - totalPadding;
		if (child->size.y == 0.0f) {
			child->UpdateSize(sizeForInheritance, scale);
			sizeForInheritance.y -= child->GetSize().y;
		} else {
			if (!child->fractionHeight) {
				sizeForInheritance.y -= child->size.y + child->margin.y * 2.0f * child->scale;
			}
		}
		child->UpdateSize(sizeForInheritance, scale);
		vec2 childSize = child->GetSize();
		if (size.x == 0.0f) {
			sizeAbsolute.x = max(sizeAbsolute.x, childSize.x + totalPadding.x);
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
		if (sys->Released(KC_MOUSE_LEFT) || sys->Released(KC_GP_BTN_A) || sys->Released(KC_KEY_ENTER)) {
			if (selection >= 0) {
				choice = selection;
				changed = true;
			}
			open = false;
		}
		if (sys->Released(KC_GP_BTN_B) || sys->Released(KC_KEY_ESC)) {
			open = false;
		}
		if (!open) {
			guiBasic->controlDepth = parentDepth;
		}
	} else {
		pos += (margin + position) * scale;
		highlighted = selected;
		positionAbsolute = pos;
		pos += padding * scale;
		if (sys->Pressed(KC_MOUSE_LEFT) && MouseOver()) {
			open = true;
		}
		if (selected && (sys->Released(KC_GP_BTN_A) || sys->Released(KC_KEY_ENTER))) {
			open = true;
		}
		if (open) {
			guiBasic->controlDepth = depth;
			selection = choice;
		}
		children[choice]->Update(pos, selected);
	}
}

void Switch::Draw(Rendering::DrawingContext &context) const {
	if (color.a > 0.0f) {
		sys->rendering.DrawQuad(context, positionAbsolute * guiBasic->scale, sizeAbsolute * guiBasic->scale, 1.0f, 0.0f, 0.0f, Rendering::PIPELINE_BASIC_2D, (highlighted && !open) ? highlight : color);
	}
	PushScissor(context);
	if (open) {
		if (selection >= 0 && select.a > 0.0f) {
			Widget *child = children[selection];
			vec2 selectionPos = child->positionAbsolute - child->margin;
			vec2 selectionSize = child->sizeAbsolute + child->margin * 2.0f;
			sys->rendering.DrawQuad(context, selectionPos * guiBasic->scale, selectionSize * guiBasic->scale, 1.0f, 0.0f, 0.0f, Rendering::PIPELINE_BASIC_2D, select);
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
	guiBasic->controlDepth = parentDepth;
}

Text::Text() : stringFormatted(), string(), padding(0.1f), fontSize(32.0f), fontIndex(1), bold(false), paddingEM(true), alignH(Rendering::LEFT), alignV(Rendering::TOP), color(vec3(1.0f), 1.0f), colorOutline(vec3(0.0f), 1.0f), highlight(vec3(0.0f), 1.0f), highlightOutline(vec3(1.0f), 1.0f), outline(false) {
	size.y = 0.0f;
}

void Text::PushScissor(Rendering::DrawingContext &context) const {
	if (sizeAbsolute.x != 0.0f && sizeAbsolute.y != 0.0f) {
		vec2i topLeft = vec2i(
			i32((positionAbsolute.x - margin.x * scale) * guiBasic->scale),
			i32((positionAbsolute.y - margin.y * scale) * guiBasic->scale)
		);
		vec2i botRight = vec2i(
			(i32)ceil((positionAbsolute.x + margin.x * scale + sizeAbsolute.x) * guiBasic->scale),
			(i32)ceil((positionAbsolute.y + margin.y * scale + sizeAbsolute.y) * guiBasic->scale)
		);
		sys->rendering.PushScissor(context, topLeft, botRight);
	}
}

void Text::UpdateSize(vec2 container, f32 _scale) {
	scale = _scale;
	vec2 totalMargin = margin * 2.0f * scale;
	vec2 totalPadding = padding * 2.0f * scale;
	if (size.x > 0.0f) {
		sizeAbsolute.x = fractionWidth ? (container.x * size.x - totalMargin.x) : size.x * scale;
	} else {
		sizeAbsolute.x = sys->rendering.StringWidth(stringFormatted, fontIndex) * fontSize + totalPadding.x * (paddingEM ? fontSize : 1.0f);
	}
	if (size.y > 0.0f) {
		sizeAbsolute.y = fractionHeight ? (container.y * size.y - totalMargin.y) : size.y;
	} else {
		sizeAbsolute.y = Rendering::StringHeight(stringFormatted) * fontSize + totalPadding.y * (paddingEM ? fontSize : 1.0f);
	}
	LimitSize();
}

void Text::Update(vec2 pos, bool selected) {
	if (size.x != 0.0f) {
		stringFormatted = sys->rendering.StringAddNewlines(string, fontIndex, sizeAbsolute.x / fontSize);
	} else {
		stringFormatted = string;
	}
	Widget::Update(pos, selected);
}

void Text::Draw(Rendering::DrawingContext &context) const {
	PushScissor(context);
	vec2 paddingAbsolute = padding;
	if (paddingEM) paddingAbsolute *= fontSize;
	vec2 drawPos = (positionAbsolute + paddingAbsolute) * guiBasic->scale;
	vec2 textScale = vec2(fontSize) * guiBasic->scale * scale;
	vec2 textArea = (sizeAbsolute - paddingAbsolute * 2.0f) * guiBasic->scale;
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
		vec4 bg = highlighted? highlightOutline : colorOutline;
		sys->rendering.DrawText(context, stringFormatted, fontIndex, bg, drawPos, textScale, alignH, alignV, textArea.x, 0.05f, bounds - 0.325f - clamp((1.0f - (bg.r + bg.g + bg.b) / 3.0f) * 2.0f, 0.0f, 2.0f)/textScale.y);
	}
	vec4 fg = highlighted? highlight : color;
	bounds -= clamp((1.0f - (fg.r + fg.g + fg.b) / 3.0f) * 2.0f, 0.0f, 2.0f) / textScale.y;
	sys->rendering.DrawText(context, stringFormatted, fontIndex, fg, drawPos, textScale, alignH, alignV, textArea.x, 0.0f, bounds);
	PopScissor(context);
}

Image::Image() : texIndex(0), pipeline(Rendering::PIPELINE_BASIC_2D), color(vec4(1.0f)) { occludes = true; }

void Image::Draw(Rendering::DrawingContext &context) const {
	sys->rendering.DrawQuad(context, positionAbsolute * guiBasic->scale, sizeAbsolute * guiBasic->scale, 1.0f, 0.0f, 0.0f, pipeline, color, texIndex);
}

Text* Button::AddDefaultText(WString string) {
	AzAssert(children.size == 0, "Buttons can only have 1 child");
	Text *buttonText = new Text();
	buttonText->alignH = Rendering::FontAlign::CENTER;
	buttonText->alignV = Rendering::FontAlign::CENTER;
	buttonText->fontIndex = guiBasic->fontIndex;
	buttonText->fontSize = 28.0f;
	buttonText->color = vec4(vec3(1.0f), 1.0f);
	buttonText->highlight = vec4(vec3(0.0f), 1.0f);
	buttonText->size.y = 1.0f;
	buttonText->fractionHeight = true;
	buttonText->padding = 0.0f;
	buttonText->margin = 0.0f;
	buttonText->string = string;
	AddWidget(this, buttonText);
	return buttonText;
}

Button::Button() : padding(0.0f), colorBG(vec3(0.15f), 0.9f), highlightBG(colorHighlightMedium, 0.9f), state(), keycodeActivators() {
	selectable = true;
	occludes = true;
}

void Button::UpdateSize(vec2 container, f32 _scale) {
	scale = _scale;
	sizeAbsolute = vec2(0.0f);
	f32 childScale = state.Down() ? 0.9f : 1.0f;
	vec2 totalMargin = margin * 2.0f * scale;
	vec2 totalPadding = padding * 2.0f * scale;
	if (size.x > 0.0f) {
		sizeAbsolute.x = fractionWidth ? (container.x * size.x - totalMargin.x) : size.x * scale;
	} else {
		sizeAbsolute.x = totalPadding.x;
	}
	if (size.y > 0.0f) {
		sizeAbsolute.y = fractionHeight ? (container.y * size.y - totalMargin.y) : size.y * scale;
	} else {
		sizeAbsolute.y = totalPadding.y;
	}
	LimitSize();
	if (children.size) {
		Widget *child = children[0];
		vec2 sizeForInheritance = sizeAbsolute - totalPadding;
		if (size.x == 0.0f || size.y == 0.0f) {
			child->UpdateSize(sizeForInheritance, scale);
			vec2 childSize = child->GetSize();
			if (size.x == 0.0f) {
				sizeAbsolute.x = max(sizeAbsolute.x, childSize.x + totalPadding.x);
			}
			if (size.y == 0.0f) {
				sizeAbsolute.y = max(sizeAbsolute.y, childSize.y + totalPadding.y);
			}
			sizeForInheritance = sizeAbsolute - totalPadding;
		}
		child->UpdateSize(sizeForInheritance * childScale, childScale * scale);
		LimitSize();
	}
}

void Button::Update(vec2 pos, bool selected) {
	pos += (margin + position) * scale;
	f32 childScale = state.Down() ? 0.9f : 1.0f;
	positionAbsolute = pos;
	pos += padding * scale;
	highlighted = selected;
	{
		bool mouseoverNew = MouseOver();
		if (mouseoverNew && !mouseover) {
			guiBasic->sndClickSoft.Play();
		}
		if (!mouseoverNew && mouseover) {
			// Mouse leave should prevent clicking
			state.Set(false, false, false);
		}
		mouseover = mouseoverNew;
	}
	if (children.size) {
		Widget *child = children[0];
		child->Update(pos + (1.0f - childScale) * sizeAbsolute * 0.5f, selected || mouseover || state.Down());
	}
	state.Tick(0.0f);
	if (mouseover) {
		if (sys->Pressed(KC_MOUSE_LEFT)) {
			state.Press();
		}
		if (sys->Released(KC_MOUSE_LEFT) && state.Down()) {
			state.Release();
		}
	}
	if (guiBasic->controlDepth == depth) {
		if (selected) {
			if (sys->Pressed(KC_GP_BTN_A) || sys->Pressed(KC_KEY_ENTER)) {
				state.Press();
			}
			if (sys->Released(KC_GP_BTN_A) || sys->Released(KC_KEY_ENTER)) {
				state.Release();
			}
		}
		for (u8 kc : keycodeActivators) {
			if (sys->Pressed(kc)) {
				state.Press();
			}
			if (sys->Released(kc)) {
				state.Release();
			}
		}
	}
	if (state.Pressed()) {
		guiBasic->sndClickIn.Play();
	}
	if (state.Released()) {
		guiBasic->sndClickOut.Play();
	}
	highlighted = selected || mouseover || state.Down();
}

void Button::Draw(Rendering::DrawingContext &context) const {
	PushScissor(context);
	f32 childScale = state.Down() ? 0.9f : 1.0f;
	sys->rendering.DrawQuad(context, positionAbsolute * guiBasic->scale + sizeAbsolute * guiBasic->scale * 0.5f, vec2(1.0f), sizeAbsolute * guiBasic->scale * childScale, vec2(0.5f), 0.0f, Rendering::PIPELINE_BASIC_2D, highlighted ? highlightBG : colorBG);
	if (children.size) {
		Widget *child = children[0];
		child->Draw(context);
	}
	// sys->rendering.DrawText(context, string, fontIndex,  highlighted ? highlightText : colorText, drawPos, vec2(fontSize * scale), Rendering::CENTER, Rendering::CENTER, sizeAbsolute.x * guiBasic->scale);
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
	if (guiBasic->controlDepth != depth) {
		highlighted = false;
	}
	if (mouseover) {
		highlighted = true;
		if (sys->Released(KC_MOUSE_LEFT)) {
			checked = !checked;
			if (checked) {
				guiBasic->sndCheckboxOn.Play();
			} else {
				guiBasic->sndCheckboxOff.Play();
			}
		}
	}
	if (guiBasic->controlDepth == depth) {
		if (selected) {
			if (sys->Released(KC_GP_BTN_A) || sys->Released(KC_KEY_ENTER)) {
				checked = !checked;
				if (checked) {
					guiBasic->sndCheckboxOn.Play();
				} else {
					guiBasic->sndCheckboxOff.Play();
				}
			}
		}
	}
	if (checked) {
		transition = decay(transition, 1.0f, 0.05f, sys->timestep);
	} else {
		transition = decay(transition, 0.0f, 0.05f, sys->timestep);
	}
}

void Checkbox::Draw(Rendering::DrawingContext &context) const {
	const vec4 &colorOnActual = highlighted ? highlightOn : colorOn;
	const vec4 &colorOffActual = highlighted ? highlightOff : colorOff;
	vec4 colorActual = lerp(colorOffActual, colorOnActual, transition);
	vec2 switchPos = (positionAbsolute + sizeAbsolute * vec2(lerp(0.25f, 0.75f, transition), 0.5f)) * guiBasic->scale;
	sys->rendering.DrawQuad(context, positionAbsolute * guiBasic->scale, sizeAbsolute * guiBasic->scale, 1.0f, 0.0f, 0.0f, Rendering::PIPELINE_BASIC_2D, colorActual);
	sys->rendering.DrawQuad(context, switchPos, (sizeAbsolute * vec2(0.375f, 0.75f)) * guiBasic->scale, 1.0f, 0.5f, -halfpi * transition, Rendering::PIPELINE_BASIC_2D, vec4(vec3(0.0f), 0.8f));
}

TextBox::TextBox() : string(), stringFormatted(), stringSuffix(), colorBG(vec3(0.15f), 0.9f), highlightBG(vec3(0.2f), 0.9f), errorBG(0.1f, 0.0f, 0.0f, 0.9f), colorText(vec3(1.0f), 1.0f), highlightText(vec3(1.0f), 1.0f), errorText(1.0f, 0.5f, 0.5f, 1.0f), padding(2.0f), cursor(0), fontIndex(1), fontSize(17.39f), cursorBlinkTimer(0.0f), alignH(Rendering::LEFT), textFilter(TextFilterBasic), textValidate(TextValidateAll), entry(false), multiline(false) {
	selectable = true;
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

bool TextValidateDecimalsNegative(const WString &string) {
	if (string.size == 0) return false;
	if (string[0] != '-') return false;
	if (string.size == 1 && (string[0] == '.' || string[0] == '-')) return false;
	if (string.size == 2 && (string[0] == '-' && string[1] == '.')) return false;
	i32 cur = 1;
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

bool TextValidateDecimalsNegativeAndInfinity(const WString &string) {
	static WString negInfinity = ToWString("-Infinity");
	if (string == negInfinity) return true;
	if (string.size == 0) return false;
	if (string[0] != '-') return false;
	if (string.size == 1 && (string[0] == '.' || string[0] == '-')) return false;
	if (string.size == 2 && (string[0] == '-' && string[1] == '.')) return false;
	i32 cur = 1;
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
	f32 spaceScale, spaceWidth, tabWidth;
	spaceWidth = sys->assets.CharacterWidth(' ', fontIndex) * fontSize * scale;
	tabWidth = sys->assets.CharacterWidth((char32)'_', fontIndex) * fontSize * scale * 4.0f;
	const char32 *lineString = stringFormatted.data;
	i32 formatNewlines = 0;
	cursor = 0;
	cursorPos.y += fontSize * Rendering::lineHeight * scale + positionAbsolute.y + padding.y * scale;
	if (cursorPos.y <= position.y / guiBasic->scale) {
		for (; cursor < stringFormatted.size-stringSuffix.size; cursor++) {
			const char32 &c = stringFormatted[cursor];
			if (c == '\n') {
				if (string[cursor-formatNewlines] != '\n' && string[cursor-formatNewlines] != ' ' && string[cursor-formatNewlines] != '\t') {
					formatNewlines++;
				}
				lineString = &c+1;
				cursorPos.y += fontSize * Rendering::lineHeight * scale;
				if (cursorPos.y > position.y / guiBasic->scale) {
					cursor++;
					break;
				}
			}
		}
	}
	sys->rendering.LineCursorStartAndSpaceScale(cursorPos.x, spaceScale, fontSize * scale, spaceWidth, fontIndex, lineString, sizeAbsolute.x - padding.x * 2.0f * scale, alignH);
	cursorPos.x += positionAbsolute.x + padding.x;
	if (alignH == Rendering::CENTER) {
		cursorPos.x += sizeAbsolute.x * 0.5f - padding.x * scale;
	} else if (alignH == Rendering::RIGHT) {
		cursorPos.x += sizeAbsolute.x - padding.x * 2.0f * scale;
	}
	cursorPos *= guiBasic->scale;
	spaceWidth *= spaceScale * guiBasic->scale;
	f32 advanceCarry;
	for (; cursor < stringFormatted.size-stringSuffix.size; cursor++) {
		const char32 &c = stringFormatted[cursor];
		if (c == '\n') {
			break;
		} else if (c == '\t') {
			advanceCarry = (ceil((cursorPos.x-positionAbsolute.x)/tabWidth+0.05f) * tabWidth - (cursorPos.x-positionAbsolute.x)) * 0.5f;
		} else if (c == ' ') {
			advanceCarry = spaceWidth * 0.5f;
		} else {
			advanceCarry = sys->assets.CharacterWidth(c, fontIndex) * fontSize * scale * guiBasic->scale * 0.5f;
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
	f32 spaceScale, spaceWidth, tabWidth;
	spaceWidth = sys->assets.CharacterWidth(' ', fontIndex) * fontSize * scale;
	tabWidth = sys->assets.CharacterWidth((char32)'_', fontIndex) * fontSize * scale * 4.0f;
	const char32 *lineString = stringFormatted.data;
	i32 lineStart = 0;
	i32 formatNewlines = 0;
	for (i32 i = 0; i < cursor+formatNewlines; i++) {
		const char32 &c = stringFormatted[i];
		if (c == '\n') {
			if (string[i-formatNewlines] != '\n' && string[i-formatNewlines] != ' ' && string[i-formatNewlines] != '\t') {
				formatNewlines++;
			}
			cursorPos.y += fontSize * Rendering::lineHeight * scale;
			lineString = &c+1;
			lineStart = i+1;
		}
	}
	sys->rendering.LineCursorStartAndSpaceScale(cursorPos.x, spaceScale, fontSize * scale, spaceWidth, fontIndex, lineString, sizeAbsolute.x - padding.x * 2.0f * scale, alignH);
	spaceWidth *= spaceScale;
	for (i32 i = lineStart; i < cursor+formatNewlines; i++) {
		const char32 &c = stringFormatted[i];
		if (c == '\n') {
			break;
		} if (c == '\t') {
			cursorPos.x = ceil((cursorPos.x)/tabWidth+0.05f) * tabWidth;
			continue;
		}
		if (c == ' ') {
			cursorPos.x += spaceWidth;
		} else {
			cursorPos.x += sys->assets.CharacterWidth(c, fontIndex) * fontSize * scale;
		}
	}
	if (alignH == Rendering::CENTER) {
		cursorPos.x += sizeAbsolute.x * 0.5f - padding.x * scale;
	} else if (alignH == Rendering::RIGHT) {
		cursorPos.x += sizeAbsolute.x - padding.x * scale * 2.0f;
	}
	cursorPos += positionAbsolute + padding * scale;
	cursorPos *= guiBasic->scale;
	return cursorPos;
}

void TextBox::UpdateSize(vec2 container, f32 _scale) {
	scale = _scale;
	vec2 totalMargin = margin * 2.0f * scale;
	vec2 totalPadding = padding * 2.0f * scale;
	if (size.x > 0.0f) {
		sizeAbsolute.x = fractionWidth ? (container.x * size.x - totalMargin.x) : size.x * scale;
	} else {
		sizeAbsolute.x = sys->rendering.StringWidth(stringFormatted, fontIndex) * fontSize * scale + totalPadding.x;
	}
	if (size.y > 0.0f) {
		sizeAbsolute.y = fractionHeight ? (container.y * size.y - totalMargin.y) : size.y * scale;
	} else {
		sizeAbsolute.y = Rendering::StringHeight(stringFormatted) * fontSize * scale + totalPadding.y;
	}
	LimitSize();
}

void TextBox::Update(vec2 pos, bool selected) {
	bool stoppedEntry = false;
	if (entry) {
		cursorBlinkTimer += sys->timestep;
		if (cursorBlinkTimer > 1.0f) {
			cursorBlinkTimer -= 1.0f;
		}
		highlighted = true;
		if (sys->input.AnyKey.Pressed()) {
			for (i32 i = 0; i < sys->input.typingString.size; i++) {
				const char32 c = sys->input.typingString[i];
				if (textFilter(c)) {
					string.Insert(cursor, c);
					cursorBlinkTimer = 0.0f;
					cursor++;
				}
			}
		}
		sys->input.typingString.Clear();
		if (sys->input.Repeated(KC_KEY_BACKSPACE)) {
			if (cursor <= string.size && cursor > 0) {
				string.Erase(cursor-1);
				cursorBlinkTimer = 0.0f;
				cursor--;
			}
		}
		if (sys->input.Repeated(KC_KEY_DELETE)) {
			if (cursor < string.size) {
				string.Erase(cursor);
				cursorBlinkTimer = 0.0f;
			}
		}
		if (sys->input.Pressed(KC_KEY_HOME)) {
			if (sys->input.Down(KC_KEY_LEFTCTRL) || sys->input.Down(KC_KEY_RIGHTCTRL) || !multiline) {
				cursor = 0;
			} else {
				for (cursor--; cursor >= 0; cursor--) {
					if (string[cursor] == '\n') break;
				}
				cursor++;
			}
			cursorBlinkTimer = 0.0f;
		}
		if (sys->input.Pressed(KC_KEY_END)) {
			if (sys->input.Down(KC_KEY_LEFTCTRL) || sys->input.Down(KC_KEY_RIGHTCTRL) || !multiline) {
				cursor = string.size;
			} else {
				for (; cursor < string.size; cursor++) {
					if (string[cursor] == '\n') break;
				}
			}
			cursorBlinkTimer = 0.0f;
		}
		if (sys->input.Repeated(KC_KEY_TAB)) {
			string.Insert(cursor, '\t');
			cursor++;
			cursorBlinkTimer = 0.0f;
		}
		if (multiline) {
			if (sys->input.Repeated(KC_KEY_ENTER)) {
				string.Insert(cursor, '\n');
				cursor++;
				cursorBlinkTimer = 0.0f;
			}
			if (sys->input.Repeated(KC_KEY_UP)) {
				vec2 cursorPos = PositionFromCursor();
				cursorPos.y -= fontSize * guiBasic->scale * Rendering::lineHeight * 0.5f;
				CursorFromPosition(cursorPos);
				cursorBlinkTimer = 0.0f;
			}
			if (sys->input.Repeated(KC_KEY_DOWN)) {
				vec2 cursorPos = PositionFromCursor();
				cursorPos.y += fontSize * guiBasic->scale * Rendering::lineHeight * 1.5f;
				CursorFromPosition(cursorPos);
				cursorBlinkTimer = 0.0f;
			}
		}
		if (sys->input.Repeated(KC_KEY_LEFT)) {
			cursorBlinkTimer = 0.0f;
			if (sys->input.Down(KC_KEY_LEFTCTRL) || sys->input.Down(KC_KEY_RIGHTCTRL)) {
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
		if (sys->input.Repeated(KC_KEY_RIGHT)) {
			cursorBlinkTimer = 0.0f;
			if (sys->input.Down(KC_KEY_LEFTCTRL) || sys->input.Down(KC_KEY_RIGHTCTRL)) {
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
		if (!multiline && sys->input.Released(KC_KEY_ENTER)) {
			entry = false;
			stoppedEntry = true;
			if (guiBasic->controlDepth == depth+1) {
				guiBasic->controlDepth = depth;
			}
		}
	}
	if (size.x != 0.0f && multiline) {
		stringFormatted = sys->rendering.StringAddNewlines(string + stringSuffix, fontIndex, (sizeAbsolute.x - padding.x * 2.0f * scale) / fontSize);
	} else {
		stringFormatted = string + stringSuffix;
	}
	Widget::Update(pos, selected);
	bool mouseover = MouseOver();
	if (guiBasic->controlDepth != depth) {
		highlighted = false;
	}
	if (mouseover) {
		highlighted = true;
	}
	if (sys->Pressed(KC_MOUSE_LEFT)) {
		if (mouseover) {
			if (guiBasic->controlDepth == depth) {
				guiBasic->controlDepth = depth+1;
			}
			const vec2 mouse = vec2(sys->input.cursor);
			CursorFromPosition(mouse);
			cursorBlinkTimer = 0.0f;
		}
		if (!mouseover && entry && guiBasic->controlDepth == depth+1) {
			guiBasic->controlDepth = depth;
			entry = false;
		} else {
			entry = mouseover;
		}
	}
	if (guiBasic->controlDepth == depth) {
		if (selected) {
			if ((sys->Released(KC_GP_BTN_A) || sys->Released(KC_KEY_ENTER)) && !stoppedEntry) {
				entry = true;
				guiBasic->controlDepth++;
			} else {
				entry = false;
			}
		}
	} else if (guiBasic->controlDepth == depth+1) {
		if (selected) {
			if (sys->Released(KC_GP_BTN_B) || sys->Released(KC_KEY_ESC)) {
				// Prevent esc bleeding
				// sys->input.inputs[KC_KEY_ESC].Set(false, false, false);
				entry = false;
				guiBasic->controlDepth--;
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
	vec2 drawPosText = (positionAbsolute + padding * scale) * guiBasic->scale;
	vec2 textScale = vec2(fontSize * guiBasic->scale) * scale;
	vec2 textArea = (sizeAbsolute - padding * 2.0f * scale) * guiBasic->scale;
	if (alignH == Rendering::CENTER) {
		drawPosText.x += textArea.x * 0.5f;
	} else if (alignH == Rendering::RIGHT) {
		drawPosText.x += textArea.x;
	}
	vec2 drawPos = positionAbsolute * guiBasic->scale;
	sys->rendering.DrawQuad(context, drawPos, sizeAbsolute * guiBasic->scale, 1.0f, 0.0f, 0.0f, Rendering::PIPELINE_BASIC_2D, bg);
	sys->rendering.DrawText(context, stringFormatted, fontIndex, text, drawPosText, textScale, alignH, Rendering::TOP, textArea.x);
	if (cursorBlinkTimer < 0.5f && entry) {
		vec2 cursorPos = PositionFromCursor();
		cursorPos.y += fontSize * guiBasic->scale * 0.6f * scale;
		sys->rendering.DrawQuad(context, cursorPos, vec2(ceil(guiBasic->scale), guiBasic->scale), vec2(1.0f, fontSize * Rendering::lineHeight * 0.9f * scale), 0.5f, 0.0f, Rendering::PIPELINE_BASIC_2D, text);
	}
	PopScissor(context);
}

Slider::Slider() :
value(1.0f),
valueMin(0.0f),                 valueMax(1.0f),
valueTick(-0.1f),               valueTickShiftMult(0.1f),
minOverride(false),             minOverrideValue(0.0f),
maxOverride(false),             maxOverrideValue(1.0f),
mirror(nullptr),
colorBG(vec3(0.15f), 0.9f),     colorSlider(colorHighlightMedium, 1.0f),
highlightBG(vec3(0.2f), 0.9f),  highlightSlider(colorHighlightHigh, 1.0f),
grabbed(false), left(), right()
{
	occludes = true;
	selectable = true;
}

void Slider::Update(vec2 pos, bool selected) {
	Widget::Update(pos, selected);
	mouseover = MouseOver();
	f32 knobSize = 16.0f * scale;
	left.Tick(sys->timestep);
	right.Tick(sys->timestep);
	if (selected && guiBasic->controlDepth == depth) {
		bool held = sys->Down(KC_MOUSE_LEFT);
		bool leftHeld = held || sys->Down(KC_GP_AXIS_LS_LEFT) || sys->Down(KC_KEY_LEFT);
		bool rightHeld = held || sys->Down(KC_GP_AXIS_LS_RIGHT) || sys->Down(KC_KEY_RIGHT);
		if (sys->Pressed(KC_GP_AXIS_LS_LEFT) || sys->Pressed(KC_KEY_LEFT)) {
			left.Press();
		} else if (left.Down() && !leftHeld) {
			left.Release();
		}
		if (sys->Pressed(KC_GP_AXIS_LS_RIGHT) || sys->Pressed(KC_KEY_RIGHT)) {
			right.Press();
		} else if (right.Down() && !rightHeld) {
			right.Release();
		}
	}
	if (mouseover && !grabbed) {
		i32 mousePos = 0;
		f32 mouseX = (f32)sys->input.cursor.x / guiBasic->scale - positionAbsolute.x;
		f32 sliderX = map(value, valueMin, valueMax, 0.0f, sizeAbsolute.x - knobSize);
		if (mouseX < sliderX) {
			mousePos = -1;
		} else if (mouseX > sliderX+knobSize) {
			mousePos = 1;
		}
		if (sys->Pressed(KC_MOUSE_LEFT)) {
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
	f32 scale = (valueMax - valueMin) / (sizeAbsolute.x - knobSize);
	if (grabbed) {
		f32 moved = f32(sys->input.cursor.x - sys->input.cursorPrevious.x) / guiBasic->scale * scale;
		if (sys->Down(KC_KEY_LEFTSHIFT)) {
			moved /= 10.0f;
		}
		if (moved != 0.0f) updated = true;
		value = clamp(value + moved, valueMin, valueMax);
	}
	scale = valueTick >= 0.0f ? valueTick : (valueMax - valueMin) * -valueTick;
	if (sys->Down(KC_KEY_LEFTSHIFT)) {
		scale *= valueTickShiftMult;
	}
	if (right.Repeated()) {
		value = clamp(value + scale, valueMin, valueMax);
		updated = true;
	}
	if (left.Repeated()) {
		value = clamp(value - scale, valueMin, valueMax);
		updated = true;
	}
	if (sys->Released(KC_MOUSE_LEFT)) {
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
			UpdateMirror();
		} else if (mirror->entry) {
			if (mirror->textValidate(mirror->string)) {
				WStringToF32(mirror->string, &value);
				value = clamp(value, valueMin, valueMax);
			}
		}
	}
}

void Slider::Draw(Rendering::DrawingContext &context) const {
	f32 knobSize = 16.0f * scale;
	vec4 bg = highlighted ? highlightBG : colorBG;
	vec4 slider = highlighted ? highlightSlider : colorSlider;
	vec2 drawPos = positionAbsolute * guiBasic->scale;
	sys->rendering.DrawQuad(context, drawPos, sizeAbsolute * guiBasic->scale, 1.0f, 0.0f, 0.0f, Rendering::PIPELINE_BASIC_2D, bg);
	drawPos.x += map(value, valueMin, valueMax, 2.0f * scale, sizeAbsolute.x - knobSize) * guiBasic->scale;
	drawPos.y += 2.0f * guiBasic->scale * scale;
	sys->rendering.DrawQuad(context, drawPos, vec2(12.0f * scale, sizeAbsolute.y - 4.0f * scale) * guiBasic->scale, 1.0f, 0.0f, 0.0f, Rendering::PIPELINE_BASIC_2D, slider);
}

void Slider::SetValue(f32 newValue) {
	value = clamp(newValue, valueMin, valueMax);
}

f32 Slider::GetActualValue() {
	f32 actualValue;
	if (minOverride && value == valueMin) {
		actualValue = minOverrideValue;
	} else if (maxOverride && value == valueMax) {
		actualValue = maxOverrideValue;
	} else {
		actualValue = value;
	}
	return actualValue;
}

void Slider::UpdateMirror() {
	f32 actualValue = GetActualValue();
	mirror->string = ToWString(ToString(actualValue, 10, 1));
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
}

Hideable::Hideable(Widget *child) : hidden(false), hiddenPrev(false) {
	margin = 0.0f;
	AddWidget(this, child);
	size = child->size; // We need to inherit this for Lists to work properly
	fractionWidth = child->fractionWidth;
	fractionHeight = child->fractionHeight;
	occludes = child->occludes;
	selectable = child->selectable;
}

void Hideable::UpdateSize(vec2 container, f32 _scale) {
	scale = _scale;
	if (hidden) {
		sizeAbsolute = 0.0f;
	} else {
		children[0]->UpdateSize(container, scale);
		scale = children[0]->scale;
		sizeAbsolute = children[0]->GetSize();
	}
}

void Hideable::Update(vec2 pos, bool selected) {
	if (!hidden) {
		children[0]->Update(pos + position * scale, selected);
		positionAbsolute = children[0]->positionAbsolute;
		selectable = children[0]->selectable;
	}
	if (hidden && !hiddenPrev) {
		selectable = false;
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

} // namespace Az2D::Gui

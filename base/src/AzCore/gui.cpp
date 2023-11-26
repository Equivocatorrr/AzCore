/*
	File: gui.cpp
	Author: Philip Haynes
*/

#include "gui.hpp"
#include "Profiling.hpp"

namespace AzCore::GuiGeneric {

#define CALL_EVENT_FUNCTION(event) if(_system->functions.event) _system->functions.event(_system->data, &data);

static bool IsWidgetIsAlreadyTracked(System *system, Widget *widget) {
	for (UniquePtr<Widget> &w : system->_allWidgets) {
		if (w.RawPtr() == widget) {
			return true;
		}
	}
	return false;
}

void System::AddWidget(Widget *parent, Widget *newWidget, bool deeper) {
	AzAssert(!IsWidgetIsAlreadyTracked(this, newWidget), "Widget already tracked");
	_allWidgets.Append(newWidget);
	newWidget->_system = this;
	if (parent) {
		newWidget->depth = parent->depth + (deeper ? 1 : 0);
		parent->children.Append(newWidget);
	}
}

void System::AddWidget(Widget *parent, Switch *newWidget) {
	AzAssert(!IsWidgetIsAlreadyTracked(this, newWidget), "Widget already tracked");
	_allWidgets.Append(newWidget);
	newWidget->_system = this;
	if (parent) {
		newWidget->depth = parent->depth + 1;
		newWidget->parentDepth = parent->depth;
		parent->children.Append(newWidget);
	}
}

void System::AddWidgetAsDefault(List *parent, Widget *newWidget, bool deeper) {
	AzAssert(!IsWidgetIsAlreadyTracked(this, newWidget), "Widget already tracked");
	_allWidgets.Append(newWidget);
	newWidget->_system = this;
	newWidget->depth = parent->depth + (deeper ? 1 : 0);
	parent->selectionDefault = parent->children.size;
	parent->children.Append(newWidget);
}

void System::AddWidgetAsDefault(List *parent, Switch *newWidget) {
	AzAssert(!IsWidgetIsAlreadyTracked(this, newWidget), "Widget already tracked");
	_allWidgets.Append(newWidget);
	newWidget->_system = this;
	newWidget->depth = parent->depth + 1;
	newWidget->parentDepth = parent->depth;
	parent->selectionDefault = parent->children.size;
	parent->children.Append(newWidget);
}

Screen* System::CreateScreen() {
	Screen *result = new Screen();
	_allWidgets.Append(result);
	result->_system = this;
	return result;
}

Spacer* System::CreateSpacer(Widget *parent, bool deeper) {
	Spacer *result = new Spacer(defaults.spacer);
	AddWidget(parent, result, deeper);
	return result;
}

ListV* System::CreateListV(Widget *parent, bool deeper) {
	ListV *result = new ListV(defaults.listV);
	AddWidget(parent, result, deeper);
	return result;
}

ListH* System::CreateListH(Widget *parent, bool deeper) {
	ListH *result = new ListH(defaults.listH);
	AddWidget(parent, result, deeper);
	return result;
}

Switch* System::CreateSwitch(Widget *parent) {
	Switch *result = new Switch(defaults.switch_);
	AddWidget(parent, result);
	return result;
}

Text* System::CreateText(Widget *parent, bool deeper) {
	Text *result = new Text(defaults.text);
	AddWidget(parent, result, deeper);
	return result;
}

Image* System::CreateImage(Widget *parent, bool deeper) {
	Image *result = new Image(defaults.image);
	AddWidget(parent, result, deeper);
	return result;
}

Button* System::CreateButton(Widget *parent, bool deeper) {
	Button *result = new Button(defaults.button);
	AddWidget(parent, result, deeper);
	return result;
}

Checkbox* System::CreateCheckbox(Widget *parent, bool deeper) {
	Checkbox *result = new Checkbox(defaults.checkbox);
	AddWidget(parent, result, deeper);
	return result;
}

Textbox* System::CreateTextbox(Widget *parent, bool deeper) {
	Textbox *result = new Textbox(defaults.textbox);
	AddWidget(parent, result, deeper);
	return result;
}

Slider* System::CreateSlider(Widget *parent, bool deeper) {
	Slider *result = new Slider(defaults.slider);
	AddWidget(parent, result, deeper);
	return result;
}

Hideable* System::CreateHideable(Widget *parent, Widget *child, bool deeper) {
	Hideable *result = new Hideable(child);
	AddWidget(parent, result, deeper);
	child->_system = this;
	if (parent) {
		child->depth = parent->depth + (deeper ? 1 : 0);
	}
	return result;
}

Hideable* System::CreateHideable(Widget *parent, Switch *child, bool deeper) {
	Hideable *result = new Hideable(child);
	AddWidget(parent, result, deeper);
	if (parent) {
		child->depth = parent->depth + (deeper ? 1 : 0);
		child->parentDepth = parent->depth;
	}
	return result;
}

Spacer* System::CreateSpacerFrom(Widget *parent, const Spacer &src, bool deeper) {
	Spacer *result = new Spacer(src);
	AddWidget(parent, result, deeper);
	return result;
}

ListV* System::CreateListVFrom(Widget *parent, const ListV &src, bool deeper) {
	ListV *result = new ListV(src);
	AddWidget(parent, result, deeper);
	return result;
}

ListH* System::CreateListHFrom(Widget *parent, const ListH &src, bool deeper) {
	ListH *result = new ListH(src);
	AddWidget(parent, result, deeper);
	return result;
}

Switch* System::CreateSwitchFrom(Widget *parent, const Switch &src) {
	Switch *result = new Switch(src);
	AddWidget(parent, result);
	return result;
}

Text* System::CreateTextFrom(Widget *parent, const Text &src, bool deeper) {
	Text *result = new Text(src);
	AddWidget(parent, result, deeper);
	return result;
}

Image* System::CreateImageFrom(Widget *parent, const Image &src, bool deeper) {
	Image *result = new Image(src);
	AddWidget(parent, result, deeper);
	return result;
}

Button* System::CreateButtonFrom(Widget *parent, const Button &src, bool deeper) {
	Button *result = new Button(src);
	AddWidget(parent, result, deeper);
	return result;
}

Checkbox* System::CreateCheckboxFrom(Widget *parent, const Checkbox &src, bool deeper) {
	Checkbox *result = new Checkbox(src);
	AddWidget(parent, result, deeper);
	return result;
}

Textbox* System::CreateTextboxFrom(Widget *parent, const Textbox &src, bool deeper) {
	Textbox *result = new Textbox(src);
	AddWidget(parent, result, deeper);
	return result;
}

Slider* System::CreateSliderFrom(Widget *parent, const Slider &src, bool deeper) {
	Slider *result = new Slider(src);
	AddWidget(parent, result, deeper);
	return result;
}

void System::Update(vec2 newMouseCursor, vec2 _canvasSize, f32 _timestep) {
	canvasSize = _canvasSize;
	timestep = _timestep;
	mouseoverWidget = nullptr;
	mouseoverDepth = -1;
	if (newMouseCursor != mouseCursor) {
		inputMethod = InputMethod::MOUSE;
	} else if (false /* TODO: Implement this somehow: functions.KeycodePressed(data, nullptr, KC_GP_ANY) */) {
		inputMethod = InputMethod::GAMEPAD;
	} else if (functions.KeycodePressed(data, nullptr, KC_KEY_UP) || functions.KeycodePressed(data, nullptr, KC_KEY_DOWN) || functions.KeycodePressed(data, nullptr, KC_KEY_LEFT) || functions.KeycodePressed(data, nullptr, KC_KEY_RIGHT)) {
		inputMethod = InputMethod::ARROWS;
	}
	mouseCursorPrev = mouseCursor;
	mouseCursor = newMouseCursor;
}


Widget::Widget() : children(), margin(8.0f), size(1.0f), fractionWidth(true), fractionHeight(true), minSize(0.0f), maxSize(-1.0f), position(0.0f), sizeAbsolute(0.0f), positionAbsolute(0.0f), depth(0), selectable(false), highlighted(false), occludes(false), mouseover(false) {}

bool Widget::UpdateSelectable() {
	for (Widget *child : children) {
		bool childSelectable = child->UpdateSelectable();
		selectable = selectable || childSelectable;
	}
	return selectable;
}

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

void Widget::PushScissor() const {
	const Scissor upScissor = _system->stackScissors.Back();
	Scissor scissor;
	scissor.topLeft = vec2i(
		max(upScissor.topLeft.x, i32(positionAbsolute.x * _system->scale)),
		max(upScissor.topLeft.y, i32(positionAbsolute.y * _system->scale))
	);
	scissor.botRight = vec2i(
		min(upScissor.botRight.x, (i32)ceil((positionAbsolute.x + sizeAbsolute.x) * _system->scale)),
		min(upScissor.botRight.y, (i32)ceil((positionAbsolute.y + sizeAbsolute.y) * _system->scale))
	);
	_system->functions.SetScissor(_system->data, const_cast<Any*>(&data), scissor.topLeft, scissor.botRight-scissor.topLeft);
	_system->stackScissors.Append(scissor);
}

void Widget::PushScissor(vec2 pos, vec2 size) const {
	const Scissor upScissor = _system->stackScissors.Back();
	Scissor scissor;
	scissor.topLeft = vec2i(
		max(upScissor.topLeft.x, i32(pos.x * _system->scale)),
		max(upScissor.topLeft.y, i32(pos.y * _system->scale))
	);
	scissor.botRight = vec2i(
		min(upScissor.botRight.x, (i32)ceil((pos.x + size.x) * _system->scale)),
		min(upScissor.botRight.y, (i32)ceil((pos.y + size.y) * _system->scale))
	);
	_system->functions.SetScissor(_system->data, const_cast<Any*>(&data), scissor.topLeft, scissor.botRight-scissor.topLeft);
	_system->stackScissors.Append(scissor);
}

void Widget::PopScissor() const {
	AzAssert(_system->stackScissors.size > 1, "Cannot pop any more scissors!");
	_system->stackScissors.Erase(_system->stackScissors.size-1);
	const Scissor scissor = _system->stackScissors.Back();
	_system->functions.SetScissor(_system->data, const_cast<Any*>(&data), scissor.topLeft, scissor.botRight-scissor.topLeft);
}

void Widget::Update(vec2 pos, bool selected) {
	pos += (margin + position) * scale;
	positionAbsolute = pos;
	if (selected && selectable) {
		_system->selectedCenter = positionAbsolute + sizeAbsolute * 0.5f;
	}
	highlighted = selected;
	for (Widget* child : children) {
		child->Update(pos, selected);
	}
}

void Widget::Draw() const {
	for (const Widget* child : children) {
		child->Draw();
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
	if (_system->inputMethod == InputMethod::MOUSE) {
		mouse = vec2(_system->mouseCursor) / _system->scale;
	} else {
		mouse = -1.0f;
	}
	return mouse.x == median(positionAbsolute.x, mouse.x, positionAbsolute.x + sizeAbsolute.x)
		&& mouse.y == median(positionAbsolute.y, mouse.y, positionAbsolute.y + sizeAbsolute.y);
}

void Widget::FindMouseoverDepth(i32 actualDepth) {
	if (actualDepth <= _system->mouseoverDepth) return;
	if (MouseOver()) {
		if (occludes) {
			_system->mouseoverDepth = actualDepth;
			_system->mouseoverWidget = this;
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
	AZCORE_PROFILING_FUNC_TIMER()
	UpdateSelectable();
	UpdateSize(_system->canvasSize / _system->scale, 1.0f);
	Widget::Update(pos, selected);
	FindMouseoverDepth(0);
}

void Screen::UpdateSize(vec2 container, f32 _scale) {
	AZCORE_PROFILING_FUNC_TIMER()
	scale = _scale;
	sizeAbsolute = container - margin * 2.0f * scale;
	for (Widget* child : children) {
		child->UpdateSize(sizeAbsolute, scale);
	}
}

List::List() : padding(8.0f), color(0.05f, 0.05f, 0.05f, 0.9f), colorHighlighted(0.05f, 0.05f, 0.05f, 0.9f), colorSelection(0.2f, 0.2f, 0.2f, 0.0f), selection(-2), selectionDefault(-1), scroll(0.0f), sizeContents(vec2(0.0f)), scrollableX(false), scrollableY(true) { occludes = true; }

bool List::UpdateSelection(bool selected, StaticArray<u8, 4> keyCodeSelect, StaticArray<u8, 4> keyCodeBack, StaticArray<u8, 4> keyCodeIncrement, StaticArray<u8, 4> keyCodeDecrement) {
	highlighted = selected;
	if (selected) {
		bool select = false;
		for (u8 kc : keyCodeSelect) {
			if (_system->functions.KeycodeReleased(_system->data, &data, kc)) {
				select = true;
				break;
			}
		}
		bool back = false;
		for (u8 kc : keyCodeBack) {
			if (_system->functions.KeycodeReleased(_system->data, &data, kc) && !_system->_goneBack) {
				back = true;
				if (_system->controlDepth > depth) {
					_system->_goneBack = true;
				}
			}
		}
		bool increment = false;
		for (u8 kc : keyCodeIncrement) {
			if (_system->functions.KeycodeRepeated(_system->data, &data, kc)) increment = true;
		}
		bool decrement = false;
		for (u8 kc : keyCodeDecrement) {
			if (_system->functions.KeycodeRepeated(_system->data, &data, kc)) decrement = true;
		}
		if (_system->controlDepth == depth) {
			if (selection >= 0 && selection < children.size && select) {
				_system->controlDepth = children[selection]->depth;
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
		} else if (_system->controlDepth == depth+1 && back) {
			_system->controlDepth = depth;
		}
		if (_system->controlDepth > depth) {
			highlighted = false;
		}
	} else {
		selection = -2;
	}
	if (_system->controlDepth == depth && selected) {
		bool mouseSelect = false;
		if (_system->inputMethod == InputMethod::MOUSE) {
			if (MouseOver()) {
				mouseSelect = true;
			}
			selection = -1;
		} else if (selection < 0 && _system->inputMethod != InputMethod::MOUSE /* TODO: This: && (sys->rawInput.AnyGP.state != 0 || sys->input.AnyKey.state != 0)*/) {
			selection = selectionDefault;
		}
		return mouseSelect;
	}
	return false;
}

void List::Draw() const {
	vec4 colorActual = highlighted ? colorHighlighted : color;
	if (colorActual.a > 0.0f) {
		_system->functions.DrawQuad(_system->data, const_cast<Any*>(&data), positionAbsolute * _system->scale, sizeAbsolute * _system->scale, colorActual);
	}
	if (selection >= 0 && colorSelection.a > 0.0f) {
		vec2 selectionPos = children[selection]->positionAbsolute;
		vec2 selectionSize = children[selection]->sizeAbsolute;
		_system->functions.DrawQuad(_system->data, const_cast<Any*>(&data), selectionPos * _system->scale, selectionSize * _system->scale, colorSelection);
	}
	PushScissor();
	Widget::Draw();
	PopScissor();
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
			sizeAbsolute.x = max(sizeAbsolute.x, max(childSize.x + child->position.x, 0.0f) + totalPadding.x);
		}
	}
	sizeForInheritance = sizeAbsolute - totalPadding;
	for (Widget* child : children) {
		if (child->size.y == 0.0f || !child->fractionHeight) {
			child->UpdateSize(sizeForInheritance, scale);
			sizeForInheritance.y -= child->GetSize().y;
		}
	}
	sizeContents = vec2(0.0f);
	for (Widget* child : children) {
		child->UpdateSize(sizeForInheritance, scale);
		vec2 childSize = child->GetSize();
		sizeContents.x = max(sizeContents.x, childSize.x);
		sizeContents.y += childSize.y;
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
	if (selected && selectable) {
		_system->selectedCenter = positionAbsolute + sizeAbsolute * 0.5f;
	}
	const bool mouseSelect = UpdateSelection(selected, {KC_GP_BTN_A, KC_KEY_ENTER}, {KC_GP_BTN_B, KC_KEY_ESC}, {KC_GP_AXIS_LS_DOWN, KC_KEY_DOWN}, {KC_GP_AXIS_LS_UP, KC_KEY_UP});
	pos += padding * scale;
	// Scrolling
	vec2 sizeAvailable = sizeAbsolute - padding * 2.0f * scale;
	vec2 scrollable = sizeContents - sizeAvailable;
	scrollable.x = max(0.0f, scrollable.x);
	scrollable.y = max(0.0f, scrollable.y);
	if (!scrollableX) scrollable.x = 0.0f;
	if (!scrollableY) scrollable.y = 0.0f;
	pos -= scrollable * scroll;
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
	{ // Scrolling
		vec2 mouse = vec2(_system->mouseCursor) / _system->scale;
		bool modifyScrollTarget = true;
		if (_system->inputMethod == InputMethod::MOUSE) {
			scrollTarget = (mouse - positionAbsolute) / sizeAbsolute;
		} else if (selection >= 0 && selection < children.size) {
			scrollTarget = (_system->selectedCenter - positionAbsolute) / sizeAbsolute;
		} else {
			modifyScrollTarget = false;
		}
		if (modifyScrollTarget) {
			scrollTarget = (scrollTarget - 0.5f) * 2.0f + 0.5f;
			scrollTarget.x = clamp01(scrollTarget.x);
			scrollTarget.y = clamp01(scrollTarget.y);
		}
		scroll = decay(scroll, scrollTarget, 0.1f, _system->timestep);
	}
}

ListH::ListH() {
	color = vec4(0.0f, 0.0f, 0.0f, 0.9f);
	colorHighlighted = vec4(0.1f, 0.1f, 0.1f, 0.9f);
	occludes = true;
	scrollableX = true;
	scrollableY = false;
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
			sizeAbsolute.y = max(sizeAbsolute.y, max(childSize.y + child->position.y, 0.0f) + totalPadding.y);
		}
		sizeForInheritance = sizeAbsolute - totalPadding;
	}
	for (Widget* child : children) {
		if (child->size.x == 0.0f || !child->fractionWidth) {
			child->UpdateSize(sizeForInheritance, scale);
			sizeForInheritance.x -= child->GetSize().x;
		}
	}
	sizeContents = vec2(0.0f);
	for (Widget* child : children) {
		child->UpdateSize(sizeForInheritance, scale);
		vec2 childSize = child->GetSize();
		sizeContents.x += childSize.x;
		sizeContents.y = max(sizeContents.y, childSize.y);
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
	if (selected && selectable) {
		_system->selectedCenter = positionAbsolute + sizeAbsolute * 0.5f;
	}
	const bool mouseSelect = UpdateSelection(selected, {KC_GP_BTN_A, KC_KEY_ENTER}, {KC_GP_BTN_B, KC_KEY_ESC}, {KC_GP_AXIS_LS_RIGHT, KC_KEY_RIGHT}, {KC_GP_AXIS_LS_LEFT, KC_KEY_LEFT});
	pos += padding * scale;
	// Scrolling
	vec2 sizeAvailable = sizeAbsolute - padding * 2.0f * scale;
	vec2 scrollable = sizeContents - sizeAvailable;
	scrollable.x = max(0.0f, scrollable.x);
	scrollable.y = max(0.0f, scrollable.y);
	if (!scrollableX) scrollable.x = 0.0f;
	if (!scrollableY) scrollable.y = 0.0f;
	pos -= scrollable * scroll;
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
	{ // Scrolling
		vec2 mouse = vec2(_system->mouseCursor) / _system->scale;
		vec2 scrollTarget = vec2(0.0f);
		if (_system->inputMethod == InputMethod::MOUSE) {
			scrollTarget = (mouse - positionAbsolute) / sizeAbsolute;
		} else if (selection >= 0 && selection < children.size) {
			scrollTarget = (_system->selectedCenter - positionAbsolute) / sizeAbsolute;
		}
		scrollTarget = (scrollTarget - 0.5f) * 2.0f + 0.5f;
		scrollTarget.x = clamp01(scrollTarget.x);
		scrollTarget.y = clamp01(scrollTarget.y);
		scroll = decay(scroll, scrollTarget, 0.1f, _system->timestep);
	}
}

Switch::Switch() : choice(0), open(false), changed(false), colorChoice(vec4(vec3(0.0f), 0.9f)) {
	selectable = true;
	selectionDefault = 0;
	color = vec4(vec3(0.2f), 0.9f);
	colorHighlighted = vec4(0.4f, 0.9f, 1.0f, 0.9f);
	colorSelection = vec4(0.4f, 0.9f, 1.0f, 0.9f);
	scrollableY = false;
}

void Switch::UpdateSize(vec2 container, f32 _scale) {
	scale = _scale;
	vec2 totalMargin = margin * 2.0f * scale;
	vec2 totalPadding = padding * 2.0f * scale;
	if (open) {
		ListV::UpdateSize(container, scale);
		openSizeAbsolute = sizeAbsolute;
		sizeAbsolute.y = children[choice]->GetSize().y + totalPadding.y;
	} else {
		sizeAbsolute = vec2(0.0f);
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
		// Spoof our size for mouse picking
		vec2 closedSizeAbsolute = sizeAbsolute;
		sizeAbsolute = openSizeAbsolute;
		ListV::Update(pos, true);
		// Set it back for layout
		sizeAbsolute = closedSizeAbsolute;
		if (_system->functions.KeycodeReleased(_system->data, &data, KC_MOUSE_LEFT) || _system->functions.KeycodeReleased(_system->data, &data, KC_GP_BTN_A) || _system->functions.KeycodeReleased(_system->data, &data, KC_KEY_ENTER)) {
			if (selection >= 0) {
				choice = selection;
				changed = true;
			}
			if (!MouseOver()) {
				highlighted = false;
			}
			open = false;
		}
		if (_system->functions.KeycodeReleased(_system->data, &data, KC_GP_BTN_B) || _system->functions.KeycodeReleased(_system->data, &data, KC_KEY_ESC)) {
			open = false;
		}
		if (!open) {
			_system->controlDepth = parentDepth;
		}
	} else {
		pos += (margin + position) * scale;
		if (selected && selectable) {
			_system->selectedCenter = positionAbsolute + sizeAbsolute * 0.5f;
		}
		highlighted = selected;
		positionAbsolute = pos;
		pos += padding * scale;
		if (_system->functions.KeycodeReleased(_system->data, &data, KC_MOUSE_LEFT) && MouseOver()) {
			open = true;
		}
		if (selected && (_system->functions.KeycodeReleased(_system->data, &data, KC_GP_BTN_A) || _system->functions.KeycodeReleased(_system->data, &data, KC_KEY_ENTER))) {
			open = true;
		}
		if (open) {
			_system->controlDepth = depth;
			selection = choice;
		}
		children[choice]->Update(pos, selected);
	}
}

void Switch::Draw() const {
	if (color.a > 0.0f) {
		vec2 fullSize = sizeAbsolute;
		if (open) {
			fullSize = max(fullSize, children.Back()->GetSize() + children.Back()->positionAbsolute - positionAbsolute - padding - children.Back()->margin);
		}
		_system->functions.DrawQuad(_system->data, const_cast<Any*>(&data), positionAbsolute * _system->scale, fullSize * _system->scale, (highlighted && !open) ? colorHighlighted : color);
	}
	if (open) {
		PushScissor(positionAbsolute, openSizeAbsolute);
		if (selection >= 0 && colorSelection.a > 0.0f) {
			Widget *child = children[selection];
			vec2 selectionPos = child->positionAbsolute - child->margin;
			vec2 selectionSize = child->sizeAbsolute + child->margin * 2.0f;
			_system->functions.DrawQuad(_system->data, const_cast<Any*>(&data), selectionPos * _system->scale, selectionSize * _system->scale, colorSelection);
		}
		if (choice != selection && colorChoice.a > 0.0f) {
			Widget *child = children[choice];
			vec2 choicePos = child->positionAbsolute - child->margin;
			vec2 choiceSize = child->sizeAbsolute + child->margin * 2.0f;
			_system->functions.DrawQuad(_system->data, const_cast<Any*>(&data), choicePos * _system->scale, choiceSize * _system->scale, colorChoice);
		}
		Widget::Draw();
	} else {
		PushScissor();
		children[choice]->Draw();
	}
	PopScissor();
}

void Switch::OnHide() {
	Widget::OnHide();
	open = false;
	_system->controlDepth = parentDepth;
}

Text::Text() : stringFormatted(), string(), padding(0.1f), fontSize(32.0f), bold(false), paddingEM(true), color(vec3(1.0f), 1.0f), colorOutline(vec3(0.0f), 1.0f), colorHighlighted(vec3(0.0f), 1.0f), colorOutlineHighlighted(vec3(1.0f), 1.0f), outline(false) {
	size.y = 0.0f;
}

void Text::PushScissor() const {
	const Scissor upScissor = _system->stackScissors.Back();
	Scissor scissor;
	scissor.topLeft = vec2i(
		max(upScissor.topLeft.x, i32((positionAbsolute.x - margin.x * scale) * _system->scale)),
		max(upScissor.topLeft.y, i32((positionAbsolute.y - margin.y * scale) * _system->scale))
	);
	scissor.botRight = vec2i(
		min(upScissor.botRight.x, (i32)ceil((positionAbsolute.x + margin.x * scale + sizeAbsolute.x) * _system->scale)),
		min(upScissor.botRight.y, (i32)ceil((positionAbsolute.y + margin.y * scale + sizeAbsolute.y) * _system->scale))
	);
	_system->functions.SetScissor(_system->data, const_cast<Any*>(&data), scissor.topLeft, scissor.botRight-scissor.topLeft);
	_system->stackScissors.Append(scissor);
}

void Text::UpdateSize(vec2 container, f32 _scale) {
	scale = _scale;
	vec2 totalMargin = margin * 2.0f * scale;
	vec2 totalPadding = padding * 2.0f * scale;
	if (size.x == 0.0f || size.y == 0.0f) {
		sizeAbsolute = _system->functions.GetTextDimensions(_system->data, &data, stringFormatted) * fontSize * scale;
	}
	if (size.x > 0.0f) {
		sizeAbsolute.x = fractionWidth ? (container.x * size.x - totalMargin.x) : size.x * scale;
	}
	if (size.y > 0.0f) {
		sizeAbsolute.y = fractionHeight ? (container.y * size.y - totalMargin.y) : size.y * scale;
	}
	LimitSize();
}

void Text::Update(vec2 pos, bool selected) {
	if (size.x != 0.0f) {
		stringFormatted = _system->functions.ApplyTextWrapping(_system->data, &data, string, sizeAbsolute.x / fontSize);
	} else {
		stringFormatted = string;
	}
	Widget::Update(pos, selected);
}

void Text::Draw() const {
	PushScissor();
	vec2 paddingAbsolute = padding;
	if (paddingEM) paddingAbsolute *= fontSize;
	vec2 drawPos = (positionAbsolute + paddingAbsolute) * _system->scale;
	vec2 textScale = vec2(fontSize) * _system->scale * scale;
	vec2 textArea = (sizeAbsolute - paddingAbsolute * 2.0f) * _system->scale;
	vec4 colorActual = highlighted ? colorHighlighted : color;
	vec4 colorOutlineActual = outline ? (highlighted ? colorOutlineHighlighted : colorOutline) : vec4(0.0);
	_system->functions.DrawText(_system->data, const_cast<Any*>(&data), drawPos, textArea, textScale, stringFormatted, colorActual, colorOutlineActual, bold);
	PopScissor();
}

Image::Image() : color(vec4(1.0f)) { occludes = true; }

void Image::Draw() const {
	_system->functions.DrawImage(_system->data, const_cast<Any*>(&data), positionAbsolute * _system->scale, sizeAbsolute * _system->scale, color);
}

Text* Button::AddDefaultText(WString string) {
	AzAssert(children.size == 0, "Buttons can only have 1 child");
	Text *buttonText = new Text(_system->defaults.buttonText);
	// buttonText->fontSize = 28.0f;
	// buttonText->color = vec4(vec3(1.0f), 1.0f);
	// buttonText->colorHighlighted = vec4(vec3(0.0f), 1.0f);
	// buttonText->SetHeightFraction(1.0f);
	// buttonText->padding = 0.0f;
	// buttonText->margin = 0.0f;
	buttonText->string = string;
	_system->AddWidget(this, buttonText);
	return buttonText;
}

Button::Button() : padding(0.0f), color(vec3(0.15f), 0.9f), colorHighlighted(0.4f, 0.9f, 1.0f, 0.9f), state(), keycodeActivators() {
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
	if (selected && selectable) {
		_system->selectedCenter = positionAbsolute + sizeAbsolute * 0.5f;
	}
	pos += padding * scale;
	bool wasHighlighted = highlighted;
	highlighted = selected;
	{
		bool wasMouseover = mouseover;
		mouseover = MouseOver();
		if (wasMouseover && !mouseover) {
			// Mouse leave should prevent clicking
			state.Set(false, false, false);
		}
	}
	if (children.size) {
		Widget *child = children[0];
		child->Update(pos + (1.0f - childScale) * sizeAbsolute * 0.5f, selected || mouseover || state.Down());
	}
	state.Tick(0.0f);
	if (mouseover) {
		if (_system->functions.KeycodePressed(_system->data, &data, KC_MOUSE_LEFT)) {
			state.Press();
		}
		if (_system->functions.KeycodeReleased(_system->data, &data, KC_MOUSE_LEFT) && state.Down()) {
			state.Release();
		}
	}
	if (_system->controlDepth == depth) {
		if (selected) {
			if (_system->functions.KeycodePressed(_system->data, &data, KC_GP_BTN_A) || _system->functions.KeycodePressed(_system->data, &data, KC_KEY_ENTER)) {
				state.Press();
			}
			if (_system->functions.KeycodeReleased(_system->data, &data, KC_GP_BTN_A) || _system->functions.KeycodeReleased(_system->data, &data, KC_KEY_ENTER)) {
				state.Release();
			}
		}
		for (u8 kc : keycodeActivators) {
			if (_system->functions.KeycodePressed(_system->data, &data, kc)) {
				state.Press();
			}
			if (_system->functions.KeycodeReleased(_system->data, &data, kc)) {
				state.Release();
			}
		}
	}
	if (state.Pressed()) {
		CALL_EVENT_FUNCTION(OnButtonPressed);
	}
	if (state.Repeated()) {
		CALL_EVENT_FUNCTION(OnButtonRepeated);
	}
	if (state.Released()) {
		CALL_EVENT_FUNCTION(OnButtonReleased);
	}
	highlighted = selected || mouseover || state.Down();
	if (highlighted && !wasHighlighted) {
		CALL_EVENT_FUNCTION(OnButtonHighlighted);
	}
}

void Button::Draw() const {
	PushScissor();
	vec2 pos = positionAbsolute * _system->scale;
	vec2 size = sizeAbsolute * _system->scale;
	if (state.Down()) {
		pos += size * 0.05f;
		size *= 0.9f;
	}
	_system->functions.DrawQuad(_system->data, const_cast<Any*>(&data), pos, size, highlighted ? colorHighlighted : color);
	if (children.size) {
		Widget *child = children[0];
		child->Draw();
	}
	PopScissor();
}

Checkbox::Checkbox() : colorBGOff(vec3(0.15f), 0.9f), colorBGHighlightOff(0.2f, 0.45f, 0.5f, 0.9f), colorBGOn(0.4f, 0.9f, 1.0f, 1.0f), colorBGHighlightOn(0.9f, 0.98f, 1.0f, 1.0f), colorKnobOff(vec3(0.0f), 1.0f), colorKnobOn(vec3(0.0f), 1.0f), colorKnobHighlightOff(vec3(0.0f), 1.0f), colorKnobHighlightOn(vec3(0.0f), 1.0f), transition(0.0f), checked(false) {
	selectable = true;
	size = vec2(48.0f, 24.0f);
	fractionWidth = false;
	fractionHeight = false;
	occludes = true;
}

void Checkbox::Update(vec2 pos, bool selected) {
	Widget::Update(pos, selected);
	const bool mouseover = MouseOver();
	if (_system->controlDepth != depth) {
		highlighted = false;
	}
	if (mouseover) {
		highlighted = true;
		if (_system->functions.KeycodeReleased(_system->data, &data, KC_MOUSE_LEFT)) {
			checked = !checked;
			if (checked) {
				CALL_EVENT_FUNCTION(OnCheckboxTurnedOn);
			} else {
				CALL_EVENT_FUNCTION(OnCheckboxTurnedOff);
			}
		}
	}
	if (_system->controlDepth == depth) {
		if (selected) {
			if (_system->functions.KeycodeReleased(_system->data, &data, KC_GP_BTN_A) || _system->functions.KeycodeReleased(_system->data, &data, KC_KEY_ENTER)) {
				checked = !checked;
				if (checked) {
					CALL_EVENT_FUNCTION(OnCheckboxTurnedOn);
				} else {
					CALL_EVENT_FUNCTION(OnCheckboxTurnedOff);
				}
			}
		}
	}
	if (checked) {
		transition = decay(transition, 1.0f, 0.05f, _system->timestep);
	} else {
		transition = decay(transition, 0.0f, 0.05f, _system->timestep);
	}
}

void Checkbox::Draw() const {
	const vec4 &colorBGOnActual = highlighted ? colorBGHighlightOn : colorBGOn;
	const vec4 &colorBGOffActual = highlighted ? colorBGHighlightOff : colorBGOff;
	vec4 colorBGActual = lerp(colorBGOffActual, colorBGOnActual, transition);
	_system->functions.DrawQuad(_system->data, const_cast<Any*>(&data), positionAbsolute * _system->scale, sizeAbsolute * _system->scale, colorBGActual);
	
	const vec4 &colorKnobOnActual = highlighted ? colorKnobHighlightOn : colorKnobOn;
	const vec4 &colorKnobOffActual = highlighted ? colorKnobHighlightOff : colorKnobOff;
	vec4 colorKnobActual = lerp(colorKnobOffActual, colorKnobOnActual, transition);
	f32 switchSize = min(sizeAbsolute.x, sizeAbsolute.y) * 0.9f;
	f32 switchMoveArea = max(sizeAbsolute.x, sizeAbsolute.y) - switchSize * (1.0f + 0.1f / 0.9f);
	vec2 switchPos = positionAbsolute + vec2(switchSize * 0.05f / 0.9f);
	if (sizeAbsolute.y < sizeAbsolute.x) {
		// Right means on
		switchPos.x += switchMoveArea * transition;
	} else {
		// Up means on
		switchPos.y += switchMoveArea * (1.0f - transition);
	}
	_system->functions.DrawQuad(_system->data, const_cast<Any*>(&data), switchPos * _system->scale, switchSize * _system->scale, colorKnobActual);
}

Textbox::Textbox() : string(), stringFormatted(), stringSuffix(), colorBG(vec3(0.15f), 0.9f), colorBGHighlighted(vec3(0.2f), 0.9f), colorBGError(0.1f, 0.0f, 0.0f, 0.9f), colorText(vec3(1.0f), 1.0f), colorTextHighlighted(vec3(1.0f), 1.0f), colorTextError(1.0f, 0.5f, 0.5f, 1.0f), padding(2.0f), cursor(0), fontIndex(1), fontSize(17.39f), cursorBlinkTimer(0.0f), textFilter(TextFilterBasic), textValidate(TextValidateAll), entry(false), multiline(false) {
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
	static WString negInfinity = ToWString("-Inf");
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

void Textbox::UpdateSize(vec2 container, f32 _scale) {
	scale = _scale;
	vec2 totalMargin = margin * 2.0f * scale;
	vec2 totalPadding = padding * 2.0f * scale;
	if (size.x == 0.0f || size.y == 0.0f) {
		sizeAbsolute = _system->functions.GetTextDimensions(_system->data, &data, stringFormatted) * fontSize * scale + totalPadding;
	}
	if (size.x > 0.0f) {
		sizeAbsolute.x = fractionWidth ? (container.x * size.x - totalMargin.x) : size.x * scale;
	}
	if (size.y > 0.0f) {
		sizeAbsolute.y = fractionHeight ? (container.y * size.y - totalMargin.y) : size.y * scale;
	}
	LimitSize();
}

static i32 CursorFromFormattedToSource(i32 cursor, const WString &source, const WString &formatted) {
	// TODO: Verify that this works
	i32 result = 0;
	for (i32 i = 0; i < cursor; i++, result++) {
		if (formatted[i] == '\n') {
			if (!isWhitespace(source[result])) {
				result--;
			}
		}
	}
	return result;
}

static i32 CursorFromSourceToFormatted(i32 cursor, const WString &source, const WString &formatted) {
	// TODO: Verify that this works
	i32 result = 0;
	for (i32 i = 0; i < cursor; i++, result++) {
		if (formatted[result] == '\n') {
			if (!isWhitespace(source[i])) {
				i--;
			}
		}
	}
	return result;
}

void Textbox::Update(vec2 pos, bool selected) {
	bool stoppedEntry = false;
	vec2 textArea = sizeAbsolute - padding * 2.0f * scale;
	vec2 textPos = positionAbsolute + padding * scale;
	if (entry) {
		cursorBlinkTimer += _system->timestep;
		if (cursorBlinkTimer > 1.0f) {
			cursorBlinkTimer -= 1.0f;
		}
		highlighted = true;
		WString typed = _system->functions.ConsumeTypingString(_system->data, &data);
		for (char32 c : typed) {
			if (textFilter(c)) {
				string.Insert(cursor, c);
				cursorBlinkTimer = 0.0f;
				cursor++;
			}
		}
		if (_system->functions.KeycodeRepeated(_system->data, &data, KC_KEY_BACKSPACE)) {
			if (cursor <= string.size && cursor > 0) {
				string.Erase(cursor-1);
				cursorBlinkTimer = 0.0f;
				cursor--;
			}
		}
		if (_system->functions.KeycodeRepeated(_system->data, &data, KC_KEY_DELETE)) {
			if (cursor < string.size) {
				string.Erase(cursor);
				cursorBlinkTimer = 0.0f;
			}
		}
		if (_system->functions.KeycodePressed(_system->data, &data, KC_KEY_HOME)) {
			if (_system->functions.KeycodeDown(_system->data, &data, KC_KEY_LEFTCTRL) || _system->functions.KeycodeDown(_system->data, &data, KC_KEY_RIGHTCTRL) || !multiline) {
				cursor = 0;
			} else {
				for (cursor--; cursor >= 0; cursor--) {
					if (string[cursor] == '\n') break;
				}
				cursor++;
			}
			cursorBlinkTimer = 0.0f;
		}
		if (_system->functions.KeycodePressed(_system->data, &data, KC_KEY_END)) {
			if (_system->functions.KeycodeDown(_system->data, &data, KC_KEY_LEFTCTRL) || _system->functions.KeycodeDown(_system->data, &data, KC_KEY_RIGHTCTRL) || !multiline) {
				cursor = string.size;
			} else {
				for (; cursor < string.size; cursor++) {
					if (string[cursor] == '\n') break;
				}
			}
			cursorBlinkTimer = 0.0f;
		}
		if (_system->functions.KeycodeRepeated(_system->data, &data, KC_KEY_TAB)) {
			string.Insert(cursor, '\t');
			cursor++;
			cursorBlinkTimer = 0.0f;
		}
		if (multiline) {
			if (_system->functions.KeycodeRepeated(_system->data, &data, KC_KEY_ENTER)) {
				string.Insert(cursor, '\n');
				cursor++;
				cursorBlinkTimer = 0.0f;
			}
		}
	}
	if (size.x != 0.0f && multiline) {
		stringFormatted = _system->functions.ApplyTextWrapping(_system->data, &data, string + stringSuffix, (sizeAbsolute.x - padding.x * 2.0f * scale) / fontSize);
	} else {
		stringFormatted = string + stringSuffix;
	}
	if (entry) {
		if (multiline) {
			bool up = _system->functions.KeycodeRepeated(_system->data, &data, KC_KEY_UP);
			bool down = _system->functions.KeycodeRepeated(_system->data, &data, KC_KEY_DOWN);
			if (up || down) {
				vec2 cursorPos = _system->functions.GetPositionFromCursorInText(_system->data, &data, textPos, textArea, fontSize * scale, SimpleRange<char32>(stringFormatted.data, stringFormatted.size - stringSuffix.size), CursorFromSourceToFormatted(cursor, string, stringFormatted), vec2(0.0f, 0.5f));
				if (up) {
					cursorPos -= _system->functions.GetLineHeight(_system->data, &data, fontSize * scale);
				}
				if (down) {
					cursorPos += _system->functions.GetLineHeight(_system->data, &data, fontSize * scale);
				}
				cursor = _system->functions.GetCursorFromPositionInText(_system->data, &data, textPos, textArea, fontSize * scale, SimpleRange<char32>(stringFormatted.data, stringFormatted.size - stringSuffix.size), cursorPos);
				cursor = CursorFromFormattedToSource(cursor, string, stringFormatted);
				cursorBlinkTimer = 0.0f;
			}
		}
		if (_system->functions.KeycodeRepeated(_system->data, &data, KC_KEY_LEFT)) {
			cursorBlinkTimer = 0.0f;
			if (_system->functions.KeycodeDown(_system->data, &data, KC_KEY_LEFTCTRL) || _system->functions.KeycodeDown(_system->data, &data, KC_KEY_RIGHTCTRL)) {
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
		if (_system->functions.KeycodeRepeated(_system->data, &data, KC_KEY_RIGHT)) {
			cursorBlinkTimer = 0.0f;
			if (_system->functions.KeycodeDown(_system->data, &data, KC_KEY_LEFTCTRL) || _system->functions.KeycodeDown(_system->data, &data, KC_KEY_RIGHTCTRL)) {
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
		if (!multiline && _system->functions.KeycodeReleased(_system->data, &data, KC_KEY_ENTER)) {
			entry = false;
			stoppedEntry = true;
			if (_system->controlDepth == depth+1) {
				_system->controlDepth = depth;
			}
		}
	}
	Widget::Update(pos, selected);
	bool mouseover = MouseOver();
	if (_system->controlDepth != depth) {
		highlighted = false;
	}
	if (mouseover) {
		highlighted = true;
	}
	if (_system->functions.KeycodePressed(_system->data, &data, KC_MOUSE_LEFT)) {
		if (mouseover) {
			if (_system->controlDepth == depth) {
				_system->controlDepth = depth+1;
			}
			const vec2 mouse = vec2(_system->mouseCursor);
			cursor = _system->functions.GetCursorFromPositionInText(_system->data, &data, textPos, textArea, fontSize * scale, SimpleRange<char32>(stringFormatted.data, stringFormatted.size - stringSuffix.size), mouse);
			cursor = CursorFromFormattedToSource(cursor, string, stringFormatted);
			cursorBlinkTimer = 0.0f;
		}
		if (!mouseover && entry && _system->controlDepth == depth+1) {
			_system->controlDepth = depth;
			entry = false;
		} else {
			entry = mouseover;
		}
	}
	if (_system->controlDepth == depth) {
		if (selected) {
			if ((_system->functions.KeycodeReleased(_system->data, &data, KC_GP_BTN_A) || _system->functions.KeycodeReleased(_system->data, &data, KC_KEY_ENTER)) && !stoppedEntry) {
				entry = true;
				_system->controlDepth++;
			} else {
				entry = false;
			}
		}
	} else if (_system->controlDepth == depth+1) {
		if (selected) {
			if (_system->functions.KeycodeReleased(_system->data, &data, KC_GP_BTN_B) || _system->functions.KeycodeReleased(_system->data, &data, KC_KEY_ESC)) {
				entry = false;
				_system->controlDepth--;
			}
		}
	}
}

void Textbox::Draw() const {
	vec4 colorBGActual, colorTextActual;
	if (!textValidate(string)) {
		colorBGActual = colorBGError;
		colorTextActual = colorTextError;
	} else {
		colorBGActual = highlighted ? colorBGHighlighted : colorBG;
		colorTextActual = highlighted ? colorTextHighlighted : colorText;
	}
	PushScissor();
	vec2 textPos = (positionAbsolute + padding * scale) * _system->scale;
	vec2 textScale = vec2(fontSize * _system->scale) * scale;
	vec2 textArea = (sizeAbsolute - padding * 2.0f * scale) * _system->scale;
	_system->functions.DrawQuad(_system->data, const_cast<Any*>(&data), positionAbsolute * _system->scale, sizeAbsolute * _system->scale, colorBGActual);
	_system->functions.DrawText(_system->data, const_cast<Any*>(&data), textPos, textArea, textScale, stringFormatted, colorTextActual, vec4(0.0f), false); 
	if (cursorBlinkTimer < 0.5f && entry) {
		vec2 cursorPos = _system->functions.GetPositionFromCursorInText(_system->data, const_cast<Any*>(&data), textPos / _system->scale, textArea / _system->scale, textScale / _system->scale, SimpleRange<char32>(stringFormatted.data, stringFormatted.size - stringSuffix.size), CursorFromSourceToFormatted(cursor, string, stringFormatted), vec2(0.0f, 0.0f));
		_system->functions.DrawQuad(_system->data, const_cast<Any*>(&data), cursorPos, vec2(ceil(_system->scale), textScale.y), colorTextActual);
	}
	PopScissor();
}

Slider::Slider() :
value(1.0f),
valueMin(0.0f),                        valueMax(1.0f),
valueStep(0.0f),
valueTick(-0.1f),                      valueTickShiftMult(0.1f),
minOverride(false),                    minOverrideValue(0.0f),
maxOverride(false),                    maxOverrideValue(1.0f),
mirror(nullptr),                       mirrorPrecision(1),
colorBG(vec3(0.15f), 0.9f),            colorSlider(0.4f, 0.9f, 1.0f, 1.0f),
colorBGHighlighted(vec3(0.2f), 0.9f),  colorSliderHighlighted(0.9f, 0.98f, 1.0f, 1.0f),
grabbed(false), left(), right()
{
	occludes = true;
	selectable = true;
}

void Slider::Update(vec2 pos, bool selected) {
	Widget::Update(pos, selected);
	mouseover = MouseOver();
	f32 knobSize = 16.0f * scale;
	left.Tick(_system->timestep);
	right.Tick(_system->timestep);
	if (selected && _system->controlDepth == depth) {
		bool held = _system->functions.KeycodeDown(_system->data, &data, KC_MOUSE_LEFT);
		bool leftHeld = held || _system->functions.KeycodeDown(_system->data, &data, KC_GP_AXIS_LS_LEFT) || _system->functions.KeycodeDown(_system->data, &data, KC_KEY_LEFT);
		bool rightHeld = held || _system->functions.KeycodeDown(_system->data, &data, KC_GP_AXIS_LS_RIGHT) || _system->functions.KeycodeDown(_system->data, &data, KC_KEY_RIGHT);
		if (_system->functions.KeycodePressed(_system->data, &data, KC_GP_AXIS_LS_LEFT) || _system->functions.KeycodePressed(_system->data, &data, KC_KEY_LEFT)) {
			left.Press();
		} else if (left.Down() && !leftHeld) {
			left.Release();
		}
		if (_system->functions.KeycodePressed(_system->data, &data, KC_GP_AXIS_LS_RIGHT) || _system->functions.KeycodePressed(_system->data, &data, KC_KEY_RIGHT)) {
			right.Press();
		} else if (right.Down() && !rightHeld) {
			right.Release();
		}
	}
	if (mouseover && !grabbed) {
		i32 mousePos = 0;
		f32 mouseX = (f32)_system->mouseCursor.x / _system->scale - positionAbsolute.x;
		f32 sliderX = map(value, valueMin, valueMax, 0.0f, sizeAbsolute.x - knobSize);
		if (mouseX < sliderX) {
			mousePos = -1;
		} else if (mouseX > sliderX+knobSize) {
			mousePos = 1;
		}
		if (_system->functions.KeycodePressed(_system->data, &data, KC_MOUSE_LEFT)) {
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
		f32 moved = f32(_system->mouseCursor.x - _system->mouseCursorPrev.x) / _system->scale * scale;
		if (_system->functions.KeycodeDown(_system->data, &data, KC_KEY_LEFTSHIFT)) {
			moved /= 10.0f;
		}
		if (moved != 0.0f) updated = true;
		value = clamp(value + moved, valueMin, valueMax);
	}
	scale = valueTick >= 0.0f ? valueTick : (valueMax - valueMin) * -valueTick;
	if (_system->functions.KeycodeDown(_system->data, &data, KC_KEY_LEFTSHIFT)) {
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
	if (_system->functions.KeycodeReleased(_system->data, &data, KC_MOUSE_LEFT)) {
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

void Slider::Draw() const {
	f32 knobSize = 16.0f * scale;
	vec4 colorBGActual = highlighted ? colorBGHighlighted : colorBG;
	vec4 colorSliderActual = highlighted ? colorSliderHighlighted : colorSlider;
	vec2 drawPos = positionAbsolute * _system->scale;
	_system->functions.DrawQuad(_system->data, const_cast<Any*>(&data), drawPos, sizeAbsolute * _system->scale, colorBGActual);
	drawPos.x += map(value, valueMin, valueMax, 2.0f * scale, sizeAbsolute.x - knobSize) * _system->scale;
	drawPos.y += 2.0f * _system->scale * scale;
	_system->functions.DrawQuad(_system->data, const_cast<Any*>(&data), drawPos, vec2(12.0f * scale, sizeAbsolute.y - 4.0f * scale) * _system->scale, colorSliderActual);
}

void Slider::SetValue(f32 newValue) {
	value = clamp(newValue, valueMin, valueMax);
}

f32 Slider::GetActualValue() {
	f32 actualValue;
	if (valueStep != 0.0f) {
		actualValue = valueMin + round((value - valueMin) / valueStep) * valueStep;
	} else {
		actualValue = value;
	}
	if (minOverride && abs(actualValue - valueMin) < 0.000001f) {
		actualValue = minOverrideValue;
	} else if (maxOverride && abs(actualValue - valueMax) < 0.000001f) {
		actualValue = maxOverrideValue;
	}
	return actualValue;
}

void Slider::UpdateMirror() {
	f32 actualValue = GetActualValue();
	mirror->string = ToWString(ToString(actualValue, 10, mirrorPrecision));
}

Hideable::Hideable(Widget *child) : hidden(false), hiddenPrev(false) {
	size = 0.0f;
	margin = 0.0f;
	children = {child};
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

void Hideable::Draw() const {
	if (!hidden) {
		children[0]->Draw();
	}
}

bool Hideable::Selectable() const {
	return selectable && !hidden;
}

} // namespace Az2D::Gui

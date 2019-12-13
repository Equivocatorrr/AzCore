/*
    File: Gamepad.cpp
    Author: Philip Haynes
*/

#include "Gamepad.hpp"
#include "../keycodes.hpp"

namespace AzCore {

namespace io {

Gamepad::Gamepad()
{
    for (u32 i = 0; i < IO_GAMEPAD_MAX_AXES * 2; i++)
    {
        axisPush[i].canRepeat = true;
    }
    for (u32 i = 0; i < IO_GAMEPAD_MAX_BUTTONS; i++)
    {
        button[i].canRepeat = true;
    }
    for (u32 i = 0; i < 4; i++)
    {
        hat[i].canRepeat = true;
    }
}

bool Gamepad::Pressed(u8 keyCode) const
{
    if (keyCode >= KC_GP_AXIS_LS_RIGHT && keyCode <= KC_GP_AXIS_H0_UP)
    {
        return axisPush[keyCode - KC_GP_AXIS_LS_RIGHT].Pressed();
    }
    else if (keyCode >= KC_GP_BTN_A && keyCode <= KC_GP_BTN_THUMBR)
    {
        return button[keyCode - KC_GP_BTN_A].Pressed();
    }
    else if (keyCode >= KC_GP_AXIS_H0_UP_RIGHT && keyCode <= KC_GP_AXIS_H0_UP_LEFT)
    {
        return hat[keyCode - KC_GP_AXIS_H0_UP_RIGHT].Pressed();
    }
    else
    {
        return false;
    }
}

bool Gamepad::Down(u8 keyCode) const
{
    if (keyCode >= KC_GP_AXIS_LS_RIGHT && keyCode <= KC_GP_AXIS_H0_UP)
    {
        return axisPush[keyCode - KC_GP_AXIS_LS_RIGHT].Down();
    }
    else if (keyCode >= KC_GP_BTN_A && keyCode <= KC_GP_BTN_THUMBR)
    {
        return button[keyCode - KC_GP_BTN_A].Down();
    }
    else if (keyCode >= KC_GP_AXIS_H0_UP_RIGHT && keyCode <= KC_GP_AXIS_H0_UP_LEFT)
    {
        return hat[keyCode - KC_GP_AXIS_H0_UP_RIGHT].Down();
    }
    else
    {
        return false;
    }
}

bool Gamepad::Released(u8 keyCode) const
{
    if (keyCode >= KC_GP_AXIS_LS_RIGHT && keyCode <= KC_GP_AXIS_H0_UP)
    {
        return axisPush[keyCode - KC_GP_AXIS_LS_RIGHT].Released();
    }
    else if (keyCode >= KC_GP_BTN_A && keyCode <= KC_GP_BTN_THUMBR)
    {
        return button[keyCode - KC_GP_BTN_A].Released();
    }
    else if (keyCode >= KC_GP_AXIS_H0_UP_RIGHT && keyCode <= KC_GP_AXIS_H0_UP_LEFT)
    {
        return hat[keyCode - KC_GP_AXIS_H0_UP_RIGHT].Released();
    }
    else
    {
        return false;
    }
}

} // namespace io

} // namespace AzCore

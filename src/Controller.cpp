// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Controller.h"

Controller::Controller()
{
    reset();
}

Controller::~Controller() = default;

void Controller::reset()
{
    buttons.fill(false);
    mode = ControllerMode::Joystick;
}

uint8_t Controller::read() const
{
    uint8_t value = 0xFF;

    if (mode == ControllerMode::Joystick)
    {
        // Joystick mode: directions + left fire.
        if (isButtonPressed(ControllerButton::Up))
            value &= static_cast<uint8_t>(~0x01);

        if (isButtonPressed(ControllerButton::Right))
            value &= static_cast<uint8_t>(~0x02);

        if (isButtonPressed(ControllerButton::Down))
            value &= static_cast<uint8_t>(~0x04);

        if (isButtonPressed(ControllerButton::Left))
            value &= static_cast<uint8_t>(~0x08);

        if (isButtonPressed(ControllerButton::FireLeft))
            value &= static_cast<uint8_t>(~0x40);

        return value;
    }

    // Keypad mode: keypad nibble + right fire.
    value = static_cast<uint8_t>(0xF0 | readKeypadNibble());

    if (isButtonPressed(ControllerButton::FireRight))
        value &= static_cast<uint8_t>(~0x40);

    return value;
}

uint8_t Controller::readKeypadNibble() const
{
    uint8_t data = 0x0F;

    if (isButtonPressed(ControllerButton::Key1))     data &= 0x0D;
    if (isButtonPressed(ControllerButton::Key2))     data &= 0x07;
    if (isButtonPressed(ControllerButton::Key3))     data &= 0x0C;

    if (isButtonPressed(ControllerButton::Key4))     data &= 0x02;
    if (isButtonPressed(ControllerButton::Key5))     data &= 0x03;
    if (isButtonPressed(ControllerButton::Key6))     data &= 0x0E;

    if (isButtonPressed(ControllerButton::Key7))     data &= 0x05;
    if (isButtonPressed(ControllerButton::Key8))     data &= 0x01;
    if (isButtonPressed(ControllerButton::Key9))     data &= 0x0B;

    if (isButtonPressed(ControllerButton::KeyStar))  data &= 0x09;
    if (isButtonPressed(ControllerButton::Key0))     data &= 0x0A;
    if (isButtonPressed(ControllerButton::KeyPound)) data &= 0x06;

    return data;
}

void Controller::setButton(ControllerButton button, bool pressed)
{
    buttons[static_cast<size_t>(button)] = pressed;
}

bool Controller::isButtonPressed(ControllerButton button) const
{
    return buttons[static_cast<size_t>(button)];
}

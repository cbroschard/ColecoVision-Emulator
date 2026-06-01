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

}

Controller::~Controller() = default;

uint8_t Controller::read() const
{
    return 0xFF;
}

void Controller::setButton(ControllerButton button, bool pressed)
{
    buttons[static_cast<size_t>(button)] = pressed;
}

bool Controller::isButtonPressed(ControllerButton button) const
{
    return buttons[static_cast<size_t>(button)];
}

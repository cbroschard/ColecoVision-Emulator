// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "InputManager.h"

InputManager::InputManager() :
    controller1(nullptr),
    controller2(nullptr)
{

}

InputManager::~InputManager() = default;

void InputManager::reset()
{
    if (controller1)
        controller1->reset();

    if (controller2)
        controller2->reset();
}

void InputManager::handleEvent(const SDL_Event& event)
{
    if (event.type != SDL_EVENT_KEY_DOWN &&
        event.type != SDL_EVENT_KEY_UP)
    {
        return;
    }

    const bool pressed = event.type == SDL_EVENT_KEY_DOWN;
    const SDL_Keycode key = event.key.key;

#ifdef DEBUG
    std::cout << "SDL key = " << key
              << (pressed ? " pressed" : " released")
              << std::endl;
#endif

    switch (key)
    {
        // Controller 1 joystick
        case SDLK_UP:
            if (controller1) controller1->setButton(ControllerButton::Up, pressed);
            break;

        case SDLK_DOWN:
            if (controller1) controller1->setButton(ControllerButton::Down, pressed);
            break;

        case SDLK_LEFT:
            if (controller1) controller1->setButton(ControllerButton::Left, pressed);
            break;

        case SDLK_RIGHT:
            if (controller1) controller1->setButton(ControllerButton::Right, pressed);
            break;

        case SDLK_RCTRL:
        case SDLK_SPACE:
            if (controller1) controller1->setButton(ControllerButton::FireLeft, pressed);
            break;

        case SDLK_RSHIFT:
            if (controller1) controller1->setButton(ControllerButton::FireRight, pressed);
            break;

        // Controller 1 keypad
        case SDLK_0:
            if (controller1) controller1->setButton(ControllerButton::Key0, pressed);
            break;

        case SDLK_1:
            if (controller1) controller1->setButton(ControllerButton::Key1, pressed);
            break;

        case SDLK_2:
            if (controller1) controller1->setButton(ControllerButton::Key2, pressed);
            break;

        case SDLK_3:
            if (controller1) controller1->setButton(ControllerButton::Key3, pressed);
            break;

        case SDLK_4:
            if (controller1) controller1->setButton(ControllerButton::Key4, pressed);
            break;

        case SDLK_5:
            if (controller1) controller1->setButton(ControllerButton::Key5, pressed);
            break;

        case SDLK_6:
            if (controller1) controller1->setButton(ControllerButton::Key6, pressed);
            break;

        case SDLK_7:
            if (controller1) controller1->setButton(ControllerButton::Key7, pressed);
            break;

        case SDLK_8:
            if (controller1) controller1->setButton(ControllerButton::Key8, pressed);
            break;

        case SDLK_9:
            if (controller1) controller1->setButton(ControllerButton::Key9, pressed);
            break;

        case SDLK_Z:
            if (controller1) controller1->setButton(ControllerButton::KeyStar, pressed);
            break;

        case SDLK_X:
            if (controller1) controller1->setButton(ControllerButton::KeyPound, pressed);
            break;

        // Controller 2 joystick
        case SDLK_W:
            if (controller2) controller2->setButton(ControllerButton::Up, pressed);
            break;

        case SDLK_S:
            if (controller2) controller2->setButton(ControllerButton::Down, pressed);
            break;

        case SDLK_A:
            if (controller2) controller2->setButton(ControllerButton::Left, pressed);
            break;

        case SDLK_D:
            if (controller2) controller2->setButton(ControllerButton::Right, pressed);
            break;

        case SDLK_LCTRL:
            if (controller2) controller2->setButton(ControllerButton::FireLeft, pressed);
            break;

        case SDLK_LSHIFT:
            if (controller2) controller2->setButton(ControllerButton::FireRight, pressed);
            break;

        // Controller 2 Keypad
        case SDLK_KP_0:
            if (controller2) controller2->setButton(ControllerButton::Key0, pressed);
            break;

        case SDLK_KP_1:
            if (controller2) controller2->setButton(ControllerButton::Key1, pressed);
            break;

        case SDLK_KP_2:
            if (controller2) controller2->setButton(ControllerButton::Key2, pressed);
            break;

        case SDLK_KP_3:
            if (controller2) controller2->setButton(ControllerButton::Key3, pressed);
            break;

        case SDLK_KP_4:
            if (controller2) controller2->setButton(ControllerButton::Key4, pressed);
            break;

        case SDLK_KP_5:
            if (controller2) controller2->setButton(ControllerButton::Key5, pressed);
            break;

        case SDLK_KP_6:
            if (controller2) controller2->setButton(ControllerButton::Key6, pressed);
            break;

        case SDLK_KP_7:
            if (controller2) controller2->setButton(ControllerButton::Key7, pressed);
            break;

        case SDLK_KP_8:
            if (controller2) controller2->setButton(ControllerButton::Key8, pressed);
            break;

        case SDLK_KP_9:
            if (controller2) controller2->setButton(ControllerButton::Key9, pressed);
            break;

        case SDLK_KP_MULTIPLY:
            if (controller2) controller2->setButton(ControllerButton::KeyStar, pressed);
            break;

        case SDLK_KP_MINUS:
            if (controller2) controller2->setButton(ControllerButton::KeyPound, pressed);
            break;

         default:
            break;
    }
}

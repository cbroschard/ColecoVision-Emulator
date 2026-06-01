// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <array>
#include <cstdint>
#include <cstddef>

enum class ControllerMode : uint8_t
{
    Keypad,
    Joystick
};

enum class ControllerButton : uint8_t
{
    Up = 0,
    Down,
    Left,
    Right,

    FireLeft,
    FireRight,

    Key0,
    Key1,
    Key2,
    Key3,
    Key4,
    Key5,
    Key6,
    Key7,
    Key8,
    Key9,
    KeyStar,
    KeyPound,

    Count
};

class Controller
{
    public:
        Controller();
        virtual ~Controller();

        void reset();

        inline ControllerMode getControllerMode() { return mode; }
        inline void setControllerMode(ControllerMode newMode) { mode = newMode; }

        uint8_t read() const;

        void setButton(ControllerButton button, bool pressed);
        bool isButtonPressed(ControllerButton button) const;

    protected:

    private:
        std::array<bool, static_cast<size_t>(ControllerButton::Count)> buttons{};

        ControllerMode mode = ControllerMode::Joystick;
};

#endif // CONTROLLER_H

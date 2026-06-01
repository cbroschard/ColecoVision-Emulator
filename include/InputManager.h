// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "Controller.h"
#include "SDL3\SDL.h"

class InputManager
{
    public:
        InputManager();
        virtual ~InputManager();

        inline void attachController1(Controller* controller) { controller1 = controller; }
        inline void attachController2(Controller* controller) { controller2 = controller; }

        void reset();

        void handleEvent(const SDL_Event& event);

    protected:

    private:
        Controller* controller1;
        Controller* controller2;

        void setController1Key(int key, bool pressed);
        void setController2Key(int key, bool pressed);

        void handleKey(SDL_Keycode key, bool pressed);
        void handleController1Key(SDL_Keycode key, bool pressed);
        void handleController2Key(SDL_Keycode key, bool pressed);
};

#endif // INPUTMANAGER_H

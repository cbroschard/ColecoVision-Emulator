// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Bus.h"

Bus::Bus() :
    controller1(nullptr),
    controller2(nullptr),
    mem(nullptr),
    psg(nullptr),
    vdp(nullptr)
{

}

Bus::~Bus() = default;

uint8_t Bus::readMemory(uint16_t address)
{
    if (!mem)
        return 0xFF;

    return mem->read(address);
}

void Bus::writeMemory(uint16_t address, uint8_t value)
{
    if (!mem)
        return;

    mem->write(address, value);
}

uint8_t Bus::readIO(uint8_t port)
{
    switch (port & 0xE0)
    {
        case 0xA0:
            // VDP read range: A0-BF
            if (port & 0x01)
                return vdp->readStatus();   // A1, A3, A5 ... BF
            else
                return vdp->readData();     // A0, A2, A4 ... BE

        case 0xE0:
            // Controller read range: E0-FF
            // E0/E1 = controller 1
            // E2/E3 = controller 2
            if (port & 0x02)
                return controller2->read();
            else
                return controller1->read();

        default:
            return 0xFF;
    }
}

void Bus::writeIO(uint8_t port, uint8_t value)
{
    switch (port & 0xE0)
    {
        case 0x80:
            // VDP write range: 80-9F
            if (port & 0x01)
                vdp->writeControl(value);   // 81, 83, 85 ... 9F
            else
                vdp->writeData(value);      // 80, 82, 84 ... 9E
            return;

        case 0xC0:
            // Controller keypad mode select
            controller1->setControllerMode(ControllerMode::Keypad);
            controller2->setControllerMode(ControllerMode::Keypad);
            return;

        case 0xE0:
            // PSG write + controller joystick mode select
            psg->write(value);

            controller1->setControllerMode(ControllerMode::Joystick);
            controller2->setControllerMode(ControllerMode::Joystick);
            return;

        default:
            return;
    }
}

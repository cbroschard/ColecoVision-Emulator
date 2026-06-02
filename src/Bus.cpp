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
    // VDP read ports: BE = data, BF = status
    if ((port & 0xFE) == 0xBE)
    {
        if (!vdp)
            return 0xFF;

        if (port & 0x01)
            return vdp->readStatus();
        else
            return vdp->readData();
    }

    // Controller read range: $E0-$FF.
    // Even mirrored ports are controller 1.
    // Ports with bit 1 set are controller 2.
    if ((port & 0xE0) == 0xE0)
    {
        if (port & 0x02)
            return controller2 ? controller2->read() : 0xFF;

        return controller1 ? controller1->read() : 0xFF;
    }

    return 0xFF;
}

void Bus::writeIO(uint8_t port, uint8_t value)
{
    // VDP write ports: BE = data, BF = control
    if ((port & 0xFE) == 0xBE)
    {
        if (vdp)
        {
            if (port & 0x01)
                vdp->writeControl(value);
            else
                vdp->writeData(value);
        }

        return;
    }

    switch (port & 0xE0)
    {
        case 0x80:
            // Controller keypad/right-fire mode: $80-$9F
            if (controller1)
                controller1->setControllerMode(ControllerMode::Keypad);

            if (controller2)
                controller2->setControllerMode(ControllerMode::Keypad);

            return;

        case 0xA0:
            // PSG write range: $A0-$BF, except BE/BF handled above.
            if (psg)
                psg->write(value);
            return;

        case 0xC0:
            // Controller joystick/left-fire mode: $C0-$DF
            if (controller1)
                controller1->setControllerMode(ControllerMode::Joystick);

            if (controller2)
                controller2->setControllerMode(ControllerMode::Joystick);

            return;

        case 0xE0:
            if (psg)
                psg->write(value);
            return;

        default:
            return;
    }
}

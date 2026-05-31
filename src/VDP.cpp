// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "VDP.h"

VDP::VDP() :
    address(0),
    readBuffer(0),
    statusReg(0),
    controlLatch(false),
    controlFirstByte(0)
{
    vram.fill(0x00);
    regs.fill(0x00);
}

VDP::~VDP() = default;

void VDP::reset()
{
    address             = 0;
    readBuffer          = 0;
    statusReg           = 0;
    controlLatch        = false;
    controlFirstByte    = 0;

    vram.fill(0x00);
    regs.fill(0x00);
}

uint8_t VDP::readStatus()
{
    uint8_t result = statusReg;

    // Clear VBlank flag / interrupt flag.
    statusReg &= ~0x80;

    // Reading status also resets the control latch.
    controlLatch = false;

    return result;
}

uint8_t VDP::readData()
{
    uint8_t result = readBuffer;

    readBuffer = vram[address & 0x3FFF];

    address = (address + 1) & 0x3FFF;

    // Data read resets the control latch.
    controlLatch = false;

    return result;
}

void VDP::writeControl(uint8_t value)
{
    if (!controlLatch)
    {
        controlFirstByte = value;
        controlLatch = true;
        return;
    }

    uint8_t first = controlFirstByte;
    uint8_t second = value;

    controlLatch = false;

    uint8_t command = second & 0xC0;

    switch (command)
    {
        case 0x00:
            // Set VRAM read address
            address = ((second & 0x3F) << 8) | first;
            address &= 0x3FFF;

            // TMS9918-style read prefetch
            readBuffer = vram[address];
            address = (address + 1) & 0x3FFF;
            break;

        case 0x40:
            // Set VRAM write address
            address = ((second & 0x3F) << 8) | first;
            address &= 0x3FFF;
            break;

        case 0x80:
            // Register write
            {
                uint8_t reg = second & 0x07;
                regs[reg] = first;
                updateModeFromRegisters();
            }
            break;

        case 0xC0:
            // Also treated like VRAM write address on many TMS9918 implementations
            address = ((second & 0x3F) << 8) | first;
            address &= 0x3FFF;
            break;
    }
}

void VDP::writeData(uint8_t value)
{
    vram[address & 0x3FFF] = value;

    address = (address + 1) & 0x3FFF;

    // A data write resets the control latch.
    controlLatch = false;
}

void VDP::updateModeFromRegisters()
{

}

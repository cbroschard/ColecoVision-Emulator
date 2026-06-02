// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "VDP.h"

VDP::VDP() :
    mode(VDPMode::GraphicsI),
    address(0),
    readBuffer(0),
    statusReg(0),
    controlLatch(false),
    controlFirstByte(0),
    cycleCounter(0),
    scanline(0),
    irqAsserted(false)
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
    cycleCounter        = 0;
    scanline            = 0;
    irqAsserted         = false;
    mode                = VDPMode::GraphicsI;

    vram.fill(0x00);
    regs.fill(0x00);
}

void VDP::tick(int cpuCycles)
{
    cycleCounter += cpuCycles;

    while (cycleCounter >= CPU_CYCLES_PER_SCANLINE)
    {
        cycleCounter -= CPU_CYCLES_PER_SCANLINE;
        scanline++;

        if (scanline == VBLANK_START_LINE)
        {
            statusReg |= 0x80; // VBlank flag

            if (regs[1] & 0x20)
            {
                irqAsserted = true;
            }
        }

        if (scanline >= SCANLINES_PER_FRAME)
        {
            scanline = 0;
        }
    }
}

uint8_t VDP::readStatus()
{
    uint8_t result = statusReg;

    statusReg &= ~0x80;
    irqAsserted = false;
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
                // TMS9918 has registers 0-7.
                const uint8_t reg = second & 0x07;
                regs[reg] = first;
                updateModeFromRegisters();
                break;
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

    readBuffer = value;

    address = (address + 1) & 0x3FFF;

    controlLatch = false;
}

void VDP::renderFrame(VideoOutput& output)
{
    if (!isDisplayEnabled())
    {
        clearToBackdrop(output);
        return;
    }

    switch (mode)
    {
        case VDPMode::GraphicsI:
            renderGraphicsI(output);
            break;

        case VDPMode::GraphicsII:
        case VDPMode::Text:
        case VDPMode::Multicolor:
        default:
            renderUnsupportedMode(output);
            break;
    }
}

void VDP::renderGraphicsI(VideoOutput& output)
{
    const uint8_t backdrop = getBackdropColor();

    const uint16_t nameTableBase =
        static_cast<uint16_t>((regs[2] & 0x0F) << 10);

    const uint16_t colorTableBase =
        static_cast<uint16_t>(regs[3] << 6);

    const uint16_t patternTableBase =
        static_cast<uint16_t>((regs[4] & 0x07) << 11);

    for (int tileY = 0; tileY < 24; ++tileY)
    {
        for (int tileX = 0; tileX < 32; ++tileX)
        {
            const uint16_t nameAddr =
                static_cast<uint16_t>(nameTableBase + tileY * 32 + tileX);

            const uint8_t patternIndex = vram[nameAddr & 0x3FFF];

            const uint8_t colorByte =
                vram[(colorTableBase + (patternIndex >> 3)) & 0x3FFF];

            uint8_t fg = static_cast<uint8_t>(colorByte >> 4);
            uint8_t bg = static_cast<uint8_t>(colorByte & 0x0F);

            if (fg == 0) fg = backdrop;
            if (bg == 0) bg = backdrop;

            for (int row = 0; row < 8; ++row)
            {
                const uint8_t patternByte =
                    vram[(patternTableBase + patternIndex * 8 + row) & 0x3FFF];

                for (int col = 0; col < 8; ++col)
                {
                    const bool pixelOn = (patternByte & (0x80 >> col)) != 0;

                    const int x = tileX * 8 + col;
                    const int y = tileY * 8 + row;

                    output.setPixel(x, y, pixelOn ? fg : bg);
                }
            }
        }
    }
}

void VDP::renderUnsupportedMode(VideoOutput& output)
{
    clearToBackdrop(output);
}

void VDP::updateModeFromRegisters()
{
    const bool m1 = (regs[1] & 0x10) != 0;
    const bool m2 = (regs[1] & 0x08) != 0;
    const bool m3 = (regs[0] & 0x02) != 0;

    if (!m1 && !m2 && !m3)
    {
        mode = VDPMode::GraphicsI;
    }
    else if (!m1 && !m2 && m3)
    {
        mode = VDPMode::GraphicsII;
    }
    else if (m1 && !m2 && !m3)
    {
        mode = VDPMode::Text;
    }
    else if (!m1 && m2 && !m3)
    {
        mode = VDPMode::Multicolor;
    }
    else
    {
        // Unsupported mixed modes for now.
        mode = VDPMode::GraphicsI;
    }
}

void VDP::clearToBackdrop(VideoOutput& output)
{
    const uint8_t backdrop = getBackdropColor();

    for (int y = 0; y < 192; ++y)
    {
        for (int x = 0; x < 256; ++x)
        {
            output.setPixel(x, y, backdrop);
        }
    }
}

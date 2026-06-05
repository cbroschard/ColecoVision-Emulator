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

    statusReg = 0x00;
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
            renderGraphicsII(output);
            break;
        case VDPMode::Text:
        case VDPMode::Multicolor:
        default:
            renderUnsupportedMode(output);
            break;
    }

    // Sprites render after background
    renderSprites(output);
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

void VDP::renderGraphicsII(VideoOutput& output)
{
    const uint8_t backdrop = getBackdropColor();

    const uint16_t nameTableBase =
        static_cast<uint16_t>((regs[2] & 0x0F) << 10);

    /*
        Graphics II uses register masks differently than Graphics I.

        Pattern table:
            R4 bits 0-2 select/enable pattern table pages.

        Color table:
            R3 bits select/enable color table pages.

        In the common ColecoVision setup, games usually configure these
        so each of the three screen thirds gets its own 256-pattern block.

        Screen is 32x24 tiles:
            tileY 0-7   = top third
            tileY 8-15  = middle third
            tileY 16-23 = bottom third
    */

    const uint16_t colorTableBase =
        static_cast<uint16_t>((regs[3] & 0x80) << 6);

    const uint16_t patternTableBase =
        static_cast<uint16_t>((regs[4] & 0x04) << 11);

    for (int tileY = 0; tileY < 24; ++tileY)
    {
        const int third = tileY / 8;

        for (int tileX = 0; tileX < 32; ++tileX)
        {
            const uint16_t nameAddr =
                static_cast<uint16_t>(nameTableBase + tileY * 32 + tileX);

            const uint8_t patternIndex =
                vram[nameAddr & 0x3FFF];

            /*
                Graphics II pattern/color layout:

                Each screen third has a separate 256-character section.

                    third 0: patterns 0x000-0x7FF
                    third 1: patterns 0x800-0xFFF
                    third 2: patterns 0x1000-0x17FF

                Same idea for color bytes.

                Each pattern row has its own color byte.
            */
            const uint16_t charBase =
                static_cast<uint16_t>(third * 0x0800 + patternIndex * 8);

            for (int row = 0; row < 8; ++row)
            {
                const uint16_t patternAddr =
                    static_cast<uint16_t>(patternTableBase + charBase + row);

                const uint16_t colorAddr =
                    static_cast<uint16_t>(colorTableBase + charBase + row);

                const uint8_t patternByte =
                    vram[patternAddr & 0x3FFF];

                const uint8_t colorByte =
                    vram[colorAddr & 0x3FFF];

                uint8_t fg = static_cast<uint8_t>(colorByte >> 4);
                uint8_t bg = static_cast<uint8_t>(colorByte & 0x0F);

                if (fg == 0) fg = backdrop;
                if (bg == 0) bg = backdrop;

                for (int col = 0; col < 8; ++col)
                {
                    const bool pixelOn =
                        (patternByte & (0x80 >> col)) != 0;

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

void VDP::renderSprites(VideoOutput& output)
{
    const uint16_t spriteAttributeBase =
        static_cast<uint16_t>((regs[5] & 0x7F) << 7);

    const uint16_t spritePatternBase =
        static_cast<uint16_t>((regs[6] & 0x07) << 11);

    const bool sprite16x16 = (regs[1] & 0x02) != 0;
    const bool magnified   = (regs[1] & 0x01) != 0;

    const int logicalSpriteSize = sprite16x16 ? 16 : 8;
    const int scale = magnified ? 2 : 1;
    const int visibleSpriteSize = logicalSpriteSize * scale;

    // First find the active sprite count using the Y=208 terminator.
    int spriteCount = 32;

    for (int sprite = 0; sprite < 32; ++sprite)
    {
        const uint16_t attrAddr =
            static_cast<uint16_t>(spriteAttributeBase + sprite * 4);

        const uint8_t yRaw =
            vram[(attrAddr + 0) & 0x3FFF];

        if (yRaw == 208)
        {
            spriteCount = sprite;
            break;
        }
    }

    // Render scanline by scanline so we can enforce the TMS9918
    // "first 4 sprites per scanline" rule.
    for (int screenY = 0; screenY < 192; ++screenY)
    {
        int visibleSprites[4] = { -1, -1, -1, -1 };
        int visibleCount = 0;

        // Hardware considers sprites in ascending sprite number order.
        for (int sprite = 0; sprite < spriteCount; ++sprite)
        {
            const uint16_t attrAddr =
                static_cast<uint16_t>(spriteAttributeBase + sprite * 4);

            const uint8_t yRaw =
                vram[(attrAddr + 0) & 0x3FFF];

            int spriteY = static_cast<int>(yRaw) + 1;

            // TMS9918 vertical wrap.
            if (spriteY >= 225)
                spriteY -= 256;

            if (screenY < spriteY || screenY >= spriteY + visibleSpriteSize)
                continue;

            visibleSprites[visibleCount++] = sprite;

            // Only first 4 sprites on this scanline are displayed.
            if (visibleCount >= 4)
                break;
        }

        // Draw selected sprites in reverse order so lower sprite numbers
        // end up on top.
        for (int listIndex = visibleCount - 1; listIndex >= 0; --listIndex)
        {
            const int sprite = visibleSprites[listIndex];

            if (sprite < 0)
                continue;

            const uint16_t attrAddr =
                static_cast<uint16_t>(spriteAttributeBase + sprite * 4);

            const uint8_t yRaw =
                vram[(attrAddr + 0) & 0x3FFF];

            const uint8_t xRaw =
                vram[(attrAddr + 1) & 0x3FFF];

            uint8_t patternIndex =
                vram[(attrAddr + 2) & 0x3FFF];

            const uint8_t colorByte =
                vram[(attrAddr + 3) & 0x3FFF];

            const uint8_t color =
                static_cast<uint8_t>(colorByte & 0x0F);

            if (color == 0)
                continue;

            if (sprite16x16)
                patternIndex &= 0xFC;

            int spriteX = static_cast<int>(xRaw);
            int spriteY = static_cast<int>(yRaw) + 1;

            if (colorByte & 0x80)
                spriteX -= 32;

            if (spriteY >= 225)
                spriteY -= 256;

            const int visibleRow = screenY - spriteY;
            const int sy = visibleRow / scale;

            if (sy < 0 || sy >= logicalSpriteSize)
                continue;

            for (int sx = 0; sx < logicalSpriteSize; ++sx)
            {
                int patternOffset = 0;

                if (sprite16x16)
                {
                    const bool rightHalf  = sx >= 8;
                    const bool bottomHalf = sy >= 8;

                    // TMS9918 16x16 sprite pattern layout:
                    // +0 = upper-left
                    // +1 = lower-left
                    // +2 = upper-right
                    // +3 = lower-right
                    if (bottomHalf)
                        patternOffset += 1;

                    if (rightHalf)
                        patternOffset += 2;
                }

                const int patternRow = sy & 0x07;
                const int patternCol = sx & 0x07;

                const uint16_t patternAddr =
                    static_cast<uint16_t>(
                        spritePatternBase +
                        static_cast<uint16_t>((patternIndex + patternOffset) * 8) +
                        patternRow
                    );

                const uint8_t patternByte =
                    vram[patternAddr & 0x3FFF];

                const bool pixelOn =
                    (patternByte & (0x80 >> patternCol)) != 0;

                if (!pixelOn)
                    continue;

                const int baseX = spriteX + sx * scale;

                for (int dx = 0; dx < scale; ++dx)
                {
                    const int x = baseX + dx;

                    if (x < 0 || x >= 256)
                        continue;

                    output.setPixel(x, screenY, color);
                }
            }
        }
    }
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

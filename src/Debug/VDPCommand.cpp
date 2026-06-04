// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Debug/MLMonitor.h"
#include "Debug/VDPCommand.h"

static const char* decodeVDPModeName(VDPMode mode)
{
    switch(mode)
    {
        case VDPMode::GraphicsI:
            return "Graphics I";
        case VDPMode::GraphicsII:
            return "Graphics II";
        case VDPMode::Text:
            return "Text";
        case VDPMode::Multicolor:
            return "Multicolor";
        default:
            return "Unsupported Mode";
    }
}

VDPCommand::VDPCommand() = default;

VDPCommand::~VDPCommand() = default;

int VDPCommand::order() const
{
    return 5;
}

std::string VDPCommand::name() const
{
    return "vdp";
}

std::string VDPCommand::category() const
{
    return "Chip/TMS9918A VDP";
}

std::string VDPCommand::shortHelp() const
{
    return "vdp       - VDP operations (use 'vdp help')";
}

std::string VDPCommand::help() const
{
    return
        "vdp <subcommand>:\n"
        "    mode                   Show current TMS9918A VDP graphics mode\n"
        "    regs                   Show VDP registers and decoded table addresses\n"
        "    sprites                Show VDP sprite attribute table\n"
        "    status                 Show VDP status snapshot without clearing VBlank/IRQ\n"
        "    tables                 Show resolved VDP table addresses for current mode\n"
        "    vram <addr> [len]      Dump VDP VRAM without changing VDP address/buffer\n";
}

void VDPCommand::execute(MLMonitor& mlMonitor, const std::vector<std::string>& args)
{
    if (args.size() < 2)
    {
        std::cout << help() << std::endl;
        return;
    }

    const std::string& sub = args[1];

    if (isHelp(sub))
    {
        std::cout << "Usage:\n" << help() << std::endl;
        return;
    }
    else if (sub == "mode")
    {
        VDPMode currentMode = mlMonitor.getMLMonitorBackend()->getVDPMode();

        std::cout << "Current VDP mode: "
                  << decodeVDPModeName(currentMode)
                  << std::endl;

        return;
    }
    else if (sub == "regs")
    {
        auto* backend = mlMonitor.getMLMonitorBackend();

        const uint8_t r0 = backend->getVDPRegister(0);
        const uint8_t r1 = backend->getVDPRegister(1);
        const uint8_t r2 = backend->getVDPRegister(2);
        const uint8_t r3 = backend->getVDPRegister(3);
        const uint8_t r4 = backend->getVDPRegister(4);
        const uint8_t r5 = backend->getVDPRegister(5);
        const uint8_t r6 = backend->getVDPRegister(6);
        const uint8_t r7 = backend->getVDPRegister(7);

        const uint16_t nameTable =
            static_cast<uint16_t>((r2 & 0x0F) << 10);

        const uint16_t colorTableGraphicsI =
            static_cast<uint16_t>(r3 << 6);

        const uint16_t colorTableGraphicsII =
            static_cast<uint16_t>((r3 & 0x80) << 6);

        const uint16_t patternTableGraphicsI =
            static_cast<uint16_t>((r4 & 0x07) << 11);

        const uint16_t patternTableGraphicsII =
            static_cast<uint16_t>((r4 & 0x04) << 11);

        const uint16_t spriteAttributeTable =
            static_cast<uint16_t>((r5 & 0x7F) << 7);

        const uint16_t spritePatternTable =
            static_cast<uint16_t>((r6 & 0x07) << 11);

        const bool m1 = (r1 & 0x10) != 0;
        const bool m2 = (r1 & 0x08) != 0;
        const bool m3 = (r0 & 0x02) != 0;

        const bool displayEnabled = (r1 & 0x40) != 0;
        const bool irqEnabled     = (r1 & 0x20) != 0;
        const bool sprite16x16    = (r1 & 0x02) != 0;
        const bool spriteMag      = (r1 & 0x01) != 0;

        std::cout << "VDP Registers:" << std::endl;

        std::cout << "R0: $" << hex2(r0)
                  << "  M3=" << (m3 ? "1" : "0")
                  << std::endl;

        std::cout << "R1: $" << hex2(r1)
                  << "  Display=" << (displayEnabled ? "on" : "off")
                  << " IRQ=" << (irqEnabled ? "on" : "off")
                  << " M1=" << (m1 ? "1" : "0")
                  << " M2=" << (m2 ? "1" : "0")
                  << " SpriteSize=" << (sprite16x16 ? "16x16" : "8x8")
                  << " Mag=" << (spriteMag ? "on" : "off")
                  << std::endl;

        std::cout << "R2: $" << hex2(r2)
                  << "  Name Table: $" << hex4(nameTable)
                  << std::endl;

        std::cout << "R3: $" << hex2(r3)
                  << "  Color Table G1: $" << hex4(colorTableGraphicsI)
                  << "  G2: $" << hex4(colorTableGraphicsII)
                  << std::endl;

        std::cout << "R4: $" << hex2(r4)
                  << "  Pattern Table G1: $" << hex4(patternTableGraphicsI)
                  << "  G2: $" << hex4(patternTableGraphicsII)
                  << std::endl;

        std::cout << "R5: $" << hex2(r5)
                  << "  Sprite Attribute Table: $" << hex4(spriteAttributeTable)
                  << std::endl;

        std::cout << "R6: $" << hex2(r6)
                  << "  Sprite Pattern Table: $" << hex4(spritePatternTable)
                  << std::endl;

        std::cout << "R7: $" << hex2(r7)
                  << "  Text FG=$" << hex2(static_cast<uint8_t>((r7 >> 4) & 0x0F))
                  << " Backdrop=$" << hex2(static_cast<uint8_t>(r7 & 0x0F))
                  << std::endl;

        return;
    }
    else if (sub == "sprites")
    {
        auto* backend = mlMonitor.getMLMonitorBackend();

        const uint8_t r1 = backend->getVDPRegister(1);
        const uint8_t r5 = backend->getVDPRegister(5);
        const uint8_t r6 = backend->getVDPRegister(6);

        const uint16_t spriteAttributeTable =
            static_cast<uint16_t>((r5 & 0x7F) << 7);

        const uint16_t spritePatternTable =
            static_cast<uint16_t>((r6 & 0x07) << 11);

        const bool sprite16x16 = (r1 & 0x02) != 0;
        const bool magnified   = (r1 & 0x01) != 0;

        std::cout << "VDP Sprites:" << std::endl;

        std::cout << "Sprite Attribute Table: $"
                  << hex4(spriteAttributeTable)
                  << std::endl;

        std::cout << "Sprite Pattern Table:   $"
                  << hex4(spritePatternTable)
                  << std::endl;

        std::cout << "Size:                   "
                  << (sprite16x16 ? "16x16" : "8x8")
                  << "  Magnified: "
                  << (magnified ? "on" : "off")
                  << std::endl;

        std::cout << std::endl;

        std::cout << "#   Addr   YRaw ScreenY XRaw ScreenX Pat PatUsed Color EC"
                  << std::endl;

        for (int sprite = 0; sprite < 32; ++sprite)
        {
            const uint16_t attrAddr =
                static_cast<uint16_t>(spriteAttributeTable + sprite * 4);

            const uint8_t yRaw =
                backend->peekVDPVRAM(static_cast<uint16_t>(attrAddr + 0));

            const uint8_t xRaw =
                backend->peekVDPVRAM(static_cast<uint16_t>(attrAddr + 1));

            const uint8_t patternRaw =
                backend->peekVDPVRAM(static_cast<uint16_t>(attrAddr + 2));

            const uint8_t colorByte =
                backend->peekVDPVRAM(static_cast<uint16_t>(attrAddr + 3));

            if (yRaw == 208)
            {
                std::cout << sprite
                          << "   $"
                          << hex4(attrAddr)
                          << "  $"
                          << hex2(yRaw)
                          << "  terminator"
                          << std::endl;

                break;
            }

            int screenY = static_cast<int>(yRaw) + 1;

            if (screenY >= 225)
                screenY -= 256;

            int screenX = static_cast<int>(xRaw);

            const bool earlyClock = (colorByte & 0x80) != 0;

            if (earlyClock)
                screenX -= 32;

            uint8_t patternUsed = patternRaw;

            if (sprite16x16)
                patternUsed &= 0xFC;

            const uint8_t color =
                static_cast<uint8_t>(colorByte & 0x0F);

            std::cout << sprite;

            if (sprite < 10)
                std::cout << "   ";
            else
                std::cout << "  ";

            std::cout << "$" << hex4(attrAddr)
                      << "  $" << hex2(yRaw);

            if (screenY < 0)
                std::cout << "  " << screenY << "     ";
            else if (screenY < 10)
                std::cout << "  " << screenY << "      ";
            else if (screenY < 100)
                std::cout << "  " << screenY << "     ";
            else
                std::cout << "  " << screenY << "    ";

            std::cout << "$" << hex2(xRaw);

            if (screenX < 0)
                std::cout << "  " << screenX << "     ";
            else if (screenX < 10)
                std::cout << "  " << screenX << "      ";
            else if (screenX < 100)
                std::cout << "  " << screenX << "     ";
            else
                std::cout << "  " << screenX << "    ";

            std::cout << "$" << hex2(patternRaw)
                      << " $" << hex2(patternUsed)
                      << "     $" << hex2(color)
                      << "    " << (earlyClock ? "yes" : "no")
                      << std::endl;
        }

        return;
    }
    else if (sub == "status")
    {
        auto* backend = mlMonitor.getMLMonitorBackend();
        const VDPStatusSnapshot s = backend->getVDPStatusSnapshot();

        std::cout << "VDP Status:" << std::endl;

        std::cout << "Status:   $" << hex2(s.statusReg)
                  << "  VBlank=" << ((s.statusReg & 0x80) ? "yes" : "no")
                  << "  5thSprite=" << ((s.statusReg & 0x40) ? "yes" : "no")
                  << "  Collision=" << ((s.statusReg & 0x20) ? "yes" : "no")
                  << std::endl;

        std::cout << "IRQ:      " << (s.irqAsserted ? "asserted" : "clear")
                  << std::endl;

        std::cout << "Scanline: " << s.scanline
                  << std::endl;

        std::cout << "Cycle:    " << s.cycleCounter
                  << std::endl;

        std::cout << "Address:  $" << hex4(s.address)
                  << std::endl;

        std::cout << "Buffer:   $" << hex2(s.readBuffer)
                  << std::endl;

        std::cout << "Latch:    " << (s.controlLatch ? "waiting for second byte" : "clear");

        if (s.controlLatch)
        {
            std::cout << "  FirstByte=$" << hex2(s.controlFirstByte);
        }

        std::cout << std::endl;

        return;
    }
    else if (sub == "tables")
    {
        auto* backend = mlMonitor.getMLMonitorBackend();

        const VDPMode mode = backend->getVDPMode();

        const uint8_t r1 = backend->getVDPRegister(1);
        const uint8_t r2 = backend->getVDPRegister(2);
        const uint8_t r3 = backend->getVDPRegister(3);
        const uint8_t r4 = backend->getVDPRegister(4);
        const uint8_t r5 = backend->getVDPRegister(5);
        const uint8_t r6 = backend->getVDPRegister(6);
        const uint8_t r7 = backend->getVDPRegister(7);

        const bool displayEnabled = (r1 & 0x40) != 0;

        const uint16_t nameTable =
            static_cast<uint16_t>((r2 & 0x0F) << 10);

        const uint16_t colorTableGraphicsI =
            static_cast<uint16_t>(r3 << 6);

        const uint16_t colorTableGraphicsII =
            static_cast<uint16_t>((r3 & 0x80) << 6);

        const uint16_t patternTableGraphicsI =
            static_cast<uint16_t>((r4 & 0x07) << 11);

        const uint16_t patternTableGraphicsII =
            static_cast<uint16_t>((r4 & 0x04) << 11);

        const uint16_t spriteAttributeTable =
            static_cast<uint16_t>((r5 & 0x7F) << 7);

        const uint16_t spritePatternTable =
            static_cast<uint16_t>((r6 & 0x07) << 11);

        std::cout << "VDP Tables:" << std::endl;

        std::cout << "Mode:              "
                  << decodeVDPModeName(mode)
                  << std::endl;

        std::cout << "Display:           "
                  << (displayEnabled ? "on" : "off")
                  << std::endl;

        std::cout << "Name Table:        $"
                  << hex4(nameTable)
                  << std::endl;

        if (mode == VDPMode::GraphicsII)
        {
            std::cout << "Color Table:       $"
                      << hex4(colorTableGraphicsII)
                      << "  Graphics II"
                      << std::endl;

            std::cout << "Pattern Table:     $"
                      << hex4(patternTableGraphicsII)
                      << "  Graphics II"
                      << std::endl;
        }
        else
        {
            std::cout << "Color Table:       $"
                      << hex4(colorTableGraphicsI)
                      << "  Graphics I/Text"
                      << std::endl;

            std::cout << "Pattern Table:     $"
                      << hex4(patternTableGraphicsI)
                      << "  Graphics I/Text"
                      << std::endl;
        }

        std::cout << "Sprite Attr Table: $"
                  << hex4(spriteAttributeTable)
                  << std::endl;

        std::cout << "Sprite Patt Table: $"
                  << hex4(spritePatternTable)
                  << std::endl;

        std::cout << "Backdrop Color:    $"
                  << hex2(static_cast<uint8_t>(r7 & 0x0F))
                  << std::endl;

        return;
    }
    else if (sub == "vram")
    {
        if (args.size() < 3)
        {
            std::cout << "Usage: vdp vram <addr> [len]" << std::endl;
            return;
        }

        auto* backend = mlMonitor.getMLMonitorBackend();

        uint16_t start = 0;
        uint16_t length = 128;

        try
        {
            start = parseAddress(args[2]);

            if (args.size() >= 4)
                length = parseAddress(args[3]);
        }
        catch (const std::exception& e)
        {
            std::cout << "Invalid vram arguments: " << e.what() << std::endl;
            std::cout << "Usage: vdp vram <addr> [len]" << std::endl;
            return;
        }

        if (length == 0)
            length = 1;

        if (length > 0x4000)
            length = 0x4000;

        std::cout << "VDP VRAM dump from $"
                  << hex4(start & 0x3FFF)
                  << " length $"
                  << hex4(length)
                  << std::endl;

        for (uint16_t offset = 0; offset < length; offset += 16)
        {
            const uint16_t rowAddr =
                static_cast<uint16_t>((start + offset) & 0x3FFF);

            std::cout << hex4(rowAddr) << ":";

            const uint16_t rowCount =
                static_cast<uint16_t>(((length - offset) < 16) ? (length - offset) : 16);

            for (uint16_t i = 0; i < rowCount; ++i)
            {
                const uint16_t addr =
                    static_cast<uint16_t>((start + offset + i) & 0x3FFF);

                const uint8_t value = backend->peekVDPVRAM(addr);

                std::cout << " " << hex2(value);
            }

            std::cout << std::endl;
        }

        return;
    }
    else
    {
        std::cout << "Unknown vdp subcommand: " << sub << std::endl;
        std::cout << help() << std::endl;
        return;
    }
}

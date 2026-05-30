// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Cartridge.h"

Cartridge::Cartridge() :
    rom(0x8000, 0x00),
    cartLoaded(false)
{

}

Cartridge::~Cartridge()
{

}

void Cartridge::reset()
{
    // no-op for now
}

bool Cartridge::loadROM(const std::string& ROM)
{
    std::ifstream file(ROM, std::ios::binary);
    if (!file)
    {
        cartLoaded = false;
        return false;
    }

    // Clear cartridge space first.
    rom.assign(0x8000, 0x00);

    file.read(reinterpret_cast<char*>(rom.data()), rom.size());

    const std::streamsize bytesRead = file.gcount();

    if (bytesRead <= 0)
    {
        cartLoaded = false;
        return false;
    }

    cartLoaded = true;
    return true;
}

uint8_t Cartridge::read(uint16_t offset) const
{
    if (!isCartridgeLoaded() || rom.empty())
        return 0xFF;

    if (offset >= rom.size())
        return 0xFF;

    return rom[offset];
}


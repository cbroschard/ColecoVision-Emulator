// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Memory.h"

Memory::Memory() :
    cart(nullptr)
{
    mem.fill(0x00);
}

Memory::~Memory()
{

}

void Memory::reset()
{
    // Reset RAM
    mem.fill(0x00);
}

uint8_t Memory::read(uint16_t address) const
{
    if (address >= BIOS_START && address <= BIOS_END)
        return bios[address];
    else if (address >= RAM_START && address <= RAM_END)
        return mem[address & 0x03FF];
    else if ((address >= CART_START && address <= CART_END) && cart)
        return cart ? cart->read(address - 0x8000) : 0xFF;
    else
        return 0xFF;
}

void Memory::write(uint16_t address, uint8_t value)
{
    if (address >= RAM_START && address <= RAM_END)
    {
        mem[address & 0x03FF] = value;
        return;
    }

    // BIOS and CART are read only
}

bool Memory::loadBIOS(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;

    file.read(reinterpret_cast<char*>(bios.data()), bios.size());

    return file.gcount() == static_cast<std::streamsize>(bios.size());
}

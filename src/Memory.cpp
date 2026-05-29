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

bool Memory::loadBIOS(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;

    file.read(reinterpret_cast<char*>(bios.data()), bios.size());

    return file.gcount() == static_cast<std::streamsize>(bios.size());
}

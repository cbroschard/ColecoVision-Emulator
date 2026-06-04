// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Cartridge.h"

Cartridge::Cartridge() :
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
    std::ifstream file(ROM, std::ios::binary | std::ios::ate);
    if (!file)
    {
        rom.clear();
        romFileName.clear();
        cartLoaded = false;
        return false;
    }

    const std::streamsize fileSize = file.tellg();

    if (fileSize <= 0)
    {
        rom.clear();
        romFileName.clear();
        cartLoaded = false;
        return false;
    }

    file.seekg(0, std::ios::beg);

    rom.resize(static_cast<size_t>(fileSize));

    if (!file.read(reinterpret_cast<char*>(rom.data()), fileSize))
    {
        rom.clear();
        romFileName.clear();
        cartLoaded = false;
        return false;
    }

    romFileName = ROM;
    cartLoaded = true;
    return true;
}

uint8_t Cartridge::read(uint16_t offset) const
{
    if (!isCartridgeLoaded() || rom.empty())
        return 0xFF;

    const size_t wrappedOffset = static_cast<size_t>(offset) % rom.size();

    return rom[wrappedOffset];
}

size_t Cartridge::getROMSize() const
{
    return rom.size();
}

uint8_t Cartridge::peekROM(size_t offset) const
{
    if (!isCartridgeLoaded() || offset >= rom.size())
        return 0xFF;

    return rom[offset];
}

uint32_t Cartridge::calculateCRC32() const
{
    uint32_t crc = 0xFFFFFFFF;

    for (uint8_t byte : rom)
    {
        crc ^= byte;

        for (int bit = 0; bit < 8; ++bit)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }

    return crc ^ 0xFFFFFFFF;
}

CartridgeInfo Cartridge::getInfo() const
{
    CartridgeInfo info;

    info.loaded = cartLoaded;

    if (!cartLoaded || rom.empty())
    {
        info.mappingType = "None";
        info.mirrorDescription = "None";
        return info;
    }

    info.fileName = romFileName;

    info.romSizeBytes = rom.size();
    info.romSizeKB = rom.size() / 1024;

    info.signature0 = rom.size() > 0 ? rom[0] : 0x00;
    info.signature1 = rom.size() > 1 ? rom[1] : 0x00;

    info.validColecoSignature =
        (info.signature0 == 0x55 && info.signature1 == 0xAA) ||
        (info.signature0 == 0xAA && info.signature1 == 0x55);

    /*
        ColecoVision cartridge header layout commonly places the game start
        address at ROM offsets $000A/$000B. Since cartridge ROM is mapped at
        CPU $8000, this value should usually be a CPU address like $805B.
    */
    if (rom.size() > 0x0B)
    {
        info.entryPoint =
            static_cast<uint16_t>(rom[0x0A]) |
            static_cast<uint16_t>(rom[0x0B] << 8);

        info.hasEntryPoint = true;
    }

    info.crc32 = calculateCRC32();

    info.mappingType = "Standard ColecoVision ROM";
    info.mirrorDescription = "$8000-$FFFF, mirrored/wrapped by ROM size";

    return info;
}

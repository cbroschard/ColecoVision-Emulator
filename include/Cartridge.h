// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

struct CartridgeInfo
{
    bool loaded = false;

    std::string fileName;

    size_t romSizeBytes = 0;
    size_t romSizeKB = 0;

    uint8_t signature0 = 0x00;
    uint8_t signature1 = 0x00;
    bool validColecoSignature = false;

    uint16_t entryPoint = 0x0000;
    bool hasEntryPoint = false;

    uint32_t crc32 = 0x00000000;

    std::string mappingType;
    std::string mirrorDescription;
};

class Cartridge
{
    public:
        Cartridge();
        virtual ~Cartridge();

        void reset();

        bool loadROM(const std::string& ROM);

        uint8_t read(uint16_t offset) const;

        inline bool isCartridgeLoaded() const { return cartLoaded; }

        CartridgeInfo getInfo() const;

        size_t getROMSize() const;
        uint8_t peekROM(size_t offset) const;

    private:
        std::vector<uint8_t> rom;
        bool cartLoaded;

        std::string romFileName;

        uint32_t calculateCRC32() const;
};

#endif

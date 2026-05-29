// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef MEMORY_H
#define MEMORY_H

#include <array>
#include <cstdint>
#include <fstream>
#include "Cartridge.h"

class Memory
{
    public:
        Memory();
        virtual ~Memory();

    inline void attachCartridgeInstance(Cartridge* cart) { this->cart = cart; }

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);

    void reset();

    bool loadBIOS(const std::string& path);

    protected:

    private:
        // Non-owning pointers
        Cartridge* cart;

        std::array<uint8_t, 0x2000> bios{}; // 8K BIOS ROM
        std::array<uint8_t, 0x0400> mem{}; // 1K RAM
};

#endif // MEMORY_H

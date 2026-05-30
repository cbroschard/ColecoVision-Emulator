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

    uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t value);

    void reset();

    bool loadBIOS(const std::string& path);

    protected:

    private:
        // Non-owning pointers
        Cartridge* cart;

        // Memory constants
        static constexpr uint16_t BIOS_START = 0x0000;
        static constexpr uint16_t BIOS_END   = 0x1FFF;
        static constexpr uint16_t RAM_START  = 0x6000;
        static constexpr uint16_t RAM_END    = 0x7FFF;
        static constexpr uint16_t CART_START = 0x8000;
        static constexpr uint16_t CART_END   = 0xFFFF;

        std::array<uint8_t, 0x2000> bios{}; // 8K BIOS ROM
        std::array<uint8_t, 0x0400> mem{}; // 1K RAM
};

#endif // MEMORY_H

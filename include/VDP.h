// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef VDP_H
#define VDP_H

#include <array>
#include <cstdint>
#include "VideoOutput.h"

class VDP
{
    public:
        VDP();
        virtual ~VDP();

        void reset();

        void tick(int cpuCycles);

        uint8_t readStatus();
        uint8_t readData();

        void writeControl(uint8_t value);
        void writeData(uint8_t value);

        void renderFrame(VideoOutput& output);

    protected:

    private:
        std::array<uint8_t, 0x4000> vram{};     // 16K VRAM
        std::array<uint8_t, 8> regs{};          // VDP registers 0-7

        static constexpr int CPU_CYCLES_PER_SCANLINE = 228;
        static constexpr int SCANLINES_PER_FRAME = 262;
        static constexpr int VBLANK_START_LINE = 192;

        uint16_t address;                       // Current VRAM address
        uint8_t readBuffer;                     // Delayed VRAM read buffer
        uint8_t statusReg;                       // VDP status register

        bool controlLatch;                      // Waiting for second control byte?
        uint8_t controlFirstByte;               // First byte written to control port

        int cycleCounter;
        int scanline;

        void updateModeFromRegisters();
};

#endif // VDP_H

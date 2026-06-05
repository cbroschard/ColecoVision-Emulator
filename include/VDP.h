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

struct VDPStatusSnapshot
{
    uint8_t statusReg;
    uint16_t address;
    uint8_t readBuffer;

    bool controlLatch;
    uint8_t controlFirstByte;

    int cycleCounter;
    int scanline;

    bool irqAsserted;
};

enum class VDPMode
{
    GraphicsI,
    GraphicsII,
    Text,
    Multicolor
};

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

        inline bool isIRQAsserted() const { return irqAsserted; }

        // ML Monitor
        inline VDPMode getMode() const { return mode; }
        inline uint8_t getRegister(uint8_t index) const { return regs[index & 0x07]; }
        inline VDPStatusSnapshot getStatusSnapshot() const { return {statusReg, address, readBuffer, controlLatch, controlFirstByte,
                                                                     cycleCounter, scanline, irqAsserted }; }
        inline uint8_t peekVRAM(uint16_t address) const { return vram[address & 0x3FFF]; }

    protected:

    private:
        std::array<uint8_t, 0x4000> vram{};     // 16K VRAM
        std::array<uint8_t, 8> regs{};          // VDP registers 0-7

        VDPMode mode;

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

        bool irqAsserted;

        // Graphics modes rendering
        void renderGraphicsI(VideoOutput& output);
        void renderGraphicsII(VideoOutput& output);
        void renderUnsupportedMode(VideoOutput& output);

        // Sprite rendering
        void renderSprites(VideoOutput& output);

        // Helpers
        inline uint8_t getBackdropColor() const { return regs[7] & 0x0F; }
        inline bool isDisplayEnabled() const { return (regs[1] & 0x40) != 0; }

        void updateModeFromRegisters();
        void updateIRQState();

        void clearToBackdrop(VideoOutput& output);
};

#endif // VDP_H

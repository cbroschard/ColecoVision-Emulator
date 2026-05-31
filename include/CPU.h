// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef CPU_H
#define CPU_H

#include <array>
#include <cstdint>
#include <functional>
#include "Bus.h"

class CPU
{
    public:
        CPU();
        virtual ~CPU();

        inline void attachBusInstance(Bus* bus) { this->bus = bus; }

        void reset();

        int step();

    protected:

    private:
        // Non-owning pointers
        Bus* bus;

        using OpcodeHandler = std::function<int()>;
        std::array<OpcodeHandler, 256> opcodeTable;

        inline static constexpr std::array<uint8_t, 256> CYCLE_COUNTS =
        {{
            // 0x00 - 0x0F
             4, 10,  7,  6,  4,  4,  7,  4,  4, 11,  7,  6,  4,  4,  7,  4,
            // 0x10 - 0x1F
             8, 10,  7,  6,  4,  4,  7,  4, 12, 11,  7,  6,  4,  4,  7,  4,
            // 0x20 - 0x2F
             7, 10, 16,  6,  4,  4,  7,  4,  7, 11, 16,  6,  4,  4,  7,  4,
            // 0x30 - 0x3F
             7, 10, 13,  6, 11, 11, 10,  4,  7, 11, 13,  6,  4,  4,  7,  4,
            // 0x40 - 0x4F
             4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
            // 0x50 - 0x5F
             4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
            // 0x60 - 0x6F
             4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
            // 0x70 - 0x7F
             7,  7,  7,  7,  7,  7,  4,  7,  4,  4,  4,  4,  4,  4,  7,  4,
            // 0x80 - 0x8F
             4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
            // 0x90 - 0x9F
             4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
            // 0xA0 - 0xAF
             4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
            // 0xB0 - 0xBF
             4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
            // 0xC0 - 0xCF
             5, 10, 10, 10, 10, 11,  7, 11,  5, 10, 10,  0, 10, 17,  7, 11,
            // 0xD0 - 0xDF
             5, 10, 10, 11, 10, 11,  7, 11,  5,  4, 10, 11, 10,  0,  7, 11,
            // 0xE0 - 0xEF
             5, 10, 10, 19, 10, 11,  7, 11,  5,  4, 10,  4, 10,  0,  7, 11,
            // 0xF0 - 0xFF
             5, 10, 10,  4, 10, 11,  7, 11,  5,  6, 10,  4, 10,  0,  7, 11
        }};

        inline static constexpr std::array<uint8_t, 256> CB_CYCLE_COUNTS =
        {{
            // 0x00 - 0x0F
             8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
            // 0x10 - 0x1F
             8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
            // 0x20 - 0x2F
             8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
            // 0x30 - 0x3F
             8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
            // 0x40 - 0x4F
             8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
            // 0x50 - 0x5F
             8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
            // 0x60 - 0x6F
             8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
            // 0x70 - 0x7F
             8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
            // 0x80 - 0x8F
             8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
            // 0x90 - 0x9F
             // 0x90 - 0x9F
             8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
            // 0xA0 - 0xAF
             8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
            // 0xB0 - 0xBF
             8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
            // 0xC0 - 0xCF
             8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
            // 0xD0 - 0xDF
             8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
            // 0xE0 - 0xEF
             8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8,
            // 0xF0 - 0xFF
             8,  8,  8,  8,  8,  8, 15,  8,  8,  8,  8,  8,  8,  8, 15,  8
        }};

        inline static constexpr std::array<uint8_t, 256> ED_CYCLE_COUNTS =
        {{
            // 0x00 - 0x0F
             8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
            // 0x10 - 0x1F
             8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
            // 0x20 - 0x2F
             8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
            // 0x30 - 0x3F
             8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
            // 0x40 - 0x4F
            12, 12, 15, 20,  8, 14,  8,  9, 12, 12, 15, 20,  8, 14,  8,  9,
            // 0x50 - 0x5F
            12, 12, 15, 20,  8, 14,  8,  9, 12, 12, 15, 20,  8, 14,  8,  9,
            // 0x60 - 0x6F
            12, 12, 15, 20,  8, 14,  8, 18, 12, 12, 15, 20,  8, 14,  8, 18,
            // 0x70 - 0x7F
            12, 12, 15, 20,  8, 14,  8,  8, 12, 12, 15, 20,  8, 14,  8,  8,
            // 0x80 - 0x8F
             8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
            // 0x90 - 0x9F
             8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
            // 0xA0 - 0xAF
            16, 16, 16, 16,  8,  8,  8,  8, 16, 16, 16, 16,  8,  8,  8,  8,
            // 0xB0 - 0xBF
            16, 16, 16, 16,  8,  8,  8,  8, 16, 16, 16, 16,  8,  8,  8,  8,
            // 0xC0 - 0xFF invalid ED opcodes
             8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
             8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
             8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
             8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8
        }};

        inline static constexpr std::array<uint8_t, 256> DD_CYCLE_COUNTS =
        {{
            // 0x00 - 0x0F
             8, 14, 11, 10,  8,  8, 11,  8,  8, 15, 11, 10,  8,  8, 11,  8,
            // 0x10 - 0x1F
            12, 14, 11, 10,  8,  8, 11,  8, 16, 15, 11, 10,  8,  8, 11,  8,
            // 0x20 - 0x2F
            11, 14, 20, 10,  8,  8, 11,  8, 11, 15, 20, 10,  8,  8, 11,  8,
            // 0x30 - 0x3F
            11, 14, 17, 10, 23, 23, 19,  8, 11, 15, 17, 10,  8,  8, 11,  8,

            // 0x40 - 0x4F
             8,  8,  8,  8,  8,  8, 19,  8,  8,  8,  8,  8,  8,  8, 19,  8,
            // 0x50 - 0x5F
             8,  8,  8,  8,  8,  8, 19,  8,  8,  8,  8,  8,  8,  8, 19,  8,
            // 0x60 - 0x6F
             8,  8,  8,  8,  8,  8, 19,  8,  8,  8,  8,  8,  8,  8, 19,  8,
            // 0x70 - 0x7F
            19, 19, 19, 19, 19, 19,  8, 19,  8,  8,  8,  8,  8,  8, 19,  8,

            // 0x80 - 0x8F
             8,  8,  8,  8,  8,  8, 19,  8,  8,  8,  8,  8,  8,  8, 19,  8,
            // 0x90 - 0x9F
             8,  8,  8,  8,  8,  8, 19,  8,  8,  8,  8,  8,  8,  8, 19,  8,
            // 0xA0 - 0xAF
             8,  8,  8,  8,  8,  8, 19,  8,  8,  8,  8,  8,  8,  8, 19,  8,
            // 0xB0 - 0xBF
             8,  8,  8,  8,  8,  8, 19,  8,  8,  8,  8,  8,  8,  8, 19,  8,

            // 0xC0 - 0xCF
             9, 14, 14, 14, 14, 15, 11, 15,  9, 14, 14,  0, 14, 21, 11, 15,
            // 0xD0 - 0xDF
             9, 14, 14, 15, 14, 15, 11, 15,  9,  8, 14, 15, 14,  4, 11, 15,
            // 0xE0 - 0xEF
             9, 14, 14, 23, 14, 15, 11, 15,  9,  8, 14,  8, 14,  4, 11, 15,
            // 0xF0 - 0xFF
             9, 14, 14,  8, 14, 15, 11, 15,  9, 10, 14,  8, 14,  4, 11, 15
        }};

        inline static constexpr std::array<uint8_t, 256> FD_CYCLE_COUNTS = DD_CYCLE_COUNTS;

        inline static constexpr std::array<uint8_t, 256> DDCB_CYCLE_COUNTS =
        {{
            // 0x00 - 0x0F
            23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
            // 0x10 - 0x1F
            23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
            // 0x20 - 0x2F
            23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
            // 0x30 - 0x3F
            23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,

            // 0x40 - 0x4F
            20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
            // 0x50 - 0x5F
            20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
            // 0x60 - 0x6F
            20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
            // 0x70 - 0x7F
            20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,

            // 0x80 - 0x8F
            23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
            // 0x90 - 0x9F
            23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
            // 0xA0 - 0xAF
            23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
            // 0xB0 - 0xBF
            23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,

            // 0xC0 - 0xCF
            23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
            // 0xD0 - 0xDF
            23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
            // 0xE0 - 0xEF
            23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
            // 0xF0 - 0xFF
            23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23
        }};

        inline static constexpr std::array<uint8_t, 256> FDCB_CYCLE_COUNTS = DDCB_CYCLE_COUNTS;

        enum Flags : uint8_t
        {
            FLAG_S  = 0x80, // Sign
            FLAG_Z  = 0x40, // Zero
            FLAG_Y  = 0x20, // Undocumented copy bit 5
            FLAG_H  = 0x10, // Half carry
            FLAG_X  = 0x08, // Undocumented copy bit 3
            FLAG_PV = 0x04, // Parity / overflow
            FLAG_N  = 0x02, // Add/Subtract
            FLAG_C  = 0x01  // Carry
        };

        // Main registers
        uint8_t A;
        uint8_t F;

        uint8_t B;
        uint8_t C;
        uint8_t D;
        uint8_t E;
        uint8_t H;
        uint8_t L;

        // Shadow registers
        uint8_t A_;
        uint8_t F_;

        uint8_t B_;
        uint8_t C_;
        uint8_t D_;
        uint8_t E_;
        uint8_t H_;
        uint8_t L_;

        // Index registers
        uint16_t IX;
        uint16_t IY;

        // Program counter and stack pointer
        uint16_t PC;
        uint16_t SP;

        // Interrupt / refresh registers
        uint8_t I;
        uint8_t R;

        // Interrupt state
        bool IFF1;
        bool IFF2;
        uint8_t IM;

        bool halted;
        bool irqPending;
        bool nmiPending;
        bool eiDelay;

        // Timing
        uint64_t cycles;

        uint8_t fetch8();
        uint16_t fetch16();

        uint8_t read8(uint16_t address) const;
        void write8(uint16_t address, uint8_t value);

        void writeIO(uint8_t port, uint8_t value);
        uint8_t readIO(uint8_t port);

        // Stack helpers
        void push16(uint16_t value);
        uint16_t pop16();

        inline uint16_t getBC() const { return static_cast<uint16_t>((B << 8) | C); }
        inline uint16_t getDE() const { return static_cast<uint16_t>((D << 8) | E); }
        inline uint16_t getHL() const { return static_cast<uint16_t>((H << 8) | L); }

        void setBC(uint16_t value);
        void setDE(uint16_t value);
        void setHL(uint16_t value);

        uint8_t getReg8(uint8_t code) const;
        void setReg8(uint8_t code, uint8_t value);

        void compareA(uint8_t value);

        void initializeOpcodeTable();

        int unimplementedOpcode(uint8_t opcode, uint16_t pc);

        int opCALLImm16();
        int opRET();

        int opCPImm();

        inline int opNOP() { return 4;}
        int opHALT();

        int opLDAImm();
        int opLDBImm();
        int opLDCImm();
        int opLDDImm();
        int opLDEImm();
        int opLDHImm();
        int opLDLImm();

        int opLDBCImm16();
        int opLDDEImm16();
        int opLDHLImm16();
        int opLDSPImm16();

        int opJPNZImm16();
        int opJPImm16();

        int opOUTImmA();

        int opLDHLFromImm16Address();
        int opLDAddrImm16FromHL();

        int executeCB();
        int executeED();
        int executeDD();
        int executeFD();
};

#endif // CPU_H

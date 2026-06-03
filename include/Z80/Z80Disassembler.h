// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef Z80DISASSEMBLER_H_INCLUDED
#define Z80DISASSEMBLER_H_INCLUDED

#include <cstdint>
#include <functional>
#include <string>
#include "Z80/OpcodeZ80.h"

struct Z80DisassembledInstruction
{
    uint16_t address = 0;
    uint8_t bytes[4] = {0, 0, 0, 0};
    uint8_t size = 0;
    std::string text;
};

class Z80Disassembler
{
    public:
        using ReadByteCallback = std::function<uint8_t(uint16_t)>;

        explicit Z80Disassembler(ReadByteCallback reader);

        Z80DisassembledInstruction disassemble(uint16_t address) const;

    private:
        ReadByteCallback readByte;

        uint8_t read(uint16_t address) const;

        Z80DisassembledInstruction disassembleWithTable(
            uint16_t address,
            const Z80InstructionInfo& info) const;

        std::string formatInstruction(
            const Z80InstructionInfo& info,
            uint16_t address,
            const uint8_t* bytes,
            uint8_t size) const;

        std::string formatOperand(
            Z80OperandMode operand,
            uint16_t address,
            const uint8_t* bytes,
            uint8_t size) const;

        static std::string hex8(uint8_t value);
        static std::string hex16(uint16_t value);
};

#endif

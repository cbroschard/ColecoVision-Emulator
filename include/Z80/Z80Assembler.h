// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef Z80ASSEMBLER_H
#define Z80ASSEMBLER_H

#include <cstdint>
#include <string>
#include <vector>
#include "Z80/OpcodeZ80.h"

class Z80Assembler
{
    public:
        struct AssembledInstruction
        {
            uint16_t startAddress = 0;
            uint16_t nextAddress = 0;

            uint8_t opcode = 0;
            Z80OpcodePage page = Z80OpcodePage::Main;

            std::vector<uint8_t> bytes;
        };

        Z80Assembler();
        virtual ~Z80Assembler();

        AssembledInstruction assembleLine(const std::string& line, uint16_t address);

    protected:

    private:
        struct ParsedOperand
        {
            Z80OperandMode mode = Z80OperandMode::None;

            uint16_t value = 0;
            bool hasValue = false;
        };

        ParsedOperand parseOperand(
            const std::string& operand,
            const std::string& mnemonic,
            int operandIndex);

        const Z80InstructionInfo& lookupInstruction(
            const std::string& mnemonic,
            const ParsedOperand& operand1,
            const ParsedOperand& operand2) const;

        static std::string trim(const std::string& text);
        static std::string toUpper(std::string text);

        static std::vector<std::string> splitOperands(
            const std::string& operandText);

        static uint16_t parseNumber(const std::string& text);

        static void appendImmediateBytes(
            std::vector<uint8_t>& bytes,
            const Z80InstructionInfo& info,
            const ParsedOperand& operand1,
            const ParsedOperand& operand2,
            uint16_t instructionAddress);
};

#endif // Z80ASSEMBLER_H

// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Z80/Z80Assembler.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

Z80Assembler::Z80Assembler() = default;

Z80Assembler::~Z80Assembler() = default;

Z80Assembler::AssembledInstruction Z80Assembler::assembleLine(const std::string& line, uint16_t address)
{
    const std::string cleanedLine = trim(line);

    if (cleanedLine.empty())
        throw std::runtime_error("Cannot assemble an empty line.");

    std::istringstream input(cleanedLine);

    std::string mnemonic;
    input >> mnemonic;

    mnemonic = toUpper(mnemonic);

    std::string operandText;
    std::getline(input, operandText);
    operandText = trim(operandText);

    const std::vector<std::string> operands =
        splitOperands(operandText);

    if (operands.size() > 2)
    {
        throw std::runtime_error(
            "Too many operands for instruction: " + line);
    }

    ParsedOperand operand1;
    ParsedOperand operand2;

    if (!operands.empty())
    {
        operand1 = parseOperand(
            operands[0],
            mnemonic,
            0);
    }

    if (operands.size() >= 2)
       operand2 = parseOperand(operands[1], mnemonic, 1);

    const Z80InstructionInfo& info = lookupInstruction(mnemonic, operand1, operand2);

    std::vector<uint8_t> bytes;

    if (info.page == Z80OpcodePage::CB)
        bytes.push_back(0xCB);

    bytes.push_back(info.opcode);

    appendImmediateBytes(bytes, info, operand1, operand2, address);

    if (bytes.size() != info.size)
    {
        throw std::runtime_error(
            "Internal assembler error: encoded size does not match "
            "opcode table length for " +
            mnemonic);
    }

    AssembledInstruction result;

    result.startAddress = address;
    result.nextAddress =
        static_cast<uint16_t>(
            address + bytes.size());

    result.opcode = info.opcode;
    result.page = info.page;
    result.bytes = std::move(bytes);

    return result;
}

Z80Assembler::ParsedOperand
Z80Assembler::parseOperand(
    const std::string& operand,
    const std::string& mnemonic,
    int operandIndex)
{
    ParsedOperand result;

    const std::string instruction =
        toUpper(trim(mnemonic));

    std::string op =
        toUpper(trim(operand));

    op.erase(
        std::remove_if(
            op.begin(),
            op.end(),
            [](unsigned char ch)
            {
                return std::isspace(ch) != 0;
            }),
        op.end());

    if (op.empty())
        return result;

    const bool isConditionalInstruction =
        instruction == "JR"   ||
        instruction == "JP"   ||
        instruction == "CALL" ||
        instruction == "RET";

    /*
     * Conditions must be considered before ordinary registers because
     * C can mean either register C or the carry condition.
     */
    if (isConditionalInstruction && operandIndex == 0)
    {
        if (op == "NZ")
        {
            result.mode = Z80OperandMode::CondNZ;
            return result;
        }

        if (op == "Z")
        {
            result.mode = Z80OperandMode::CondZ;
            return result;
        }

        if (op == "NC")
        {
            result.mode = Z80OperandMode::CondNC;
            return result;
        }

        if (op == "C")
        {
            result.mode = Z80OperandMode::CondC;
            return result;
        }

        if (op == "PO")
        {
            result.mode = Z80OperandMode::CondPO;
            return result;
        }

        if (op == "PE")
        {
            result.mode = Z80OperandMode::CondPE;
            return result;
        }

        if (op == "P")
        {
            result.mode = Z80OperandMode::CondP;
            return result;
        }

        if (op == "M")
        {
            result.mode = Z80OperandMode::CondM;
            return result;
        }
    }

    // 8-bit registers
    if (op == "A")
    {
        result.mode = Z80OperandMode::A;
    }
    else if (op == "B")
    {
        result.mode = Z80OperandMode::B;
    }
    else if (op == "C")
    {
        result.mode = Z80OperandMode::C;
    }
    else if (op == "D")
    {
        result.mode = Z80OperandMode::D;
    }
    else if (op == "E")
    {
        result.mode = Z80OperandMode::E;
    }
    else if (op == "H")
    {
        result.mode = Z80OperandMode::H;
    }
    else if (op == "L")
    {
        result.mode = Z80OperandMode::L;
    }

    // 16-bit registers
    else if (op == "AF")
    {
        result.mode = Z80OperandMode::AF;
    }
    else if (op == "BC")
    {
        result.mode = Z80OperandMode::BC;
    }
    else if (op == "DE")
    {
        result.mode = Z80OperandMode::DE;
    }
    else if (op == "HL")
    {
        result.mode = Z80OperandMode::HL;
    }
    else if (op == "SP")
    {
        result.mode = Z80OperandMode::SP;
    }
    else if (op == "IX")
    {
        result.mode = Z80OperandMode::IX;
    }
    else if (op == "IY")
    {
        result.mode = Z80OperandMode::IY;
    }

    // Alternate and special registers
    else if (op == "AF'")
    {
        result.mode = Z80OperandMode::AFPrime;
    }
    else if (op == "I")
    {
        result.mode = Z80OperandMode::I;
    }
    else if (op == "R")
    {
        result.mode = Z80OperandMode::R;
    }

    // Undocumented index-register halves
    else if (op == "IXH")
    {
        result.mode = Z80OperandMode::IXH;
    }
    else if (op == "IXL")
    {
        result.mode = Z80OperandMode::IXL;
    }
    else if (op == "IYH")
    {
        result.mode = Z80OperandMode::IYH;
    }
    else if (op == "IYL")
    {
        result.mode = Z80OperandMode::IYL;
    }

    // Register-indirect memory operands
    else if (op == "(BC)")
    {
        result.mode = Z80OperandMode::AddrBC;
    }
    else if (op == "(DE)")
    {
        result.mode = Z80OperandMode::AddrDE;
    }
    else if (op == "(HL)")
    {
        result.mode = Z80OperandMode::AddrHL;
    }
    else if (op == "(SP)")
    {
        result.mode = Z80OperandMode::AddrSP;
    }
    else if (op == "(IX)")
    {
        result.mode = Z80OperandMode::AddrIX;
    }
    else if (op == "(IY)")
    {
        result.mode = Z80OperandMode::AddrIY;
    }
    else if (op == "(C)")
    {
        result.mode = Z80OperandMode::AddrC;
    }

    /*
     * Indexed memory operands:
     *
     *     (IX+5)
     *     (IX-$10)
     *     (IY+0)
     *     (IY-128)
     *
     * The displacement is stored as its signed 8-bit encoded value.
     */
    else if ((op.rfind("(IX+", 0) == 0 ||
              op.rfind("(IX-", 0) == 0 ||
              op.rfind("(IY+", 0) == 0 ||
              op.rfind("(IY-", 0) == 0) &&
             op.back() == ')')
    {
        const bool useIX =
            op[1] == 'I' &&
            op[2] == 'X';

        const char sign = op[3];

        const std::string displacementText =
            op.substr(4, op.size() - 5);

        if (displacementText.empty())
        {
            throw std::runtime_error(
                "Missing index displacement: " +
                operand);
        }

        const uint16_t magnitude =
            parseNumber(displacementText);

        int displacement =
            static_cast<int>(magnitude);

        if (sign == '-')
            displacement = -displacement;

        if (displacement < -128 ||
            displacement > 127)
        {
            throw std::runtime_error(
                "Index displacement is out of range: " +
                operand);
        }

        result.mode =
            useIX
                ? Z80OperandMode::AddrIXd
                : Z80OperandMode::AddrIYd;

        result.value =
            static_cast<uint8_t>(
                static_cast<int8_t>(
                    displacement));

        result.hasValue = true;
    }

    // Interrupt modes: IM 0, IM 1 and IM 2
    else if (instruction == "IM" &&
             operandIndex == 0)
    {
        const uint16_t mode =
            parseNumber(op);

        result.value = mode;
        result.hasValue = true;

        switch (mode)
        {
            case 0:
                result.mode =
                    Z80OperandMode::IM0;
                break;

            case 1:
                result.mode =
                    Z80OperandMode::IM1;
                break;

            case 2:
                result.mode =
                    Z80OperandMode::IM2;
                break;

            default:
                throw std::runtime_error(
                    "Interrupt mode must be 0, 1 or 2.");
        }
    }

    // BIT, RES and SET bit-number operand
    else if ((instruction == "BIT" ||
              instruction == "RES" ||
              instruction == "SET") &&
             operandIndex == 0)
    {
        const uint16_t bit =
            parseNumber(op);

        if (bit > 7)
        {
            throw std::runtime_error(
                "Bit number must be between 0 and 7.");
        }

        result.mode =
            static_cast<Z80OperandMode>(
                static_cast<int>(
                    Z80OperandMode::Bit0) +
                static_cast<int>(bit));

        result.value = bit;
        result.hasValue = true;
    }

    // RST vectors
    else if (instruction == "RST" &&
             operandIndex == 0)
    {
        const uint16_t value =
            parseNumber(op);

        result.value = value;
        result.hasValue = true;

        switch (value)
        {
            case 0x00:
                result.mode =
                    Z80OperandMode::Rst00;
                break;

            case 0x08:
                result.mode =
                    Z80OperandMode::Rst08;
                break;

            case 0x10:
                result.mode =
                    Z80OperandMode::Rst10;
                break;

            case 0x18:
                result.mode =
                    Z80OperandMode::Rst18;
                break;

            case 0x20:
                result.mode =
                    Z80OperandMode::Rst20;
                break;

            case 0x28:
                result.mode =
                    Z80OperandMode::Rst28;
                break;

            case 0x30:
                result.mode =
                    Z80OperandMode::Rst30;
                break;

            case 0x38:
                result.mode =
                    Z80OperandMode::Rst38;
                break;

            default:
                throw std::runtime_error(
                    "Invalid Z80 RST vector: " +
                    operand);
        }
    }

    /*
     * OUT (C),0 has a special ED-prefixed encoding.
     *
     * This must be handled before ordinary numeric operands so that
     * zero becomes Const0 instead of Imm8.
     */
    else if (instruction == "OUT" &&
             operandIndex == 1 &&
             op == "0")
    {
        result.mode =
            Z80OperandMode::Const0;

        result.value = 0;
        result.hasValue = true;
    }

    /*
     * Parenthesized immediate address or port.
     *
     * IN A,(n) and OUT (n),A use an 8-bit I/O port.
     * Other parenthesized numeric operands use a 16-bit address.
     */
    else if (op.front() == '(' &&
             op.back() == ')')
    {
        const std::string inner =
            op.substr(1, op.size() - 2);

        if (inner.empty())
        {
            throw std::runtime_error(
                "Empty memory operand: " +
                operand);
        }

        const uint16_t value =
            parseNumber(inner);

        result.value = value;
        result.hasValue = true;

        if (instruction == "IN" ||
            instruction == "OUT")
        {
            if (value > 0xFF)
            {
                throw std::runtime_error(
                    "Immediate I/O port is out of range: " +
                    operand);
            }

            result.mode =
                Z80OperandMode::AddrN;
        }
        else
        {
            result.mode =
                Z80OperandMode::AddrNN;
        }
    }

    // Ordinary numeric operands
    else
    {
        const uint16_t value =
            parseNumber(op);

        result.value = value;
        result.hasValue = true;

        /*
         * JR and DJNZ operands are target addresses. The assembler's
         * emission stage should calculate:
         *
         *     target - addressAfterInstruction
         */
        if (instruction == "JR" ||
            instruction == "DJNZ")
        {
            result.mode =
                Z80OperandMode::Rel8;
        }
        else if (value <= 0xFF)
        {
            result.mode =
                Z80OperandMode::Imm8;
        }
        else
        {
            result.mode =
                Z80OperandMode::Imm16;
        }
    }

    return result;
}

const Z80InstructionInfo&
Z80Assembler::lookupInstruction(
    const std::string& mnemonic,
    const ParsedOperand& operand1,
    const ParsedOperand& operand2) const
{
    const std::string instruction =
        toUpper(trim(mnemonic));

    /*
     * Search pages in the order normally preferred by the assembler.
     *
     * Main must come first because many ordinary instructions have
     * prefixed counterparts that use similar operand combinations.
     */
    static constexpr Z80OpcodePage lookupOrder[] =
    {
        Z80OpcodePage::Main,
        Z80OpcodePage::CB,
        Z80OpcodePage::ED,
        Z80OpcodePage::DD,
        Z80OpcodePage::FD,
        Z80OpcodePage::DDCB,
        Z80OpcodePage::FDCB
    };

    auto tryLookup =
        [&](Z80OperandMode mode1,
            Z80OperandMode mode2)
            -> const Z80InstructionInfo*
    {
        for (const Z80OpcodePage page : lookupOrder)
        {
            const Z80MnemonicKey key
            {
                instruction,
                mode1,
                mode2,
                page
            };

            const auto iterator =
                Z80_MNEMONIC_TO_OPCODE.find(key);

            if (iterator !=
                Z80_MNEMONIC_TO_OPCODE.end())
            {
                return &iterator->second;
            }
        }

        return nullptr;
    };

    // First try the exact operand modes produced by the parser.
    if (const Z80InstructionInfo* info =
            tryLookup(
                operand1.mode,
                operand2.mode))
    {
        return *info;
    }

    /*
     * A numeric literal such as $0040 fits in eight bits, but an
     * instruction such as JP $0040, CALL $0040 or LD HL,$0040
     * still requires an Imm16 operand.
     */
    if (operand1.hasValue &&
        operand1.mode == Z80OperandMode::Imm8)
    {
        if (const Z80InstructionInfo* info =
                tryLookup(
                    Z80OperandMode::Imm16,
                    operand2.mode))
        {
            return *info;
        }
    }

    if (operand2.hasValue &&
        operand2.mode == Z80OperandMode::Imm8)
    {
        if (const Z80InstructionInfo* info =
                tryLookup(
                    operand1.mode,
                    Z80OperandMode::Imm16))
        {
            return *info;
        }
    }

    if (instruction == "JR" ||
        instruction == "DJNZ")
    {
        if (operand1.hasValue &&
            operand2.mode == Z80OperandMode::None)
        {
            if (const Z80InstructionInfo* info =
                    tryLookup(
                        Z80OperandMode::Rel8,
                        Z80OperandMode::None))
            {
                return *info;
            }
        }

        /*
         * Conditional JR:
         *
         *     JR NZ,address
         *     JR Z,address
         *     JR NC,address
         *     JR C,address
         */
        if (instruction == "JR" &&
            operand2.hasValue)
        {
            if (const Z80InstructionInfo* info =
                    tryLookup(
                        operand1.mode,
                        Z80OperandMode::Rel8))
            {
                return *info;
            }
        }
    }

    if ((instruction == "IN" ||
         instruction == "OUT"))
    {
        if (operand1.hasValue &&
            operand1.mode == Z80OperandMode::AddrNN &&
            operand1.value <= 0xFF)
        {
            if (const Z80InstructionInfo* info =
                    tryLookup(
                        Z80OperandMode::AddrN,
                        operand2.mode))
            {
                return *info;
            }
        }

        if (operand2.hasValue &&
            operand2.mode == Z80OperandMode::AddrNN &&
            operand2.value <= 0xFF)
        {
            if (const Z80InstructionInfo* info =
                    tryLookup(
                        operand1.mode,
                        Z80OperandMode::AddrN))
            {
                return *info;
            }
        }
    }

    throw std::runtime_error(
        "Unsupported Z80 instruction: " +
        instruction +
        " (" +
        std::to_string(
            static_cast<int>(operand1.mode)) +
        ", " +
        std::to_string(
            static_cast<int>(operand2.mode)) +
        ")");
}

std::string Z80Assembler::trim(
    const std::string& text)
{
    const auto first =
        std::find_if_not(
            text.begin(),
            text.end(),
            [](unsigned char ch)
            {
                return std::isspace(ch) != 0;
            });

    if (first == text.end())
        return "";

    const auto last =
        std::find_if_not(
            text.rbegin(),
            text.rend(),
            [](unsigned char ch)
            {
                return std::isspace(ch) != 0;
            }).base();

    return std::string(first, last);
}

std::string Z80Assembler::toUpper(
    std::string text)
{
    std::transform(
        text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char ch)
        {
            return static_cast<char>(
                std::toupper(ch));
        });

    return text;
}

std::vector<std::string>
Z80Assembler::splitOperands(
    const std::string& operandText)
{
    std::vector<std::string> operands;

    const std::string text = trim(operandText);

    if (text.empty())
        return operands;

    int parenthesisDepth = 0;
    std::size_t start = 0;

    for (std::size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == '(')
        {
            ++parenthesisDepth;
        }
        else if (text[i] == ')')
        {
            --parenthesisDepth;

            if (parenthesisDepth < 0)
            {
                throw std::runtime_error(
                    "Unmatched closing parenthesis.");
            }
        }
        else if (text[i] == ',' &&
                 parenthesisDepth == 0)
        {
            operands.push_back(
                trim(text.substr(start, i - start)));

            start = i + 1;
        }
    }

    if (parenthesisDepth != 0)
    {
        throw std::runtime_error(
            "Unmatched opening parenthesis.");
    }

    operands.push_back(
        trim(text.substr(start)));

    return operands;
}

uint16_t Z80Assembler::parseNumber(
    const std::string& text)
{
    std::string valueText = trim(text);

    if (valueText.empty())
        throw std::runtime_error("Missing numeric value.");

    int base = 16;

    if (valueText.front() == '$')
    {
        valueText.erase(valueText.begin());
    }
    else if (valueText.size() >= 2 &&
             valueText[0] == '0' &&
             (valueText[1] == 'X' ||
              valueText[1] == 'x'))
    {
        valueText = valueText.substr(2);
    }

    if (valueText.empty())
        throw std::runtime_error("Missing numeric value.");

    std::size_t consumed = 0;

    const unsigned long value =
        std::stoul(
            valueText,
            &consumed,
            base);

    if (consumed != valueText.size())
    {
        throw std::runtime_error(
            "Invalid numeric value: " + text);
    }

    if (value > 0xFFFF)
    {
        throw std::runtime_error(
            "Numeric value is larger than 16 bits: " +
            text);
    }

    return static_cast<uint16_t>(value);
}

void Z80Assembler::appendImmediateBytes(
    std::vector<uint8_t>& bytes,
    const Z80InstructionInfo& info,
    const ParsedOperand& operand1,
    const ParsedOperand& operand2,
    uint16_t instructionAddress)
{
    const ParsedOperand* valueOperand = nullptr;

    if (operand1.hasValue)
        valueOperand = &operand1;

    if (operand2.hasValue)
        valueOperand = &operand2;

    if (info.size <= bytes.size())
        return;

    if (valueOperand == nullptr)
    {
        throw std::runtime_error(
            "Instruction requires an immediate operand.");
    }

    uint16_t value = valueOperand->value;

    if (valueOperand->mode == Z80OperandMode::Rel8)
    {
        const uint16_t nextAddress =
            static_cast<uint16_t>(
                instructionAddress + info.size);

        const int32_t displacement =
            static_cast<int32_t>(value) -
            static_cast<int32_t>(nextAddress);

        if (displacement < -128 ||
            displacement > 127)
        {
            throw std::runtime_error(
                "JR target is outside the signed "
                "8-bit relative range.");
        }

        value =
            static_cast<uint8_t>(
                static_cast<int8_t>(
                    displacement));
    }

    if (info.size > bytes.size())
    {
        bytes.push_back(
            static_cast<uint8_t>(
                value & 0x00FF));
    }

    if (info.size > bytes.size())
    {
        bytes.push_back(
            static_cast<uint8_t>(
                (value >> 8) & 0x00FF));
    }
}

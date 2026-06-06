// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Z80/Z80Disassembler.h"
#include <iomanip>
#include <sstream>

Z80Disassembler::Z80Disassembler(ReadByteCallback reader) :
    readByte(reader)
{

}

uint8_t Z80Disassembler::read(uint16_t address) const
{
    if (!readByte)
        return 0xFF;

    return readByte(address);
}

std::string Z80Disassembler::hex8(uint8_t value)
{
    std::ostringstream out;
    out << "$"
        << std::uppercase
        << std::hex
        << std::setw(2)
        << std::setfill('0')
        << static_cast<int>(value);

    return out.str();
}

std::string Z80Disassembler::hex16(uint16_t value)
{
    std::ostringstream out;
    out << "$"
        << std::uppercase
        << std::hex
        << std::setw(4)
        << std::setfill('0')
        << static_cast<int>(value);

    return out.str();
}

Z80DisassembledInstruction Z80Disassembler::disassemble(uint16_t address) const
{
    const uint8_t opcode = read(address);

    if (opcode == 0xCB)
    {
        const uint8_t cbOpcode = read(static_cast<uint16_t>(address + 1));
        return disassembleWithTable(address, Z80_CB_OPCODES[cbOpcode]);
    }

    if (opcode == 0xED)
    {
        const uint8_t edOpcode = read(static_cast<uint16_t>(address + 1));
        return disassembleWithTable(address, Z80_ED_OPCODES[edOpcode]);
    }

    if (opcode == 0xDD)
    {
        const uint8_t next = read(static_cast<uint16_t>(address + 1));

        if (next == 0xCB)
        {
            // DD CB d opcode
            const uint8_t cbOpcode = read(static_cast<uint16_t>(address + 3));
            return disassembleWithTable(address, Z80_DDCB_OPCODES[cbOpcode]);
        }

        return disassembleWithTable(address, Z80_DD_OPCODES[next]);
    }

    if (opcode == 0xFD)
    {
        const uint8_t next = read(static_cast<uint16_t>(address + 1));

        if (next == 0xCB)
        {
            // FD CB d opcode
            const uint8_t cbOpcode = read(static_cast<uint16_t>(address + 3));
            return disassembleWithTable(address, Z80_FDCB_OPCODES[cbOpcode]);
        }

        return disassembleWithTable(address, Z80_FD_OPCODES[next]);
    }

    return disassembleWithTable(address, Z80_MAIN_OPCODES[opcode]);
}

Z80DisassembledInstruction Z80Disassembler::disassembleWithTable(
    uint16_t address,
    const Z80InstructionInfo& info) const
{
    Z80DisassembledInstruction result;
    result.address = address;
    result.size = info.size;

    for (uint8_t i = 0; i < result.size && i < 4; ++i)
        result.bytes[i] = read(static_cast<uint16_t>(address + i));

    result.text = formatInstruction(info, address, result.bytes, result.size);

    return result;
}

std::string Z80Disassembler::formatInstruction(
    const Z80InstructionInfo& info,
    uint16_t address,
    const uint8_t* bytes,
    uint8_t size) const
{
    std::ostringstream out;

    out << info.mnemonic;

    const std::string op1 = formatOperand(info.op1, address, bytes, size);
    const std::string op2 = formatOperand(info.op2, address, bytes, size);

    if (!op1.empty())
        out << " " << op1;

    if (!op2.empty())
        out << "," << op2;

    return out.str();
}

std::string Z80Disassembler::formatOperand(
    Z80OperandMode operand,
    uint16_t address,
    const uint8_t* bytes,
    uint8_t size) const
{
    switch (operand)
    {
        case Z80OperandMode::None: return "";

        case Z80OperandMode::A: return "A";
        case Z80OperandMode::B: return "B";
        case Z80OperandMode::C: return "C";
        case Z80OperandMode::D: return "D";
        case Z80OperandMode::E: return "E";
        case Z80OperandMode::H: return "H";
        case Z80OperandMode::L: return "L";

        case Z80OperandMode::AF: return "AF";
        case Z80OperandMode::BC: return "BC";
        case Z80OperandMode::DE: return "DE";
        case Z80OperandMode::HL: return "HL";
        case Z80OperandMode::SP: return "SP";
        case Z80OperandMode::IX: return "IX";
        case Z80OperandMode::IY: return "IY";

        case Z80OperandMode::IXH: return "IXH";
        case Z80OperandMode::IXL: return "IXL";
        case Z80OperandMode::IYH: return "IYH";
        case Z80OperandMode::IYL: return "IYL";

        case Z80OperandMode::AFPrime: return "AF'";
        case Z80OperandMode::I: return "I";
        case Z80OperandMode::R: return "R";

        case Z80OperandMode::CondNZ: return "NZ";
        case Z80OperandMode::CondZ:  return "Z";
        case Z80OperandMode::CondNC: return "NC";
        case Z80OperandMode::CondC:  return "C";
        case Z80OperandMode::CondPO: return "PO";
        case Z80OperandMode::CondPE: return "PE";
        case Z80OperandMode::CondP:  return "P";
        case Z80OperandMode::CondM:  return "M";

        case Z80OperandMode::IM0: return "0";
        case Z80OperandMode::IM1: return "1";
        case Z80OperandMode::IM2: return "2";

        case Z80OperandMode::AddrBC: return "(BC)";
        case Z80OperandMode::AddrDE: return "(DE)";
        case Z80OperandMode::AddrHL: return "(HL)";
        case Z80OperandMode::AddrSP: return "(SP)";
        case Z80OperandMode::AddrC:  return "(C)";
        case Z80OperandMode::AddrIX: return "(IX)";
        case Z80OperandMode::AddrIY: return "(IY)";

        case Z80OperandMode::AddrN:
        {
            uint8_t value = 0;

            if (size >= 2)
                value = bytes[size - 1];

            return "(" + hex8(value) + ")";
        }

        case Z80OperandMode::AddrNN:
        {
            uint16_t value = 0;

            if (size >= 3)
                value = static_cast<uint16_t>(bytes[size - 2] | (bytes[size - 1] << 8));

            return "(" + hex16(value) + ")";
        }

        case Z80OperandMode::AddrIXd:
        {
            // DD normal: DD opcode d
            // DD CB:     DD CB d opcode
            int8_t d = 0;

            if (size == 4 && bytes[0] == 0xDD && bytes[1] == 0xCB)
                d = static_cast<int8_t>(bytes[2]);
            else if (size >= 3)
                d = static_cast<int8_t>(bytes[2]);

            std::ostringstream out;
            out << "(IX";

            if (d >= 0)
                out << "+" << hex8(static_cast<uint8_t>(d));
            else
                out << "-" << hex8(static_cast<uint8_t>(-d));

            out << ")";
            return out.str();
        }

        case Z80OperandMode::AddrIYd:
        {
            // FD normal: FD opcode d
            // FD CB:     FD CB d opcode
            int8_t d = 0;

            if (size == 4 && bytes[0] == 0xFD && bytes[1] == 0xCB)
                d = static_cast<int8_t>(bytes[2]);
            else if (size >= 3)
                d = static_cast<int8_t>(bytes[2]);

            std::ostringstream out;
            out << "(IY";

            if (d >= 0)
                out << "+" << hex8(static_cast<uint8_t>(d));
            else
                out << "-" << hex8(static_cast<uint8_t>(-d));

            out << ")";
            return out.str();
        }

        case Z80OperandMode::Imm8:
        {
            uint8_t value = 0;

            if (size >= 2)
                value = bytes[size - 1];

            return hex8(value);
        }

        case Z80OperandMode::Imm16:
        {
            uint16_t value = 0;

            if (size >= 3)
                value = static_cast<uint16_t>(bytes[size - 2] | (bytes[size - 1] << 8));

            return hex16(value);
        }

        case Z80OperandMode::Const0: return "0";

        case Z80OperandMode::Rel8:
        {
            int8_t offset = 0;

            if (size >= 2)
                offset = static_cast<int8_t>(bytes[size - 1]);

            const uint16_t target = static_cast<uint16_t>(address + size + offset);
            return hex16(target);
        }

        case Z80OperandMode::Bit0: return "0";
        case Z80OperandMode::Bit1: return "1";
        case Z80OperandMode::Bit2: return "2";
        case Z80OperandMode::Bit3: return "3";
        case Z80OperandMode::Bit4: return "4";
        case Z80OperandMode::Bit5: return "5";
        case Z80OperandMode::Bit6: return "6";
        case Z80OperandMode::Bit7: return "7";

        case Z80OperandMode::Rst00: return "$00";
        case Z80OperandMode::Rst08: return "$08";
        case Z80OperandMode::Rst10: return "$10";
        case Z80OperandMode::Rst18: return "$18";
        case Z80OperandMode::Rst20: return "$20";
        case Z80OperandMode::Rst28: return "$28";
        case Z80OperandMode::Rst30: return "$30";
        case Z80OperandMode::Rst38: return "$38";
    }

    return "";
}

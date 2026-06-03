// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef OPCODEZ80_H_INCLUDED
#define OPCODEZ80_H_INCLUDED

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

enum class Z80OpcodePage
{
    Main,
    CB,
    ED,
    DD,
    FD,
    DDCB,
    FDCB
};

enum class Z80OperandMode
{
    None,

    // 8-bit registers
    A,
    B,
    C,
    D,
    E,
    H,
    L,

    // 16-bit registers
    AF,
    BC,
    DE,
    HL,
    SP,
    IX,
    IY,

    // Alternate/special registers
    AFPrime,
    I,
    R,

    // Conditions
    CondNZ,
    CondZ,
    CondNC,
    CondC,
    CondPO,
    CondPE,
    CondP,
    CondM,

    // Interrupt modes
    IM0,
    IM1,
    IM2,

    // 8-bit index register halves, undocumented but real/useful
    IXH,
    IXL,
    IYH,
    IYL,

    // Memory references
    AddrBC,      // (BC)
    AddrDE,      // (DE)
    AddrHL,      // (HL)
    AddrSP,      // (SP)
    AddrNN,      // ($1234)
    AddrN,       // ($12), immediate I/O port for IN A,(n) / OUT (n),A
    AddrIX,      // (IX), for JP (IX)
    AddrIY,      // (IY)
    AddrIXd,     // (IX+d)
    AddrIYd,     // (IY+d)
    AddrC,       // (C), for IN/OUT through C port

    // Immediate/literal values
    Imm8,        // n
    Imm16,       // nn
    Const0,      // literal 0, for OUT (C),0

    // Relative branch offset
    Rel8,        // e

    // Bit instruction operand
    Bit0,
    Bit1,
    Bit2,
    Bit3,
    Bit4,
    Bit5,
    Bit6,
    Bit7,

    // RST target
    Rst00,
    Rst08,
    Rst10,
    Rst18,
    Rst20,
    Rst28,
    Rst30,
    Rst38
};

struct Z80InstructionInfo
{
    const char* mnemonic;
    Z80OperandMode op1;
    Z80OperandMode op2;
    uint8_t size;
    uint8_t opcode;
    Z80OpcodePage page;
};

struct Z80MnemonicKey
{
    std::string mnemonic;
    Z80OperandMode op1;
    Z80OperandMode op2;
    Z80OpcodePage page;

    bool operator==(const Z80MnemonicKey& other) const
    {
        return mnemonic == other.mnemonic &&
               op1 == other.op1 &&
               op2 == other.op2 &&
               page == other.page;
    }
};

struct Z80MnemonicKeyHash
{
    std::size_t operator()(const Z80MnemonicKey& key) const
    {
        std::size_t h1 = std::hash<std::string>{}(key.mnemonic);
        std::size_t h2 = std::hash<int>{}(static_cast<int>(key.op1));
        std::size_t h3 = std::hash<int>{}(static_cast<int>(key.op2));
        std::size_t h4 = std::hash<int>{}(static_cast<int>(key.page));

        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
};

extern const Z80InstructionInfo Z80_MAIN_OPCODES[256];
extern const Z80InstructionInfo Z80_CB_OPCODES[256];
extern const Z80InstructionInfo Z80_ED_OPCODES[256];
extern const Z80InstructionInfo Z80_DD_OPCODES[256];
extern const Z80InstructionInfo Z80_FD_OPCODES[256];

extern const std::array<Z80InstructionInfo, 256> Z80_DDCB_OPCODES;
extern const std::array<Z80InstructionInfo, 256> Z80_FDCB_OPCODES;

extern const std::unordered_map<Z80MnemonicKey, Z80InstructionInfo, Z80MnemonicKeyHash> Z80_MNEMONIC_TO_OPCODE;

#endif // OPCODEZ80_H_INCLUDED

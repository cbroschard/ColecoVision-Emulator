// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include <iostream>
#include <iomanip>
#include "CPU.h"

CPU::CPU() :
    bus(nullptr),
    A(0),
    F(0),
    B(0),
    C(0),
    D(0),
    E(0),
    H(0),
    L(0),
    A_(0),
    F_(0),
    B_(0),
    C_(0),
    D_(0),
    E_(0),
    H_(0),
    L_(0),
    IX(0),
    IY(0),
    PC(0),
    SP(0),
    I(0),
    R(0),
    IFF1(false),
    IFF2(false),
    IM(0),
    halted(false),
    irqPending(false),
    nmiPending(false),
    eiDelay(false),
    cycles(0)
{
    initializeOpcodeTable();
}

CPU::~CPU()
{

}

void CPU::reset()
{
    // Main registers
    A       = 0;
    F       = 0;

    B       = 0;
    C       = 0;
    D       = 0;
    E       = 0;
    H       = 0;
    L       = 0;

    // Shadow registers
    A_      = 0;
    F_      = 0;

    B_      = 0;
    C_      = 0;
    D_      = 0;
    E_      = 0;
    H_      = 0;
    L_      = 0;

    // Index registers
    IX      = 0;
    IY      = 0;

    // Program Counter + Stack Pointer
    PC = 0x0000;
    SP = 0xFFFF;

    // Interrupts
    I       = 0;
    R       = 0;

    IFF1 = false;
    IFF2 = false;
    IM = 0;

    halted = false;
    irqPending = false;
    nmiPending = false;
    eiDelay = false;

    cycles = 0;
}

int CPU::step()
{
    if (halted)
    {
        cycles += 4;
        return 4;
    }

    const uint8_t opcode = fetch8();
    const int usedCycles = opcodeTable[opcode]();

    cycles += usedCycles;

    return usedCycles;
}

uint8_t CPU::fetch8()
{
    if (!bus)
    {
        halted = true;
        return 0xFF;
    }

    uint8_t value = bus->readMemory(PC);
    PC++;
    return value;
}

uint16_t CPU::fetch16()
{
    uint8_t lo = fetch8();
    uint8_t hi = fetch8();

    return static_cast<uint16_t>(lo | (hi << 8));
}

uint8_t CPU::read8(uint16_t address) const
{
    if (!bus)
        return 0xFF;

    return bus->readMemory(address);
}

void CPU::write8(uint16_t address, uint8_t value)
{
    if (!bus)
        return;

    bus->writeMemory(address, value);
}

void CPU::writeIO(uint8_t port, uint8_t value)
{
    if (!bus)
        return;

    bus->writeIO(port, value);
}

uint8_t CPU::readIO(uint8_t port)
{
    if (!bus)
        return 0xFF;

    return bus->readIO(port);
}

void CPU::push16(uint16_t value)
{
    SP--;
    write8(SP, static_cast<uint8_t>(value >> 8));

    SP--;
    write8(SP, static_cast<uint8_t>(value & 0xFF));
}

uint16_t CPU::pop16()
{
    const uint8_t lo = read8(SP);
    SP++;

    const uint8_t hi = read8(SP);
    SP++;

    return static_cast<uint16_t>(lo | (hi << 8));
}

void CPU::setBC(uint16_t value)
{
    B = static_cast<uint8_t>(value >> 8);
    C = static_cast<uint8_t>(value & 0xFF);
}

void CPU::setDE(uint16_t value)
{
    D = static_cast<uint8_t>(value >> 8);
    E = static_cast<uint8_t>(value & 0xFF);
}

void CPU::setHL(uint16_t value)
{
    H = static_cast<uint8_t>(value >> 8);
    L = static_cast<uint8_t>(value & 0xFF);
}

uint8_t CPU::getReg8(uint8_t code) const
{
    switch (code)
    {
        case 0: return B;
        case 1: return C;
        case 2: return D;
        case 3: return E;
        case 4: return H;
        case 5: return L;
        case 6: return read8(getHL()); // (HL)
        case 7: return A;
    }

    return 0xFF;
}

void CPU::setReg8(uint8_t code, uint8_t value)
{
    switch (code)
    {
        case 0: B = value; break;
        case 1: C = value; break;
        case 2: D = value; break;
        case 3: E = value; break;
        case 4: H = value; break;
        case 5: L = value; break;
        case 6: write8(getHL(), value); break; // (HL)
        case 7: A = value; break;
    }
}

void CPU::compareA(uint8_t value)
{
    const uint16_t result = static_cast<uint16_t>(A) - value;
    const uint8_t result8 = static_cast<uint8_t>(result & 0xFF);

    F = 0;

    // Sign
    if (result8 & 0x80)
        F |= FLAG_S;

    // Zero
    if (result8 == 0)
        F |= FLAG_Z;

    // Half borrow
    if ((A & 0x0F) < (value & 0x0F))
        F |= FLAG_H;

    // Overflow
    if (((A ^ value) & (A ^ result8) & 0x80) != 0)
        F |= FLAG_PV;

    // Subtract flag is always set for CP/SUB
    F |= FLAG_N;

    // Carry / borrow
    if (A < value)
        F |= FLAG_C;

    // Undocumented flags copied from result bits 5 and 3
    F |= result8 & (FLAG_Y | FLAG_X);
}

void CPU::initializeOpcodeTable()
{
    for (int i = 0; i < 256; ++i)
    {
        opcodeTable[i] = [this, i]()
        {
            const uint16_t opcodePC = static_cast<uint16_t>(PC - 1);
            return unimplementedOpcode(static_cast<uint8_t>(i), opcodePC);
        };
    }

    // LD r,r block: 0x40-0x7F, except 0x76 HALT
    for (int op = 0x40; op <= 0x7F; ++op)
    {
        if (op == 0x76)
            continue; // HALT already handled

        opcodeTable[op] = [this, op]()
        {
            const uint8_t dst = static_cast<uint8_t>((op >> 3) & 0x07);
            const uint8_t src = static_cast<uint8_t>(op & 0x07);

            const uint8_t value = getReg8(src);
            setReg8(dst, value);

            return (dst == 6 || src == 6) ? 7 : 4;
        };
    }

    // Calls / returns
    opcodeTable[0xCD] = [this]() { return opCALLImm16(); }; // CALL nn
    opcodeTable[0xC9] = [this]() { return opRET(); }; // RET

    // Compare
    opcodeTable[0xFE] = [this]() { return opCPImm(); }; // CP n

    // Control
    opcodeTable[0x00] = [this]() { return opNOP(); };
    opcodeTable[0x76] = [this]() { return opHALT(); };

    // Jumps
    opcodeTable[0xC2] = [this]() { return opJPNZImm16(); }; // JP NZ,nn
    opcodeTable[0xC3] = [this]() { return opJPImm16(); }; // JP nn

    // I/O
    opcodeTable[0xD3] = [this]() { return opOUTImmA(); }; // OUT (n),A

    // LD rr,nn
    opcodeTable[0x01] = [this]() { return opLDBCImm16(); }; // LD BC,nn
    opcodeTable[0x11] = [this]() { return opLDDEImm16(); }; // LD DE,nn
    opcodeTable[0x21] = [this]() { return opLDHLImm16(); }; // LD HL,nn
    opcodeTable[0x31] = [this]() { return opLDSPImm16(); }; // LD SP,nn

    // LD r,n
    opcodeTable[0x06] = [this]() { return opLDBImm(); }; // LD B,n
    opcodeTable[0x0E] = [this]() { return opLDCImm(); }; // LD C,n
    opcodeTable[0x16] = [this]() { return opLDDImm(); }; // LD D,n
    opcodeTable[0x1E] = [this]() { return opLDEImm(); }; // LD E,n
    opcodeTable[0x26] = [this]() { return opLDHImm(); }; // LD H,n
    opcodeTable[0x2E] = [this]() { return opLDLImm(); }; // LD L,n
    opcodeTable[0x3E] = [this]() { return opLDAImm(); }; // LD A,n

    // 16-bit loads
    opcodeTable[0x22] = [this]() { return opLDAddrImm16FromHL(); }; // LD (nn),HL
    opcodeTable[0x2A] = [this]() { return opLDHLFromImm16Address(); }; // LD HL,(nn)
}

int CPU::unimplementedOpcode(uint8_t opcode, uint16_t pc)
{
    std::cerr
        << "Unimplemented Z80 opcode $"
        << std::hex << std::uppercase
        << std::setw(2) << std::setfill('0')
        << static_cast<int>(opcode)
        << " at PC=$"
        << std::setw(4) << std::setfill('0')
        << pc
        << std::dec
        << std::nouppercase
        << std::endl;

    halted = true;

    return 4;
}

int CPU::opCALLImm16()
{
    const uint16_t address = fetch16();

    push16(PC);

    PC = address;

    return 17;
}

int CPU::opRET()
{
    PC = pop16();
    return 10;
}

int CPU::opCPImm()
{
    const uint8_t value = fetch8();
    compareA(value);
    return 7;
}

int CPU::opHALT()
{
    halted = true;
    return 4;
}

int CPU::opLDAImm()
{
    A = fetch8();
    return 7;
}

int CPU::opLDBImm()
{
    B = fetch8();
    return 7;
}

int CPU::opLDCImm()
{
    C = fetch8();
    return 7;
}

int CPU::opLDDImm()
{
    D = fetch8();
    return 7;
}

int CPU::opLDEImm()
{
    E = fetch8();
    return 7;
}

int CPU::opLDHImm()
{
    H = fetch8();
    return 7;
}

int CPU::opLDLImm()
{
    L = fetch8();
    return 7;
}

int CPU::opLDBCImm16()
{
    setBC(fetch16());
    return 10;
}

int CPU::opLDDEImm16()
{
    setDE(fetch16());
    return 10;
}

int CPU::opLDHLImm16()
{
    setHL(fetch16());
    return 10;
}

int CPU::opLDSPImm16()
{
    SP = fetch16();
    return 10;
}

int CPU::opJPNZImm16()
{
    const uint16_t address = fetch16();

    if ((F & FLAG_Z) == 0)
    {
        PC = address;
    }

    return 10;
}

int CPU::opJPImm16()
{
    PC = fetch16();
    return 10;
}

int CPU::opOUTImmA()
{
    const uint8_t port = fetch8();

    writeIO(port, A);

    return 11;
}

int CPU::opLDHLFromImm16Address()
{
    const uint16_t address = fetch16();

    const uint8_t lo = read8(address);
    const uint8_t hi = read8(static_cast<uint16_t>(address + 1));

    L = lo;
    H = hi;

    return 16;
}

int CPU::opLDAddrImm16FromHL()
{
    const uint16_t address = fetch16();

    write8(address, L);
    write8(static_cast<uint16_t>(address + 1), H);

    return 16;
}

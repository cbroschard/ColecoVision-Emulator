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

static bool parityEven(uint8_t value)
{
    value ^= value >> 4;
    value &= 0x0F;
    return ((0x6996 >> value) & 1) != 0;
}

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
    // NMI has priority and ignores IFF1.
    if (nmiPending)
    {
        const int usedCycles = serviceNMI();
        cycles += usedCycles;
        return usedCycles;
    }

    // Maskable IRQ only accepted when enabled.
    if (irqPending && IFF1 && !eiDelay)
    {
        const int usedCycles = serviceIRQ();
        cycles += usedCycles;
        return usedCycles;
    }

    if (halted)
    {
        cycles += 4;
        return 4;
    }

    const bool wasEIDelayed = eiDelay;

    const uint8_t opcode = fetch8();
    const int usedCycles = opcodeTable[opcode]();

    // If EI was pending before this instruction, enable interrupts now.
    // But do not immediately enable during the EI instruction itself.
    if (wasEIDelayed && opcode != 0xFB)
    {
        IFF1 = true;
        IFF2 = true;
        eiDelay = false;
    }

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

int CPU::serviceIRQ()
{
    // If CPU was halted, an interrupt wakes it.
    halted = false;

    // Maskable interrupt accepted: disable further maskable interrupts.
    IFF1 = false;
    IFF2 = false;

    // Push current PC just like a CALL.
    push16(PC);

    switch (IM)
    {
        case 0:
            // For now, treat IM 0 like IM 1.
            // Real IM 0 executes an instruction supplied by the bus.
            PC = 0x0038;
            return 13;

        case 1:
            // IM 1 always jumps to $0038.
            PC = 0x0038;
            return 13;

        case 2:
        {
            // IM 2 uses I register as high byte of vector table.
            // The low byte normally comes from the interrupting device.
            const uint16_t vectorAddress =
                static_cast<uint16_t>((static_cast<uint16_t>(I) << 8) | 0xFF);

            const uint8_t lo = read8(vectorAddress);
            const uint8_t hi = read8(static_cast<uint16_t>(vectorAddress + 1));

            PC = static_cast<uint16_t>(lo | (hi << 8));

            return 19;
        }

        default:
            PC = 0x0038;
            return 13;
    }
}

int CPU::serviceNMI()
{
    // NMI wakes HALT.
    halted = false;

    // Preserve old maskable interrupt enable state.
    IFF2 = IFF1;

    // Disable maskable interrupts.
    IFF1 = false;

    // NMI is edge-triggered, so clear pending after accepting.
    nmiPending = false;

    // Push current PC and jump to NMI vector.
    push16(PC);
    PC = 0x0066;

    return 11;
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

uint16_t CPU::add16Index(uint16_t lhs, uint16_t rhs)
{
    const uint32_t result = static_cast<uint32_t>(lhs) + rhs;
    const uint16_t result16 = static_cast<uint16_t>(result & 0xFFFF);

    // ADD IX,rr affects: H, N, C, undocumented X/Y from high byte.
    // It preserves S, Z, and P/V.
    const uint8_t preserved = F & (FLAG_S | FLAG_Z | FLAG_PV);

    F = preserved;

    // Half carry from bit 11 to bit 12
    if (((lhs & 0x0FFF) + (rhs & 0x0FFF)) > 0x0FFF)
        F |= FLAG_H;

    // N is reset for addition.

    if (result > 0xFFFF)
        F |= FLAG_C;

    // Undocumented flags from bits 13 and 11 of result,
    // represented as bits 5 and 3 of the high byte.
    F |= static_cast<uint8_t>((result16 >> 8) & (FLAG_Y | FLAG_X));

    return result16;
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

uint8_t CPU::inc8(uint8_t value)
{
    const uint8_t result = static_cast<uint8_t>(value + 1);

    // INC preserves carry.
    const uint8_t oldCarry = F & FLAG_C;

    F = oldCarry;

    if (result & 0x80) F |= FLAG_S;
    if (result == 0)   F |= FLAG_Z;
    if (result & 0x20) F |= FLAG_Y;
    if (result & 0x08) F |= FLAG_X;

    // Half carry from bit 3 to bit 4.
    if ((value & 0x0F) == 0x0F)
        F |= FLAG_H;

    // Overflow only when 0x7F becomes 0x80.
    if (value == 0x7F)
        F |= FLAG_PV;

    // N is cleared.
    return result;
}

uint8_t CPU::dec8(uint8_t value)
{
    const uint8_t result = static_cast<uint8_t>(value - 1);

    // Preserve carry. DEC does not affect C.
    const uint8_t carry = F & FLAG_C;

    F = carry;

    if (result & 0x80)
        F |= FLAG_S;

    if (result == 0)
        F |= FLAG_Z;

    // Half borrow from bit 4
    if ((value & 0x0F) == 0x00)
        F |= FLAG_H;

    // Overflow: 0x80 - 1 = 0x7F
    if (value == 0x80)
        F |= FLAG_PV;

    // DEC is subtraction
    F |= FLAG_N;

    // Undocumented flags from result bits 5 and 3
    F |= result & (FLAG_Y | FLAG_X);

    return result;
}

void CPU::orA(uint8_t value)
{
    A = static_cast<uint8_t>(A | value);

    F = 0;

    if (A & 0x80)
        F |= FLAG_S;

    if (A == 0)
        F |= FLAG_Z;

    // OR clears H, N, and C.

    // Parity flag
    uint8_t bits = A;
    bool parity = true;

    while (bits)
    {
        parity = !parity;
        bits &= static_cast<uint8_t>(bits - 1);
    }

    if (parity)
        F |= FLAG_PV;

    // Undocumented flags from result bits 5 and 3
    F |= A & (FLAG_Y | FLAG_X);
}

void CPU::andA(uint8_t value)
{
    A = static_cast<uint8_t>(A & value);

    F = 0;

    if (A & 0x80)
        F |= FLAG_S;

    if (A == 0)
        F |= FLAG_Z;

    // AND always sets H, clears N and C.
    F |= FLAG_H;

    if (parityEven(A))
        F |= FLAG_PV;

    F |= A & (FLAG_Y | FLAG_X);
}

void CPU::addA(uint8_t value)
{
    const uint16_t result = static_cast<uint16_t>(A) + value;
    const uint8_t result8 = static_cast<uint8_t>(result & 0xFF);

    F = 0;

    // Sign
    if (result8 & 0x80)
        F |= FLAG_S;

    // Zero
    if (result8 == 0)
        F |= FLAG_Z;

    // Half carry from bit 3 to bit 4
    if (((A & 0x0F) + (value & 0x0F)) > 0x0F)
        F |= FLAG_H;

    // Overflow: adding two same-sign values produced opposite sign
    if (((A ^ result8) & (value ^ result8) & 0x80) != 0)
        F |= FLAG_PV;

    // Carry
    if (result > 0xFF)
        F |= FLAG_C;

    // Undocumented flags from result bits 5 and 3
    F |= result8 & (FLAG_Y | FLAG_X);

    A = result8;
}

void CPU::adcA(uint8_t value)
{
    const uint8_t carry = (F & FLAG_C) ? 1 : 0;

    const uint16_t result =
        static_cast<uint16_t>(A) + value + carry;

    const uint8_t result8 = static_cast<uint8_t>(result & 0xFF);

    F = 0;

    if (result8 & 0x80)
        F |= FLAG_S;

    if (result8 == 0)
        F |= FLAG_Z;

    if (((A & 0x0F) + (value & 0x0F) + carry) > 0x0F)
        F |= FLAG_H;

    if (((A ^ result8) & (value ^ result8) & 0x80) != 0)
        F |= FLAG_PV;

    if (result > 0xFF)
        F |= FLAG_C;

    F |= result8 & (FLAG_Y | FLAG_X);

    A = result8;
}

void CPU::sbcA(uint8_t value)
{
    const uint8_t carry = (F & FLAG_C) ? 1 : 0;
    const uint16_t result = static_cast<uint16_t>(A) - value - carry;
    const uint8_t result8 = static_cast<uint8_t>(result & 0xFF);

    F = 0;

    if (result8 & 0x80) F |= FLAG_S;
    if (result8 == 0)   F |= FLAG_Z;

    if ((A & 0x0F) < ((value & 0x0F) + carry))
        F |= FLAG_H;

    if (((A ^ value) & (A ^ result8) & 0x80) != 0)
        F |= FLAG_PV;

    F |= FLAG_N;

    if (result & 0x100)
        F |= FLAG_C;

    F |= result8 & (FLAG_Y | FLAG_X);

    A = result8;
}

void CPU::xorA(uint8_t value)
{
    A = static_cast<uint8_t>(A ^ value);

    F = 0;

    if (A & 0x80)
        F |= FLAG_S;

    if (A == 0)
        F |= FLAG_Z;

    if (parityEven(A))
        F |= FLAG_PV;

    F |= A & (FLAG_Y | FLAG_X);
}

void CPU::addHL(uint16_t value)
{
    const uint16_t oldHL = getHL();
    const uint32_t result = static_cast<uint32_t>(oldHL) + value;
    const uint16_t result16 = static_cast<uint16_t>(result);

    // ADD HL,rr preserves S, Z, and P/V.
    F &= static_cast<uint8_t>(FLAG_S | FLAG_Z | FLAG_PV);

    // H is carry from bit 11.
    if (((oldHL & 0x0FFF) + (value & 0x0FFF)) > 0x0FFF)
        F |= FLAG_H;

    // C is carry from bit 15.
    if (result > 0xFFFF)
        F |= FLAG_C;

    // Undocumented flags from high byte of result.
    if (result16 & 0x2000)
        F |= FLAG_Y;

    if (result16 & 0x0800)
        F |= FLAG_X;

    // N is cleared by leaving it unset.
    setHL(result16);
}

void CPU::subA(uint8_t value)
{
    const uint16_t result = static_cast<uint16_t>(A) - value;
    const uint8_t result8 = static_cast<uint8_t>(result & 0xFF);

    F = 0;

    if (result8 & 0x80)
        F |= FLAG_S;

    if (result8 == 0)
        F |= FLAG_Z;

    // Half borrow from bit 4
    if ((A & 0x0F) < (value & 0x0F))
        F |= FLAG_H;

    // Overflow: subtracting different-sign values produced opposite sign
    if (((A ^ value) & (A ^ result8) & 0x80) != 0)
        F |= FLAG_PV;

    // SUB always sets N
    F |= FLAG_N;

    // Carry means borrow
    if (A < value)
        F |= FLAG_C;

    // Undocumented flags from result
    F |= result8 & (FLAG_Y | FLAG_X);

    A = result8;
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

    // ADD A,r: 0x80-0x87
    for (int op = 0x80; op <= 0x87; ++op)
    {
        opcodeTable[op] = [this, op]()
        {
            const uint8_t src = static_cast<uint8_t>(op & 0x07);
            addA(getReg8(src));
            return (src == 6) ? 7 : 4;
        };
    }

    // ADC A,r: 0x88-0x8F
    for (int op = 0x88; op <= 0x8F; ++op)
    {
        opcodeTable[op] = [this, op]()
        {
            const uint8_t src = static_cast<uint8_t>(op & 0x07);
            adcA(getReg8(src));
            return (src == 6) ? 7 : 4;
        };
    }

    // SUB r: 0x90-0x97
    for (int op = 0x90; op <= 0x97; ++op)
    {
        opcodeTable[op] = [this, op]()
        {
            const uint8_t src = static_cast<uint8_t>(op & 0x07);
            subA(getReg8(src));
            return (src == 6) ? 7 : 4;
        };
    }

    // SBC A,r: 0x98-0x9F
    for (int op = 0x98; op <= 0x9F; ++op)
    {
        opcodeTable[op] = [this, op]()
        {
            const uint8_t src = static_cast<uint8_t>(op & 0x07);
            sbcA(getReg8(src));
            return (src == 6) ? 7 : 4;
        };
    }

    // AND r: 0xA0-0xA7
    for (int op = 0xA0; op <= 0xA7; ++op)
    {
        opcodeTable[op] = [this, op]()
        {
            const uint8_t src = static_cast<uint8_t>(op & 0x07);
            andA(getReg8(src));
            return (src == 6) ? 7 : 4;
        };
    }

    // XOR r: 0xA8-0xAF
    for (int op = 0xA8; op <= 0xAF; ++op)
    {
        opcodeTable[op] = [this, op]()
        {
            const uint8_t src = static_cast<uint8_t>(op & 0x07);
            xorA(getReg8(src));
            return (src == 6) ? 7 : 4;
        };
    }

    // OR r: 0xB0-0xB7
    for (int op = 0xB0; op <= 0xB7; ++op)
    {
        opcodeTable[op] = [this, op]()
        {
            const uint8_t src = static_cast<uint8_t>(op & 0x07);
            orA(getReg8(src));
            return (src == 6) ? 7 : 4;
        };
    }

    // CP r: 0xB8-0xBF
    for (int op = 0xB8; op <= 0xBF; ++op)
    {
        opcodeTable[op] = [this, op]()
        {
            const uint8_t src = static_cast<uint8_t>(op & 0x07);
            compareA(getReg8(src));
            return (src == 6) ? 7 : 4;
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

    // Interrupt control
    opcodeTable[0xF3] = [this]() { return opDI(); }; // DI
    opcodeTable[0xFB] = [this]() { return opEI(); }; // EI

    // ALU / logic
    opcodeTable[0xC6] = [this]() { return opADDImm(); };  // ADD A,n
    opcodeTable[0xCE] = [this]() { return opADCImm(); };  // ADC A,n
    opcodeTable[0xD6] = [this]() { return opSUBImm(); };  // SUB n
    opcodeTable[0xE6] = [this]() { return opANDImm(); };  // AND n
    opcodeTable[0xF6] = [this]() { return opORImm(); };   // OR n

    // Calls / returns
    opcodeTable[0xC0] = [this]() { return opRETNZ(); }; // RET NZ
    opcodeTable[0xC8] = [this]() { return opRETZ(); }; // RET Z
    opcodeTable[0xC9] = [this]() { return opRET(); }; // RET
    opcodeTable[0xD8] = [this]() { return opRETC(); };  // RET C
    opcodeTable[0xD0] = [this]() { return opRETNC(); }; // RET NC

    opcodeTable[0xC4] = [this]() { return opCALLNZImm16(); }; // CALL NZ,nn
    opcodeTable[0xCC] = [this]() { return opCALLZImm16(); };  // CALL Z,nn
    opcodeTable[0xCD] = [this]() { return opCALLImm16(); };   // CALL nn

    // Stack
    opcodeTable[0xC5] = [this]() { return opPUSHBC(); }; // PUSH BC
    opcodeTable[0xD5] = [this]() { return opPUSHDE(); }; // PUSH DE
    opcodeTable[0xE5] = [this]() { return opPUSHHL(); }; // PUSH HL
    opcodeTable[0xF5] = [this]() { return opPUSHAF(); }; // PUSH AF

    opcodeTable[0xC1] = [this]() { return opPOPBC(); }; // POP BC
    opcodeTable[0xD1] = [this]() { return opPOPDE(); }; // POP DE
    opcodeTable[0xE1] = [this]() { return opPOPHL(); }; // POP HL
    opcodeTable[0xF1] = [this]() { return opPOPAF(); }; // POP AF

    // Compare
    opcodeTable[0xFE] = [this]() { return opCPImm(); }; // CP n

    // Exchange
    opcodeTable[0x08] = [this]() { return opEXAFAFShadow(); }; // EX AF,AF
    opcodeTable[0xD9] = [this]() { return opEXX(); };   // EXX
    opcodeTable[0xE3] = [this]() { return opEXSPHL(); }; // EX (SP),HL
    opcodeTable[0xEB] = [this]() { return opEXDEHL(); }; // EX DE,HL

    // Control
    opcodeTable[0x00] = [this]() { return opNOP(); };
    opcodeTable[0x76] = [this]() { return opHALT(); };

    // INC / DEC
    opcodeTable[0x03] = [this]() { return opINCBC(); }; // INC BC
    opcodeTable[0x04] = [this]() { return opINCB(); }; // INC B
    opcodeTable[0x0C] = [this]() { return opINCC(); }; // INC C
    opcodeTable[0x13] = [this]() { return opINCDE(); }; // INC DE
    opcodeTable[0x14] = [this]() { return opINCD(); }; // INC D
    opcodeTable[0x1C] = [this]() { return opINCE(); }; // INC E
    opcodeTable[0x23] = [this]() { return opINCHL(); }; // INC HL
    opcodeTable[0x24] = [this]() { return opINCH(); }; // INC H
    opcodeTable[0x2C] = [this]() { return opINCL(); }; // INC L
    opcodeTable[0x33] = [this]() { return opINCSP(); }; // INC SP
    opcodeTable[0x3C] = [this]() { return opINCA(); }; // INC A

    opcodeTable[0x05] = [this]() { return opDECB(); }; // DEC B
    opcodeTable[0x0D] = [this]() { return opDECC(); }; // DEC C
    opcodeTable[0x15] = [this]() { return opDECD(); }; // DEC D
    opcodeTable[0x1D] = [this]() { return opDECE(); }; // DEC E
    opcodeTable[0x25] = [this]() { return opDECH(); }; // DEC H
    opcodeTable[0x2D] = [this]() { return opDECL(); }; // DEC L
    opcodeTable[0x3D] = [this]() { return opDECA(); }; // DEC A

    // 16-bit INC / DEC
    opcodeTable[0x0B] = [this]() { return opDECBC(); }; // DEC BC
    opcodeTable[0x1B] = [this]() { return opDECDE(); }; // DEC DE
    opcodeTable[0x2B] = [this]() { return opDECHL(); }; // DEC HL
    opcodeTable[0x3B] = [this]() { return opDECSP(); }; // DEC SP

    // 16-bit ADD HL,rr
    opcodeTable[0x09] = [this]() { return opADDHLBC(); }; // ADD HL,BC
    opcodeTable[0x19] = [this]() { return opADDHLDE(); }; // ADD HL,DE
    opcodeTable[0x29] = [this]() { return opADDHLHL(); }; // ADD HL,HL
    opcodeTable[0x39] = [this]() { return opADDHLSP(); }; // ADD HL,SP

    // Jumps
    opcodeTable[0xC2] = [this]() { return opJPNZImm16(); }; // JP NZ,nn
    opcodeTable[0xC3] = [this]() { return opJPImm16(); };   // JP nn
    opcodeTable[0xCA] = [this]() { return opJPZImm16(); };  // JP Z,nn
    opcodeTable[0xD2] = [this]() { return opJPNCImm16(); }; // JP NC,nn
    opcodeTable[0xDA] = [this]() { return opJPCImm16(); };  // JP C,nn
    opcodeTable[0xE9] = [this]() { return opJPHL(); }; // JP (HL)
    opcodeTable[0xF2] = [this]() { return opJPPImm16(); };  // JP P,nn
    opcodeTable[0xFA] = [this]() { return opJPMImm16(); };   // JP M,nn

    // I/O
    opcodeTable[0xD3] = [this]() { return opOUTImmA(); }; // OUT (n),A
    opcodeTable[0xDB] = [this]() { return opINAImm(); };  // IN A,(n)

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
    opcodeTable[0x36] = [this]() { return opLDHLAddrImm(); }; // LD (HL),n
    opcodeTable[0x3E] = [this]() { return opLDAImm(); }; // LD A,n

    // 8-bit loads / stores
    opcodeTable[0x32] = [this]() { return opLDAddrImm16FromA(); }; // LD (nn),A
    opcodeTable[0x3A] = [this]() { return opLDAFromAddrImm16(); }; // LD A,(nn)

    // 8-bit direct register-pair loads/stores
    opcodeTable[0x02] = [this]() { return opLDAddrBCFromA(); }; // LD (BC),A
    opcodeTable[0x0A] = [this]() { return opLDAFromAddrBC(); };  // LD A,(BC)
    opcodeTable[0x12] = [this]() { return opLDAddrDEFromA(); }; // LD (DE),A
    opcodeTable[0x1A] = [this]() { return opLDAFromAddrDE(); };  // LD A,(DE)

    // 16-bit loads
    opcodeTable[0x22] = [this]() { return opLDAddrImm16FromHL(); }; // LD (nn),HL
    opcodeTable[0x2A] = [this]() { return opLDHLFromImm16Address(); }; // LD HL,(nn)

    // Prefixes
    opcodeTable[0xCB] = [this]() { return executeCB(); };
    opcodeTable[0xDD] = [this]() { return executeDD(); };
    opcodeTable[0xED] = [this]() { return executeED(); };
    opcodeTable[0xFD] = [this]() { return executeFD(); };

    // Relative jumps
    opcodeTable[0x10] = [this]() { return opDJNZ(); }; // DJNZ e
    opcodeTable[0x18] = [this]() { return opJR(); };   // JR e
    opcodeTable[0x20] = [this]() { return opJRNZ(); }; // JR NZ,e
    opcodeTable[0x28] = [this]() { return opJRZ(); };  // JR Z,e
    opcodeTable[0x30] = [this]() { return opJRNC(); }; // JR NC,e
    opcodeTable[0x38] = [this]() { return opJRC(); };  // JR C,e

    // Rotate / accumulator
    opcodeTable[0x07] = [this]() { return opRLCA(); }; // RLCA
    opcodeTable[0x0F] = [this]() { return opRRCA(); }; // RRCA
    opcodeTable[0x1F] = [this]() { return opRRA(); }; // RRA
    opcodeTable[0x17] = [this]() { return opRLA(); }; // RLA
    opcodeTable[0x2F] = [this]() { return opCPL(); }; // CPL
    opcodeTable[0x37] = [this]() { return opSCF(); };  // SCF

    // RST
    opcodeTable[0xC7] = [this]() { return opRST(0x0000); }; // RST $00
    opcodeTable[0xCF] = [this]() { return opRST(0x0008); }; // RST $08
    opcodeTable[0xD7] = [this]() { return opRST(0x0010); }; // RST $10
    opcodeTable[0xDF] = [this]() { return opRST(0x0018); }; // RST $18
    opcodeTable[0xE7] = [this]() { return opRST(0x0020); }; // RST $20
    opcodeTable[0xEF] = [this]() { return opRST(0x0028); }; // RST $28
    opcodeTable[0xF7] = [this]() { return opRST(0x0030); }; // RST $30
    opcodeTable[0xFF] = [this]() { return opRST(0x0038); }; // RST $38
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

int CPU::opDI()
{
    IFF1 = false;
    IFF2 = false;
    eiDelay = false;

    return 4;
}

int CPU::opEI()
{
    // Interrupts become enabled after the next instruction.
    eiDelay = true;

    return 4;
}

int CPU::opRRD()
{
    const uint16_t address = getHL();

    const uint8_t memValue = read8(address);

    const uint8_t aHigh = static_cast<uint8_t>(A & 0xF0);
    const uint8_t aLow  = static_cast<uint8_t>(A & 0x0F);

    const uint8_t memHigh = static_cast<uint8_t>((memValue >> 4) & 0x0F);
    const uint8_t memLow  = static_cast<uint8_t>(memValue & 0x0F);

    const uint8_t newMem = static_cast<uint8_t>((aLow << 4) | memHigh);
    const uint8_t newA   = static_cast<uint8_t>(aHigh | memLow);

    write8(address, newMem);
    A = newA;

    const uint8_t oldCarry = F & FLAG_C;

    F = oldCarry;

    if (A & 0x80)
        F |= FLAG_S;

    if (A == 0)
        F |= FLAG_Z;

    if (parityEven(A))
        F |= FLAG_PV;

    F |= A & (FLAG_Y | FLAG_X);

    // H and N are reset by leaving them cleared.

    return 18;
}

int CPU::opRLD()
{
    const uint16_t address = getHL();

    const uint8_t memValue = read8(address);

    const uint8_t aHigh = static_cast<uint8_t>(A & 0xF0);
    const uint8_t aLow  = static_cast<uint8_t>(A & 0x0F);

    const uint8_t memHigh = static_cast<uint8_t>((memValue >> 4) & 0x0F);
    const uint8_t memLow  = static_cast<uint8_t>(memValue & 0x0F);

    const uint8_t newMem = static_cast<uint8_t>((memLow << 4) | aLow);
    const uint8_t newA   = static_cast<uint8_t>(aHigh | memHigh);

    write8(address, newMem);
    A = newA;

    const uint8_t oldCarry = F & FLAG_C;

    F = oldCarry;

    if (A & 0x80)
        F |= FLAG_S;

    if (A == 0)
        F |= FLAG_Z;

    if (parityEven(A))
        F |= FLAG_PV;

    F |= A & (FLAG_Y | FLAG_X);

    return 18;
}

int CPU::opANDImm()
{
    const uint8_t value = fetch8();

    andA(value);

    return 7;
}

int CPU::opORImm()
{
    const uint8_t value = fetch8();
    orA(value);
    return 7;
}

int CPU::opADDImm()
{
    const uint8_t value = fetch8();

    addA(value);

    return 7;
}

int CPU::opADCImm()
{
    const uint8_t value = fetch8();

    adcA(value);

    return 7;
}

int CPU::opSUBImm()
{
    const uint8_t value = fetch8();

    subA(value);

    return 7;
}

int CPU::opCALLNZImm16()
{
    const uint16_t address = fetch16();

    if ((F & FLAG_Z) == 0)
    {
        push16(PC);
        PC = address;
        return 17;
    }

    return 10;
}

int CPU::opCALLZImm16()
{
    const uint16_t address = fetch16();

    if (F & FLAG_Z)
    {
        push16(PC);
        PC = address;
        return 17;
    }

    return 10;
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

int CPU::opRETZ()
{
    if (F & FLAG_Z)
    {
        PC = pop16();
        return 11;
    }

    return 5;
}

int CPU::opRETNZ()
{
    if ((F & FLAG_Z) == 0)
    {
        PC = pop16();
        return 11;
    }

    return 5;
}

int CPU::opRETNC()
{
    if ((F & FLAG_C) == 0)
    {
        PC = pop16();
        return 11;
    }

    return 5;
}

int CPU::opRETC()
{
    if (F & FLAG_C)
    {
        PC = pop16();
        return 11;
    }

    return 5;
}

int CPU::opPUSHBC()
{
    push16(getBC());
    return 11;
}

int CPU::opPUSHDE()
{
    push16(getDE());
    return 11;
}

int CPU::opPUSHHL()
{
    push16(getHL());
    return 11;
}

int CPU::opPUSHAF()
{
    push16(static_cast<uint16_t>((A << 8) | F));
    return 11;
}

int CPU::opPOPBC()
{
    setBC(pop16());
    return 10;
}

int CPU::opPOPDE()
{
    setDE(pop16());
    return 10;
}

int CPU::opPOPHL()
{
    setHL(pop16());
    return 10;
}

int CPU::opPOPAF()
{
    const uint16_t value = pop16();

    F = static_cast<uint8_t>(value & 0x00FF);
    A = static_cast<uint8_t>(value >> 8);

    return 10;
}

int CPU::opCPImm()
{
    const uint8_t value = fetch8();
    compareA(value);
    return 7;
}

int CPU::opEXAFAFShadow()
{
    std::swap(A, A_);
    std::swap(F, F_);

    return 4;
}

int CPU::opEXX()
{
    std::swap(B, B_);
    std::swap(C, C_);

    std::swap(D, D_);
    std::swap(E, E_);

    std::swap(H, H_);
    std::swap(L, L_);

    return 4;
}

int CPU::opEXSPHL()
{
    const uint8_t memLo = read8(SP);
    const uint8_t memHi = read8(static_cast<uint16_t>(SP + 1));

    const uint8_t oldL = L;
    const uint8_t oldH = H;

    L = memLo;
    H = memHi;

    write8(SP, oldL);
    write8(static_cast<uint16_t>(SP + 1), oldH);

    return 19;
}

int CPU::opEXDEHL()
{
    const uint8_t oldD = D;
    const uint8_t oldE = E;

    D = H;
    E = L;

    H = oldD;
    L = oldE;

    return 4;
}

int CPU::opHALT()
{
    halted = true;
    return 4;
}

int CPU::opINCBC()
{
    setBC(static_cast<uint16_t>(getBC() + 1));
    return 6;
}

int CPU::opINCB()
{
    B = inc8(B);
    return 4;
}

int CPU::opINCC()
{
    C = inc8(C);
    return 4;
}

int CPU::opINCDE()
{
    setDE(static_cast<uint16_t>(getDE() + 1));
    return 6;
}

int CPU::opINCD()
{
    D = inc8(D);
    return 4;
}

int CPU::opINCE()
{
    E = inc8(E);
    return 4;
}

int CPU::opINCHL()
{
    setHL(static_cast<uint16_t>(getHL() + 1));
    return 6;
}

int CPU::opINCH()
{
    H = inc8(H);
    return 4;
}

int CPU::opINCL()
{
    L = inc8(L);
    return 4;
}

int CPU::opINCA()
{
    A = inc8(A);
    return 4;
}

int CPU::opINCSP()
{
    SP++;
    return 6;
}

int CPU::opDECA()
{
    A = dec8(A);
    return 4;
}

int CPU::opDECB()
{
    B = dec8(B);
    return 4;
}

int CPU::opDECC()
{
    C = dec8(C);
    return 4;
}

int CPU::opDECD()
{
    D = dec8(D);
    return 4;
}

int CPU::opDECE()
{
    E = dec8(E);
    return 4;
}

int CPU::opDECH()
{
    H = dec8(H);
    return 4;
}

int CPU::opDECL()
{
    L = dec8(L);
    return 4;
}

int CPU::opDECBC()
{
    setBC(static_cast<uint16_t>(getBC() - 1));
    return 6;
}

int CPU::opDECDE()
{
    setDE(static_cast<uint16_t>(getDE() - 1));
    return 6;
}

int CPU::opDECHL()
{
    setHL(static_cast<uint16_t>(getHL() - 1));
    return 6;
}

int CPU::opDECSP()
{
    SP--;
    return 6;
}

int CPU::opADDHLBC()
{
    addHL(getBC());
    return 11;
}

int CPU::opADDHLDE()
{
    addHL(getDE());
    return 11;
}

int CPU::opADDHLHL()
{
    addHL(getHL());
    return 11;
}

int CPU::opADDHLSP()
{
    addHL(SP);
    return 11;
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

int CPU::opLDHLAddrImm()
{
    const uint8_t value = fetch8();

    write8(getHL(), value);

    return 10;
}

int CPU::opLDAddrBCFromA()
{
    write8(getBC(), A);
    return 7;
}

int CPU::opLDAFromAddrBC()
{
    A = read8(getBC());
    return 7;
}

int CPU::opLDAddrDEFromA()
{
    write8(getDE(), A);
    return 7;
}

int CPU::opLDAFromAddrDE()
{
    A = read8(getDE());
    return 7;
}

int CPU::opLDAddrImm16FromA()
{
    const uint16_t address = fetch16();

    write8(address, A);

    return 13;
}

int CPU::opLDAFromAddrImm16()
{
    const uint16_t address = fetch16();

    A = read8(address);

    return 13;
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

int CPU::opJPPImm16()
{
    const uint16_t address = fetch16();

    if ((F & FLAG_S) == 0)
    {
        PC = address;
    }

    return 10;
}

int CPU::opJPHL()
{
    PC = getHL();
    return 4;
}

int CPU::opJPMImm16()
{
    const uint16_t address = fetch16();

    if (F & FLAG_S)
    {
        PC = address;
    }

    return 10;
}

int CPU::opJPZImm16()
{
    const uint16_t address = fetch16();

    if (F & FLAG_Z)
        PC = address;

    return 10;
}

int CPU::opJPNCImm16()
{
    const uint16_t address = fetch16();

    if ((F & FLAG_C) == 0)
        PC = address;

    return 10;
}

int CPU::opJPCImm16()
{
    const uint16_t address = fetch16();

    if (F & FLAG_C)
        PC = address;

    return 10;
}

int CPU::opOUTImmA()
{
    const uint8_t port = fetch8();

    writeIO(port, A);

    return 11;
}

int CPU::opINAImm()
{
    const uint8_t port = fetch8();

    A = readIO(port);

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

int CPU::opJRNZ()
{
    const int8_t offset = static_cast<int8_t>(fetch8());

    if ((F & FLAG_Z) == 0)
    {
        PC = static_cast<uint16_t>(PC + offset);
        return 12;
    }

    return 7;
}

int CPU::opJRZ()
{
    int8_t offset = static_cast<int8_t>(fetch8());

    if (F & FLAG_Z)
    {
        PC = static_cast<uint16_t>(PC + offset);
        return 12;
    }

    return 7;
}

int CPU::opDJNZ()
{
    const int8_t offset = static_cast<int8_t>(fetch8());

    B = static_cast<uint8_t>(B - 1);

    if (B != 0)
    {
        PC = static_cast<uint16_t>(PC + offset);
        return 13;
    }

    return 8;
}

int CPU::opJR()
{
    const int8_t offset = static_cast<int8_t>(fetch8());

    PC = static_cast<uint16_t>(PC + offset);

    return 12;
}

int CPU::opJRNC()
{
    const int8_t offset = static_cast<int8_t>(fetch8());

    if ((F & FLAG_C) == 0)
    {
        PC = static_cast<uint16_t>(PC + offset);
        return 12;
    }

    return 7;
}

int CPU::opJRC()
{
    const int8_t offset = static_cast<int8_t>(fetch8());

    if (F & FLAG_C)
    {
        PC = static_cast<uint16_t>(PC + offset);
        return 12;
    }

    return 7;
}

int CPU::opCPL()
{
    A = static_cast<uint8_t>(~A);

    // CPL sets H and N.
    F |= FLAG_H;
    F |= FLAG_N;

    // Undocumented flags copy from result bits 5 and 3.
    F &= static_cast<uint8_t>(~(FLAG_Y | FLAG_X));
    F |= A & (FLAG_Y | FLAG_X);

    return 4;
}

int CPU::opSCF()
{
    // SCF preserves S, Z, and P/V.
    // Sets C.
    // Clears H and N.
    F &= static_cast<uint8_t>(FLAG_S | FLAG_Z | FLAG_PV);

    // Undocumented flags copy from A bits 5 and 3.
    F |= A & (FLAG_Y | FLAG_X);

    F |= FLAG_C;

    return 4;
}

int CPU::opRRA()
{
    const bool oldCarry = (F & FLAG_C) != 0;
    const bool newCarry = (A & 0x01) != 0;

    A = static_cast<uint8_t>((A >> 1) | (oldCarry ? 0x80 : 0x00));

    // RRA preserves S, Z, and P/V. Clears H and N. Updates C.
    F &= static_cast<uint8_t>(FLAG_S | FLAG_Z | FLAG_PV);

    F |= A & (FLAG_Y | FLAG_X);

    if (newCarry)
        F |= FLAG_C;

    return 4;
}

int CPU::opRRCA()
{
    const bool carry = (A & 0x01) != 0;

    A = static_cast<uint8_t>((A >> 1) | (carry ? 0x80 : 0x00));

    // RRCA preserves S, Z, and P/V.
    // Clears H and N. Updates C.
    F &= static_cast<uint8_t>(FLAG_S | FLAG_Z | FLAG_PV);

    // Undocumented flags copy from new A.
    F |= A & (FLAG_Y | FLAG_X);

    if (carry)
        F |= FLAG_C;

    return 4;
}

int CPU::opRLA()
{
    const bool oldCarry = (F & FLAG_C) != 0;
    const bool newCarry = (A & 0x80) != 0;

    A = static_cast<uint8_t>((A << 1) | (oldCarry ? 0x01 : 0x00));

    // RLA preserves S, Z, and P/V.
    // Clears H and N. Updates C.
    F &= static_cast<uint8_t>(FLAG_S | FLAG_Z | FLAG_PV);

    F |= A & (FLAG_Y | FLAG_X);

    if (newCarry)
        F |= FLAG_C;

    return 4;
}

int CPU::opRLCA()
{
    const bool carry = (A & 0x80) != 0;

    A = static_cast<uint8_t>((A << 1) | (carry ? 0x01 : 0x00));

    // RLCA preserves S, Z, and P/V.
    // Clears H and N. Updates C.
    F &= static_cast<uint8_t>(FLAG_S | FLAG_Z | FLAG_PV);

    // Undocumented flags copy from new A.
    F |= A & (FLAG_Y | FLAG_X);

    if (carry)
        F |= FLAG_C;

    return 4;
}

int CPU::opRST(uint16_t address)
{
    push16(PC);
    PC = address;
    return 11;
}

int CPU::executeCB()
{
    const uint16_t prefixPC = static_cast<uint16_t>(PC - 1);
    const uint8_t opcode = fetch8();
    const uint16_t opcodePC = static_cast<uint16_t>(PC - 1);

    // CB 80-BF: RES b,r
    if (opcode >= 0x80 && opcode <= 0xBF)
    {
        const uint8_t bit = (opcode >> 3) & 0x07;
        const uint8_t reg = opcode & 0x07;

        uint8_t value = getReg8(reg);
        value &= static_cast<uint8_t>(~(1 << bit));
        setReg8(reg, value);

        return CB_CYCLE_COUNTS[opcode];
    }

    // CB C0-FF: SET b,r
    if (opcode >= 0xC0)
    {
        const uint8_t bit = (opcode >> 3) & 0x07;
        const uint8_t reg = opcode & 0x07;

        uint8_t value = getReg8(reg);
        value |= static_cast<uint8_t>(1 << bit);
        setReg8(reg, value);

        return CB_CYCLE_COUNTS[opcode];
    }

    // CB 00-3F: RLC/RRC/RL/RR/SLA/SRA/SLL/SRL r
    if (opcode <= 0x3F)
    {
        const uint8_t operation = (opcode >> 3) & 0x07;
        const uint8_t reg = opcode & 0x07;

        uint8_t value = 0;

        switch (reg)
        {
            case 0: value = B; break;
            case 1: value = C; break;
            case 2: value = D; break;
            case 3: value = E; break;
            case 4: value = H; break;
            case 5: value = L; break;
            case 6: value = read8(getHL()); break;
            case 7: value = A; break;
        }

        uint8_t result = value;
        bool carry = false;

        switch (operation)
        {
            case 0: // RLC r
                carry = (value & 0x80) != 0;
                result = static_cast<uint8_t>((value << 1) | (carry ? 1 : 0));
                break;

            case 1: // RRC r
                carry = (value & 0x01) != 0;
                result = static_cast<uint8_t>((value >> 1) | (carry ? 0x80 : 0));
                break;

            case 2: // RL r
            {
                const bool oldCarry = (F & FLAG_C) != 0;
                carry = (value & 0x80) != 0;
                result = static_cast<uint8_t>((value << 1) | (oldCarry ? 1 : 0));
                break;
            }

            case 3: // RR r
            {
                const bool oldCarry = (F & FLAG_C) != 0;
                carry = (value & 0x01) != 0;
                result = static_cast<uint8_t>((value >> 1) | (oldCarry ? 0x80 : 0));
                break;
            }

            case 4: // SLA r
                carry = (value & 0x80) != 0;
                result = static_cast<uint8_t>(value << 1);
                break;

            case 5: // SRA r
                carry = (value & 0x01) != 0;
                result = static_cast<uint8_t>((value >> 1) | (value & 0x80));
                break;

            case 6: // SLL/SLS r - undocumented, useful to support
                carry = (value & 0x80) != 0;
                result = static_cast<uint8_t>((value << 1) | 0x01);
                break;

            case 7: // SRL r
                carry = (value & 0x01) != 0;
                result = static_cast<uint8_t>(value >> 1);
                break;
        }

        switch (reg)
        {
            case 0: B = result; break;
            case 1: C = result; break;
            case 2: D = result; break;
            case 3: E = result; break;
            case 4: H = result; break;
            case 5: L = result; break;
            case 6: write8(getHL(), result); break;
            case 7: A = result; break;
        }

        F = 0;

        if (result & 0x80) F |= FLAG_S;
        if (result == 0)   F |= FLAG_Z;
        if (result & 0x20) F |= FLAG_Y;
        if (result & 0x08) F |= FLAG_X;
        if (parityEven(result)) F |= FLAG_PV;
        if (carry) F |= FLAG_C;

        return CB_CYCLE_COUNTS[opcode];
    }

    // CB 40-7F: BIT b,r
    if (opcode >= 0x40 && opcode <= 0x7F)
    {
        const uint8_t bit = (opcode >> 3) & 0x07;
        const uint8_t reg = opcode & 0x07;

        uint8_t value = 0;

        switch (reg)
        {
            case 0: value = B; break;
            case 1: value = C; break;
            case 2: value = D; break;
            case 3: value = E; break;
            case 4: value = H; break;
            case 5: value = L; break;
            case 6: value = read8(getHL()); break;
            case 7: value = A; break;
        }

        const bool bitSet = (value & (1 << bit)) != 0;
        const uint8_t oldCarry = F & FLAG_C;

        F = oldCarry;

        if (value & 0x20) F |= FLAG_Y;
        if (value & 0x08) F |= FLAG_X;

        F |= FLAG_H;

        if (!bitSet)
        {
            F |= FLAG_Z;
            F |= FLAG_PV;
        }

        if (bit == 7 && bitSet)
            F |= FLAG_S;

        return CB_CYCLE_COUNTS[opcode];
    }

    std::cerr
        << "Unimplemented Z80 CB opcode $"
        << std::hex << std::uppercase
        << std::setw(2) << std::setfill('0')
        << static_cast<int>(opcode)
        << " at PC=$"
        << std::setw(4) << opcodePC
        << " prefix PC=$"
        << std::setw(4) << prefixPC
        << std::dec << std::nouppercase
        << std::endl;

    halted = true;
    return 4;
}

int CPU::executeDD()
{
    const uint16_t prefixPC = static_cast<uint16_t>(PC - 1);
    const uint8_t opcode = fetch8();
    const uint16_t opcodePC = static_cast<uint16_t>(PC - 1);

    switch (opcode)
    {
        case 0x09: // ADD IX,BC
        {
            IX = add16Index(IX, getBC());
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x19: // ADD IX,DE
        {
            IX = add16Index(IX, getDE());
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x21: // LD IX,nn
        {
            IX = fetch16();
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x22: // LD (nn),IX
        {
            const uint16_t address = fetch16();

            write8(address, static_cast<uint8_t>(IX & 0x00FF));
            write8(static_cast<uint16_t>(address + 1), static_cast<uint8_t>(IX >> 8));

            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x23: // INC IX
        {
            IX++;
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x2A: // LD IX,(nn)
        {
            const uint16_t address = fetch16();

            const uint8_t lo = read8(address);
            const uint8_t hi = read8(static_cast<uint16_t>(address + 1));

            IX = static_cast<uint16_t>(lo | (hi << 8));

            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x2B: // DEC IX
        {
            IX--;
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x34: // INC (IX+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            const uint16_t address = static_cast<uint16_t>(IX + d);

            const uint8_t value = read8(address);
            const uint8_t result = inc8(value);

            write8(address, result);

            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x35: // DEC (IX+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            const uint16_t address = static_cast<uint16_t>(IX + d);

            const uint8_t value = read8(address);
            const uint8_t result = dec8(value);

            write8(address, result);

            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x36: // LD (IX+d),n
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            const uint8_t value = fetch8();

            write8(static_cast<uint16_t>(IX + d), value);

            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x46: // LD B,(IX+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            B = read8(static_cast<uint16_t>(IX + d));
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x4E: // LD C,(IX+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            C = read8(static_cast<uint16_t>(IX + d));
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x56: // LD D,(IX+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            D = read8(static_cast<uint16_t>(IX + d));
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x5E: // LD E,(IX+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            E = read8(static_cast<uint16_t>(IX + d));
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x66: // LD H,(IX+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            H = read8(static_cast<uint16_t>(IX + d));
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x6E: // LD L,(IX+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            L = read8(static_cast<uint16_t>(IX + d));
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x70: // LD (IX+d),B
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            write8(static_cast<uint16_t>(IX + d), B);
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x71: // LD (IX+d),C
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            write8(static_cast<uint16_t>(IX + d), C);
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x72: // LD (IX+d),D
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            write8(static_cast<uint16_t>(IX + d), D);
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x73: // LD (IX+d),E
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            write8(static_cast<uint16_t>(IX + d), E);
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x74: // LD (IX+d),H
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            write8(static_cast<uint16_t>(IX + d), H);
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x75: // LD (IX+d),L
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            write8(static_cast<uint16_t>(IX + d), L);
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x77: // LD (IX+d),A
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            write8(static_cast<uint16_t>(IX + d), A);
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x7E: // LD A,(IX+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            A = read8(static_cast<uint16_t>(IX + d));
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x86: // ADD A,(IX+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            addA(read8(static_cast<uint16_t>(IX + d)));
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x8E: // ADC A,(IX+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            adcA(read8(static_cast<uint16_t>(IX + d)));
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0x9E: // SBC A,(IX+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            sbcA(read8(static_cast<uint16_t>(IX + d)));
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0xB6: // OR (IX+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            orA(read8(static_cast<uint16_t>(IX + d)));
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0xBE: // CP (IX+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            compareA(read8(static_cast<uint16_t>(IX + d)));
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0xE1: // POP IX
        {
            IX = pop16();
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0xE5: // PUSH IX
        {
            push16(IX);
            return DD_CYCLE_COUNTS[opcode];
        }

        case 0xE9: // JP (IX)
        {
            PC = IX;
            return DD_CYCLE_COUNTS[opcode];
        }

        default:
            std::cerr
                << "Unimplemented Z80 DD opcode $"
                << std::hex << std::uppercase
                << std::setw(2) << std::setfill('0')
                << static_cast<int>(opcode)
                << " at PC=$"
                << std::setw(4) << opcodePC
                << " prefix PC=$"
                << std::setw(4) << prefixPC
                << std::dec << std::nouppercase
                << std::endl;

            halted = true;
            return 4;
    }
}

int CPU::executeED()
{
    const uint16_t prefixPC = static_cast<uint16_t>(PC - 1);
    const uint8_t opcode = fetch8();
    const uint16_t opcodePC = static_cast<uint16_t>(PC - 1);

    switch (opcode)
    {
        case 0x42: // SBC HL,BC
        {
            const uint32_t carry = (F & FLAG_C) ? 1 : 0;
            const uint16_t oldHL = getHL();
            const uint16_t value = getBC();

            const uint32_t result = static_cast<uint32_t>(oldHL) - value - carry;
            const uint16_t result16 = static_cast<uint16_t>(result);

            F = 0;

            if (result16 & 0x8000)
                F |= FLAG_S;

            if (result16 == 0)
                F |= FLAG_Z;

            if ((oldHL & 0x0FFF) < ((value & 0x0FFF) + carry))
                F |= FLAG_H;

            if (((oldHL ^ value) & (oldHL ^ result16) & 0x8000) != 0)
                F |= FLAG_PV;

            F |= FLAG_N;

            if (result & 0x10000)
                F |= FLAG_C;

            F |= static_cast<uint8_t>((result16 >> 8) & (FLAG_Y | FLAG_X));

            setHL(result16);

            return ED_CYCLE_COUNTS[opcode];
        }

        case 0x43: // LD (nn),BC
        {
            const uint16_t address = fetch16();
            write8(address, C);
            write8(static_cast<uint16_t>(address + 1), B);
            return ED_CYCLE_COUNTS[opcode];
        }

        case 0x44: // NEG
        {
            const uint8_t oldA = A;
            const uint8_t result = static_cast<uint8_t>(0 - oldA);

            A = result;

            F = 0;

            if (result & 0x80)
                F |= FLAG_S;

            if (result == 0)
                F |= FLAG_Z;

            if ((oldA & 0x0F) != 0)
                F |= FLAG_H;

            if (oldA == 0x80)
                F |= FLAG_PV;

            F |= FLAG_N;

            if (oldA != 0)
                F |= FLAG_C;

            F |= result & (FLAG_Y | FLAG_X);

            return ED_CYCLE_COUNTS[opcode];
        }

        case 0x46: // IM 0
        {
            IM = 0;
            return ED_CYCLE_COUNTS[opcode];
        }

        case 0x4B: // LD BC,(nn)
        {
            const uint16_t address = fetch16();

            const uint8_t lo = read8(address);
            const uint8_t hi = read8(static_cast<uint16_t>(address + 1));

            setBC(static_cast<uint16_t>(lo | (hi << 8)));

            return ED_CYCLE_COUNTS[opcode];
        }

        case 0x52: // SBC HL,DE
        {
            const uint32_t carry = (F & FLAG_C) ? 1 : 0;
            const uint16_t oldHL = getHL();
            const uint16_t value = getDE();

            const uint32_t result = static_cast<uint32_t>(oldHL) - value - carry;
            const uint16_t result16 = static_cast<uint16_t>(result);

            F = 0;

            if (result16 & 0x8000)
                F |= FLAG_S;

            if (result16 == 0)
                F |= FLAG_Z;

            if ((oldHL & 0x0FFF) < ((value & 0x0FFF) + carry))
                F |= FLAG_H;

            if (((oldHL ^ value) & (oldHL ^ result16) & 0x8000) != 0)
                F |= FLAG_PV;

            F |= FLAG_N;

            if (result & 0x10000)
                F |= FLAG_C;

            F |= static_cast<uint8_t>((result16 >> 8) & (FLAG_Y | FLAG_X));

            setHL(result16);

            return ED_CYCLE_COUNTS[opcode];
        }

        case 0x53: // LD (nn),DE
        {
            const uint16_t address = fetch16();
            write8(address, E);
            write8(static_cast<uint16_t>(address + 1), D);
            return ED_CYCLE_COUNTS[opcode];
        }

        case 0x56: // IM 1
        {
            IM = 1;
            return ED_CYCLE_COUNTS[opcode]; // 8
        }

        case 0x5B: // LD DE,(nn)
        {
            const uint16_t address = fetch16();

            const uint8_t lo = read8(address);
            const uint8_t hi = read8(static_cast<uint16_t>(address + 1));

            setDE(static_cast<uint16_t>(lo | (hi << 8)));

            return ED_CYCLE_COUNTS[opcode];
        }

        case 0x5E: // IM 2
        {
            IM = 2;
            return ED_CYCLE_COUNTS[opcode];
        }

        case 0x5F: // LD A,R
        {
            A = R;

            const uint8_t oldCarry = F & FLAG_C;

            F = oldCarry;

            if (A & 0x80)
                F |= FLAG_S;

            if (A == 0)
                F |= FLAG_Z;

            if (IFF2)
                F |= FLAG_PV;

            F |= A & (FLAG_Y | FLAG_X);

            return ED_CYCLE_COUNTS[opcode];
        }

        case 0x63: // LD (nn),HL
        {
            const uint16_t address = fetch16();
            write8(address, L);
            write8(static_cast<uint16_t>(address + 1), H);
            return ED_CYCLE_COUNTS[opcode];
        }

        case 0x67: // RRD
        {
            return opRRD();
        }

        case 0x6B: // LD HL,(nn)
        {
            const uint16_t address = fetch16();

            const uint8_t lo = read8(address);
            const uint8_t hi = read8(static_cast<uint16_t>(address + 1));

            setHL(static_cast<uint16_t>(lo | (hi << 8)));

            return ED_CYCLE_COUNTS[opcode];
        }

        case 0x6F: // RLD
        {
            return opRLD();
        }

        case 0x73: // LD (nn),SP
        {
            const uint16_t address = fetch16();

            write8(address, static_cast<uint8_t>(SP & 0x00FF));
            write8(static_cast<uint16_t>(address + 1), static_cast<uint8_t>(SP >> 8));

            return ED_CYCLE_COUNTS[opcode];
        }

        case 0x7B: // LD SP,(nn)
        {
            const uint16_t address = fetch16();

            const uint8_t lo = read8(address);
            const uint8_t hi = read8(static_cast<uint16_t>(address + 1));

            SP = static_cast<uint16_t>(lo | (hi << 8));

            return ED_CYCLE_COUNTS[opcode];
        }

        case 0xA2: // INI
        {
            const uint16_t hl = getHL();

            // IN (C)
            const uint8_t value = readIO(C);

            // (HL) = input value
            write8(hl, value);

            // HL++
            setHL(static_cast<uint16_t>(hl + 1));

            // B--
            B = static_cast<uint8_t>(B - 1);

            const uint8_t oldCarry = F & FLAG_C;

            F = oldCarry;

            if (B == 0)
                F |= FLAG_Z;

            if (value & 0x80)
                F |= FLAG_N;

            // Useful approximation for undocumented flags.f
            if (B & 0x80) F |= FLAG_S;
            if (B & 0x20) F |= FLAG_Y;
            if (B & 0x08) F |= FLAG_X;

            return ED_CYCLE_COUNTS[opcode]; // 16
        }

        case 0xA3: // OUTI
        {
            const uint16_t hl = getHL();
            const uint8_t value = read8(hl);

            writeIO(C, value);

            setHL(static_cast<uint16_t>(hl + 1));
            B = static_cast<uint8_t>(B - 1);

            const uint8_t oldCarry = F & FLAG_C;

            F = oldCarry;

            if (B == 0)
                F |= FLAG_Z;

            if (value & 0x80)
                F |= FLAG_N;

            if (B & 0x80) F |= FLAG_S;
            if (B & 0x20) F |= FLAG_Y;
            if (B & 0x08) F |= FLAG_X;

            return ED_CYCLE_COUNTS[opcode];
        }

        case 0xB0: // LDIR
        {
            const uint16_t hl = getHL();
            const uint16_t de = getDE();
            const uint16_t bc = getBC();

            const uint8_t value = read8(hl);

            write8(de, value);

            setHL(static_cast<uint16_t>(hl + 1));
            setDE(static_cast<uint16_t>(de + 1));
            setBC(static_cast<uint16_t>(bc - 1));

            const uint16_t newBC = static_cast<uint16_t>(bc - 1);
            const uint8_t oldCarry = F & FLAG_C;
            const uint8_t sum = static_cast<uint8_t>(A + value);

            F = oldCarry;

            if (sum & 0x08)
                F |= FLAG_X;

            if (sum & 0x20)
                F |= FLAG_Y;

            if (newBC != 0)
            {
                F |= FLAG_PV;
                PC = static_cast<uint16_t>(PC - 2);
                return 21;
            }

            return 16;
        }

        case 0xB8: // LDDR
        {
            const uint16_t hl = getHL();
            const uint16_t de = getDE();
            const uint16_t bc = getBC();

            const uint8_t value = read8(hl);

            write8(de, value);

            setHL(static_cast<uint16_t>(hl - 1));
            setDE(static_cast<uint16_t>(de - 1));
            setBC(static_cast<uint16_t>(bc - 1));

            const uint16_t newBC = static_cast<uint16_t>(bc - 1);
            const uint8_t oldCarry = F & FLAG_C;
            const uint8_t sum = static_cast<uint8_t>(A + value);

            F = oldCarry;

            // H and N are reset for LDDR.
            // P/V set while BC is not zero.
            if (sum & 0x08)
                F |= FLAG_X;

            if (sum & 0x20)
                F |= FLAG_Y;

            if (newBC != 0)
            {
                F |= FLAG_PV;

                // Repeat this ED B8 instruction.
                PC = static_cast<uint16_t>(PC - 2);

                return 21;
            }

            return 16;
        }

        default:
        {
            std::cerr
                << "Unimplemented Z80 ED opcode $"
                << std::hex << std::uppercase
                << std::setw(2) << std::setfill('0')
                << static_cast<int>(opcode)
                << " at PC=$"
                << std::setw(4) << opcodePC
                << " prefix PC=$"
                << std::setw(4) << prefixPC
                << std::dec << std::nouppercase
                << std::endl;

            halted = true;
            return 4;
        }
    }
}

int CPU::executeFD()
{
    const uint16_t prefixPC = static_cast<uint16_t>(PC - 1);
    const uint8_t opcode = fetch8();
    const uint16_t opcodePC = static_cast<uint16_t>(PC - 1);

    switch (opcode)
    {
        case 0x09: // ADD IY,BC
        {
            IY = add16Index(IY, getBC());
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x19: // ADD IY,DE
        {
            IY = add16Index(IY, getDE());
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x21: // LD IY,nn
        {
            IY = fetch16();
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x22: // LD (nn),IY
        {
            const uint16_t address = fetch16();

            write8(address, static_cast<uint8_t>(IY & 0x00FF));
            write8(static_cast<uint16_t>(address + 1), static_cast<uint8_t>(IY >> 8));

            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x23: // INC IY
        {
            IY++;
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x2A: // LD IY,(nn)
        {
            const uint16_t address = fetch16();

            const uint8_t lo = read8(address);
            const uint8_t hi = read8(static_cast<uint16_t>(address + 1));

            IY = static_cast<uint16_t>(lo | (hi << 8));

            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x2B: // DEC IY
        {
            IY--;
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x34: // INC (IY+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            const uint16_t address = static_cast<uint16_t>(IY + d);

            const uint8_t value = read8(address);
            const uint8_t result = inc8(value);

            write8(address, result);

            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x35: // DEC (IY+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            const uint16_t address = static_cast<uint16_t>(IY + d);

            const uint8_t value = read8(address);
            const uint8_t result = dec8(value);

            write8(address, result);

            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x36: // LD (IY+d),n
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            const uint8_t value = fetch8();

            write8(static_cast<uint16_t>(IY + d), value);

            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x46: // LD B,(IY+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            B = read8(static_cast<uint16_t>(IY + d));
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x4E: // LD C,(IY+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            C = read8(static_cast<uint16_t>(IY + d));
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x56: // LD D,(IY+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            D = read8(static_cast<uint16_t>(IY + d));
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x5E: // LD E,(IY+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            E = read8(static_cast<uint16_t>(IY + d));
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x66: // LD H,(IY+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            H = read8(static_cast<uint16_t>(IY + d));
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x6E: // LD L,(IY+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            L = read8(static_cast<uint16_t>(IY + d));
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x70: // LD (IY+d),B
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            write8(static_cast<uint16_t>(IY + d), B);
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x71: // LD (IY+d),C
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            write8(static_cast<uint16_t>(IY + d), C);
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x72: // LD (IY+d),D
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            write8(static_cast<uint16_t>(IY + d), D);
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x73: // LD (IY+d),E
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            write8(static_cast<uint16_t>(IY + d), E);
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x74: // LD (IY+d),H
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            write8(static_cast<uint16_t>(IY + d), H);
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x75: // LD (IY+d),L
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            write8(static_cast<uint16_t>(IY + d), L);
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x77: // LD (IY+d),A
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            write8(static_cast<uint16_t>(IY + d), A);
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x7E: // LD A,(IY+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            A = read8(static_cast<uint16_t>(IY + d));
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x86: // ADD A,(IY+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            addA(read8(static_cast<uint16_t>(IY + d)));
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x8E: // ADC A,(IY+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            adcA(read8(static_cast<uint16_t>(IY + d)));
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0x9E: // SBC A,(IY+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            sbcA(read8(static_cast<uint16_t>(IY + d)));
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0xB6: // OR (IY+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            orA(read8(static_cast<uint16_t>(IY + d)));
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0xBE: // CP (IY+d)
        {
            const int8_t d = static_cast<int8_t>(fetch8());
            compareA(read8(static_cast<uint16_t>(IY + d)));
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0xE1: // POP IY
        {
            IY = pop16();
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0xE5: // PUSH IY
        {
            push16(IY);
            return FD_CYCLE_COUNTS[opcode];
        }

        case 0xE9: // JP (IY)
        {
            PC = IY;
            return FD_CYCLE_COUNTS[opcode];
        }

        default:
            std::cerr
                << "Unimplemented Z80 FD opcode $"
                << std::hex << std::uppercase
                << std::setw(2) << std::setfill('0')
                << static_cast<int>(opcode)
                << " at PC=$"
                << std::setw(4) << opcodePC
                << " prefix PC=$"
                << std::setw(4) << prefixPC
                << std::dec << std::nouppercase
                << std::endl;

            halted = true;
            return 4;
    }
}

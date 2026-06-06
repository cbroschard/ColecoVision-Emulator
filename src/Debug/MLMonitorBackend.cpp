// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Bus.h"
#include "Cartridge.h"
#include "ColecoVisionSystem.h"
#include "Controller.h"
#include "CPU.h"
#include "Debug/MLMonitorBackend.h"
#include "Memory.h"
#include "PSG.h"

MLMonitorBackend::MLMonitorBackend() :
    bus(nullptr),
    cartridge(nullptr),
    host(nullptr),
    controller1(nullptr),
    controller2(nullptr),
    cpu(nullptr),
    memory(nullptr),
    psg(nullptr),
    vdp(nullptr)
{

}

MLMonitorBackend::~MLMonitorBackend() = default;

void MLMonitorBackend::enterMonitor()
{
    if (host)
        host->enterMonitor();
}

CartridgeInfo MLMonitorBackend::getCartridgeInfo() const
{
    if (cartridge == nullptr)
        return CartridgeInfo{};

    return cartridge->getInfo();
}

CPU& MLMonitorBackend::getCPU()
{
    return *cpu;
}

const CPU& MLMonitorBackend::getCPU() const
{
    return *cpu;
}

uint16_t MLMonitorBackend::getPC() const
{
    return cpu->getPC();
}

void MLMonitorBackend::setPC(uint16_t address)
{
    cpu->setPC(address);
}

Z80DisassembledInstruction MLMonitorBackend::disassembleAt(uint16_t address) const
{
    Z80Disassembler disassembler(
        [this](uint16_t addr) -> uint8_t
        {
            return debugRead8(addr);
        });

    return disassembler.disassemble(address);
}

int MLMonitorBackend::stepInstruction()
{
    if (cpu == nullptr)
        return 0;

    return cpu->debugStepInstruction();
}

void MLMonitorBackend::printCPUState() const
{
    if (!cpu)
    {
        std::cout << "CPU is not attached." << std::endl;
        return;
    }

    const Z80CPUState state = cpu->getState();

    const uint16_t AF  = make16(state.A,  state.F);
    const uint16_t BC  = make16(state.B,  state.C);
    const uint16_t DE  = make16(state.D,  state.E);
    const uint16_t HL  = make16(state.H,  state.L);

    const uint16_t AF_ = make16(state.A_, state.F_);
    const uint16_t BC_ = make16(state.B_, state.C_);
    const uint16_t DE_ = make16(state.D_, state.E_);
    const uint16_t HL_ = make16(state.H_, state.L_);

    std::cout
        << std::uppercase << std::hex << std::setfill('0')
        << "AF=$" << std::setw(4) << AF
        << "  BC=$" << std::setw(4) << BC
        << "  DE=$" << std::setw(4) << DE
        << "  HL=$" << std::setw(4) << HL << "\n"

        << "IX=$" << std::setw(4) << state.IX
        << "  IY=$" << std::setw(4) << state.IY
        << "  SP=$" << std::setw(4) << state.SP
        << "  PC=$" << std::setw(4) << state.PC << "\n"

        << "AF'=$" << std::setw(4) << AF_
        << " BC'=$" << std::setw(4) << BC_
        << " DE'=$" << std::setw(4) << DE_
        << " HL'=$" << std::setw(4) << HL_ << "\n"

        << "I=$" << std::setw(2) << static_cast<int>(state.I)
        << "  R=$" << std::setw(2) << static_cast<int>(state.R)
        << std::dec
        << "  IM=" << static_cast<int>(state.IM)
        << "  IFF1=" << (state.IFF1 ? 1 : 0)
        << "  IFF2=" << (state.IFF2 ? 1 : 0)
        << "  HALT=" << (state.halted ? 1 : 0) << "\n"

        << "Flags: "
        << "S="   << ((state.F & 0x80) ? 1 : 0)
        << " Z="  << ((state.F & 0x40) ? 1 : 0)
        << " H="  << ((state.F & 0x10) ? 1 : 0)
        << " P/V=" << ((state.F & 0x04) ? 1 : 0)
        << " N="  << ((state.F & 0x02) ? 1 : 0)
        << " C="  << ((state.F & 0x01) ? 1 : 0) << "\n"

        << "Cycles=" << state.cycles << "\n"

        << std::setfill(' ')
        << std::nouppercase;
}

void MLMonitorBackend::printCPUIRQState() const
{
    if (!cpu)
    {
        std::cout << "CPU is not attached." << std::endl;
        return;
    }

    const Z80CPUState state = cpu->getState();

    bool canAcceptIRQ =
        state.irqPending &&
        state.IFF1 &&
        state.eiDelay == 0 &&
        !state.halted;

    // HALT does not prevent IRQ acceptance; IRQ wakes HALT.
    // So if you want this to match the CPU behavior exactly, remove !state.halted.
    canAcceptIRQ =
        state.irqPending &&
        state.IFF1 &&
        state.eiDelay == 0;

    std::string irqTarget;

    switch (state.IM)
    {
        case 0:
            irqTarget = "$0038  (IM 0 treated as IM 1)";
            break;

        case 1:
            irqTarget = "$0038";
            break;

        case 2:
        {
            const uint16_t vectorAddress =
                static_cast<uint16_t>((static_cast<uint16_t>(state.I) << 8) | 0x00FF);

            std::ostringstream oss;
            oss << "$" << std::uppercase << std::hex << std::setw(4) << std::setfill('0')
                << vectorAddress
                << "  (vector table address)";
            irqTarget = oss.str();
            break;
        }

        default:
            irqTarget = "$0038  (invalid IM, emulator fallback)";
            break;
    }

    std::cout
        << "Interrupt State:" << "\n"
        << "  IM:" << "           " << static_cast<int>(state.IM) << "\n"
        << "  IFF1:" << "         " << (state.IFF1 ? "enabled" : "disabled") << "\n"
        << "  IFF2:" << "         " << (state.IFF2 ? "enabled" : "disabled") << "\n"
        << "  EI delay:" << "     " << static_cast<int>(state.eiDelay) << "\n"
        << "  HALT:" << "         " << (state.halted ? "yes" : "no") << "\n"
        << "\n"
        << "Pending:\n"
        << "  IRQ:          " << (state.irqPending ? "yes" : "no") << "\n"
        << "  NMI:          " << (state.nmiPending ? "yes" : "no") << "\n"
        << "\n"
        << "Effective Targets:\n"
        << "  IRQ:          " << irqTarget << "\n"
        << "  NMI:          $0066\n"
        << "\n"
        << "Accept IRQ now: " << (canAcceptIRQ ? "yes" : "no") << "\n";

    if (!canAcceptIRQ)
    {
        std::cout << "Reason:         ";

        if (!state.irqPending)
            std::cout << "IRQ not pending";
        else if (!state.IFF1)
            std::cout << "IFF1 disabled";
        else if (state.eiDelay != 0)
            std::cout << "EI delay active";
        else
            std::cout << "unknown";

        std::cout << "\n";
    }
}

void MLMonitorBackend::printCPUCycles() const
{
    if (!cpu)
    {
        std::cout << "CPU is not attached." << std::endl;
        return;
    }

    const Z80CPUState state = cpu->getState();

    std::cout
        << "CPU Cycles:\n"
        << "  Total:        " << state.cycles << "\n";
}

void MLMonitorBackend::printCPUFlags() const
{
    if (!cpu)
    {
        std::cout << "CPU is not attached." << std::endl;
        return;
    }

    const Z80CPUState state = cpu->getState();
    const uint8_t f = state.F;

    std::cout
        << "CPU Flags:\n"
        << "  F:            $"
        << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(f)
        << std::dec << std::setfill(' ')
        << "  ";

    for (int bit = 7; bit >= 0; --bit)
    {
        std::cout << ((f & (1 << bit)) ? '1' : '0');
    }

    std::cout
        << "\n\n"
        << "  S Sign:       " << ((f & 0x80) ? 1 : 0) << "\n"
        << "  Z Zero:       " << ((f & 0x40) ? 1 : 0) << "\n"
        << "  Y Bit 5:      " << ((f & 0x20) ? 1 : 0) << "\n"
        << "  H Half Carry: " << ((f & 0x10) ? 1 : 0) << "\n"
        << "  X Bit 3:      " << ((f & 0x08) ? 1 : 0) << "\n"
        << "  P/V Parity:   " << ((f & 0x04) ? 1 : 0) << "\n"
        << "  N Subtract:   " << ((f & 0x02) ? 1 : 0) << "\n"
        << "  C Carry:      " << ((f & 0x01) ? 1 : 0) << "\n"
        << std::nouppercase;
}

void MLMonitorBackend::printCPUStack(int count) const
{
    if (!cpu)
    {
        std::cout << "CPU is not attached." << std::endl;
        return;
    }

    if (count <= 0)
        count = 8;

    const Z80CPUState state = cpu->getState();
    uint16_t address = state.SP;

    std::cout
        << "CPU Stack:\n"
        << std::uppercase << std::hex << std::setfill('0')
        << "  SP=$" << std::setw(4) << state.SP << "\n\n";

    for (int i = 0; i < count; ++i)
    {
        const uint8_t lo = cpu->debugRead8(address);
        const uint8_t hi = cpu->debugRead8(static_cast<uint16_t>(address + 1));

        const uint16_t word =
            static_cast<uint16_t>(lo | (static_cast<uint16_t>(hi) << 8));

        std::cout
            << "  $" << std::setw(4) << address
            << ": " << std::setw(2) << static_cast<int>(lo)
            << " "  << std::setw(2) << static_cast<int>(hi)
            << "  word=$" << std::setw(4) << word;

        if (address == state.SP)
            std::cout << "  <- SP";

        std::cout << "\n";

        address = static_cast<uint16_t>(address + 2);
    }

    std::cout
        << std::dec
        << std::setfill(' ')
        << std::nouppercase;
}

uint8_t MLMonitorBackend::debugRead8(uint16_t address) const
{
    return cpu->debugRead8(address);
}

void MLMonitorBackend::debugWrite8(uint16_t address, uint8_t value)
{
    cpu->debugWrite8(address, value);
}

uint8_t MLMonitorBackend::readRAM(uint16_t address) const
{
    return memory->read(address);
}

uint8_t MLMonitorBackend::getVDPRegister(uint8_t index) const
{
    if (!vdp)
        return 0xFF;

    return vdp->getRegister(index);
}

VDPMode MLMonitorBackend::getVDPMode() const
{
    return vdp->getMode();
}

VDPStatusSnapshot MLMonitorBackend::getVDPStatusSnapshot() const
{
    if (!vdp)
        return {};

    return vdp->getStatusSnapshot();
}

uint8_t MLMonitorBackend::peekVDPVRAM(uint16_t address) const
{
    if (!vdp)
        return 0xFF;

    return vdp->peekVRAM(address);
}

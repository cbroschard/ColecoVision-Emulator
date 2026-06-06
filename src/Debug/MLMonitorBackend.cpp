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
    if (cpu == nullptr)
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
        << "AF=$" << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << AF
        << " BC=$" << std::setw(4) << BC
        << " DE=$" << std::setw(4) << DE
        << " HL=$" << std::setw(4) << HL
        << " IX=$" << std::setw(4) << state.IX
        << " IY=$" << std::setw(4) << state.IY
        << " SP=$" << std::setw(4) << state.SP
        << " PC=$" << std::setw(4) << state.PC
        << std::dec
        << " Cycles=" << state.cycles
        << std::endl;

    std::cout
        << "AF'=$" << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << AF_
        << " BC'=$" << std::setw(4) << BC_
        << " DE'=$" << std::setw(4) << DE_
        << " HL'=$" << std::setw(4) << HL_
        << " I=$" << std::setw(2) << static_cast<int>(state.I)
        << " R=$" << std::setw(2) << static_cast<int>(state.R)
        << std::dec
        << " IFF1=" << (state.IFF1 ? 1 : 0)
        << " IFF2=" << (state.IFF2 ? 1 : 0)
        << " IM=" << static_cast<int>(state.IM)
        << " HALT=" << (state.halted ? 1 : 0)
        << std::endl;
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

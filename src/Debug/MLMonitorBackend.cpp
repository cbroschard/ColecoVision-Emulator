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

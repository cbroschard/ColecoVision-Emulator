// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef MLMONITORBACKEND_H
#define MLMONITORBACKEND_H

#include <cstdint>
#include <string>
#include "Bus.h"
#include "Cartridge.h"
#include "Common/CommandUtils.h"
#include "VDP.h"
#include "Z80/Z80Disassembler.h"

// Forward declarations
class ColecoVisionSystem;
class Controller;
class CPU;
class Memory;
class PSG;
class VDP;

class MLMonitorBackend
{
    public:
        MLMonitorBackend();
        virtual ~MLMonitorBackend();

        inline void attachBusInstance(Bus* bus) { this->bus = bus; }
        inline void attachCartridgeInstance(Cartridge* cartridge) { this->cartridge = cartridge; }
        inline void attachController1Instance(Controller* controller1) { this->controller1 = controller1; }
        inline void attachController2Instance(Controller* controller2) { this->controller2 = controller2; }
        inline void attachCPUInstance(CPU* cpu) { this->cpu = cpu; }
        inline void attachMemoryInstance(Memory* memory) { this->memory = memory; }
        inline void attachPSGInstance(PSG* psg) { this->psg = psg; }
        inline void attachVDPInstance(VDP* vdp) { this->vdp = vdp; }

        void enterMonitor();

        // Bus
        inline uint8_t readRAM(uint16_t address) const { return bus->readMemory(address); }

        // Cartridge
        CartridgeInfo getCartridgeInfo() const;

        // CPU access for monitor/debug commands
        CPU& getCPU();
        const CPU& getCPU() const;

        // PC getter/setter
        uint16_t getPC() const;
        void setPC(uint16_t address);

        // Step Command API
        Z80DisassembledInstruction disassembleAt(uint16_t address) const;
        int stepInstruction();

        void printCPUState() const;
        void printCPUIRQState() const;
        void printCPUCycles() const;
        void printCPUFlags() const;
        void printCPUStack(int count) const;

        // Debug memory access through CPU/bus path
        uint8_t debugRead8(uint16_t address) const;
        void debugWrite8(uint16_t address, uint8_t value);

        // VDP Functions
        uint8_t getVDPRegister(uint8_t index) const;
        VDPMode getVDPMode() const;
        VDPStatusSnapshot getVDPStatusSnapshot() const;
        uint8_t peekVDPVRAM(uint16_t address) const;

    protected:

    private:
        // Non-owning pointers
        Bus* bus;
        Cartridge* cartridge;
        ColecoVisionSystem* host;
        Controller* controller1;
        Controller* controller2;
        CPU* cpu;
        Memory* memory;
        PSG* psg;
        VDP* vdp;

};

#endif // MLMONITORBACKEND_H

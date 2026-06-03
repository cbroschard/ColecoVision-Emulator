// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef MLMONITORBACKEND_H
#define MLMONITORBACKEND_H

#include "VDP.h"

// Forward declarations
class Bus;
class Cartridge;
class ColecoVisionSystem;
class Controller;
class CPU;
class Memory;
class PSG;

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

        // VDP Functions
        inline VDPMode getVDPMode() const { return vdp->getMode(); }

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

// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include <iostream>
#include <iomanip>
#include "Controller.h"
#include "Memory.h"
#include "Debug/MLMonitor.h"
#include "PSG.h"
#include "VDP.h"

class Bus
{
    public:
        Bus();
        virtual ~Bus();

    inline void attachController1Instance(Controller* controller1) { this->controller1 = controller1; }
    inline void attachController2Instance(Controller* controller2) { this->controller2 = controller2; }
    inline void attachMemoryInstance(Memory* memory) { this->memory = memory; }
    inline void attachMLMonitorInstance(MLMonitor* mlMonitor) { this->mlMonitor = mlMonitor; }
    inline void attachPSGInstance(PSG* psg) { this->psg = psg; }
    inline void attachVDPInstance(VDP* vdp) { this->vdp = vdp; }

    uint8_t readMemory(uint16_t adddress);
    void writeMemory(uint16_t address, uint8_t value);

    uint8_t readIO(uint8_t port);
    void writeIO(uint8_t port, uint8_t value);

    protected:

    private:
        // Non-owning pointers
        Controller* controller1;
        Controller* controller2;
        Memory* memory;
        MLMonitor* mlMonitor;
        PSG* psg;
        VDP* vdp;
};

#endif // BUS_H

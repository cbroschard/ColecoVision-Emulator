// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef BUS_H
#define BUS_H

#include "Cartridge.h"
#include "Controller.h"
#include "Memory.h"
#include "PSG.h"
#include "VDP.h"

class Bus
{
    public:
        Bus();
        virtual ~Bus();

    inline void attachCartridgeInstance(Cartridge* cart) { this->cart = cart; }
    inline void attachController1Instance(Controller* controller1) { this->controller1 = controller1; }
    inline void attachController2Instance(Controller* controller2) { this->controller2 = controller2; }
    inline void attachMemoryInstance(Memory* mem) { this->mem = mem; }
    inline void attachPSGInstance(PSG* psg) { this->psg = psg; }
    inline void attachVDPInstance(VDP* vdp) { this->vdp = vdp; }

    protected:

    private:
        // Non-owning pointers
        Cartridge* cart;
        Controller* controller1;
        Controller* controller2;
        Memory* mem;
        PSG* psg;
        VDP* vdp;
};

#endif // BUS_H

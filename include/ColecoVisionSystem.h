// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef COLECOVISIONSYSTEM_H
#define COLECOVISIONSYSTEM_H

#include <cstdint>
#include <memory>
#include "Bus.h"
#include "Cartridge.h"
#include "Controller.h"
#include "CPU.h"
#include "Memory.h"
#include "VDP.h"

class ColecoVisionSystem
{
    public:
        ColecoVisionSystem();
        virtual ~ColecoVisionSystem();

    protected:

    private:
        std::unique_ptr<Bus> bus;
        std::unique_ptr<Cartridge> cart;
        std::unique_ptr<Controller> controller1;
        std::unique_ptr<Controller> controller2;
        std::unique_ptr<CPU> cpu;
        std::unique_ptr<Memory> mem;
        std::unique_ptr<PSG> psg;
        std::unique_ptr<VDP> vdp;

        void wireUp();};

#endif // COLECOVISIONSYSTEM_H

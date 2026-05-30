// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "ColecoVisionSystem.h"

ColecoVisionSystem::ColecoVisionSystem()
{
    bus = std::make_unique<Bus>();
    cart = std::make_unique<Cartridge>();
    controller1 = std::make_unique<Controller>();
    controller2 = std::make_unique<Controller>();
    cpu = std::make_unique<CPU>();
    mem = std::make_unique<Memory>();
    psg = std::make_unique<PSG>();
    vdp = std::make_unique<VDP>();
}

ColecoVisionSystem::~ColecoVisionSystem()
{

}

void ColecoVisionSystem::wireUp()
{
    bus->attachCartridgeInstance(cart.get());
    bus->attachController1Instance(controller1.get());
    bus->attachController2Instance(controller2.get());
    bus->attachMemoryInstance(mem.get());
    bus->attachPSGInstance(psg.get());
    bus->attachVDPInstance(vdp.get());

    cpu->attachBusInstance(bus.get());
}

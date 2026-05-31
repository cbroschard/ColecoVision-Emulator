// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Bus.h"

Bus::Bus() :
    controller1(nullptr),
    controller2(nullptr),
    mem(nullptr),
    psg(nullptr),
    vdp(nullptr)
{

}

Bus::~Bus()
{

}

uint8_t Bus::readMemory(uint16_t address)
{
    if (!mem)
        return 0xFF;

    return mem->read(address);
}

void Bus::writeMemory(uint16_t address, uint8_t value)
{
    if (!mem)
        return;

    mem->write(address, value);
}

uint8_t Bus::readIO(uint8_t port)
{
    return 0xFF;
}

void Bus::writeIO(uint8_t port, uint8_t value)
{
    // TODO: route to VDP, PSG, controllers
}

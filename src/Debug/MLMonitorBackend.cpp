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
#include "VDP.h"

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

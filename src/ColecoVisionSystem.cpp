// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include <stdexcept>
#include "ColecoVisionSystem.h"

ColecoVisionSystem::ColecoVisionSystem()
{
    bus = std::make_unique<Bus>();
    cart = std::make_unique<Cartridge>();
    controller1 = std::make_unique<Controller>();
    controller2 = std::make_unique<Controller>();
    cpu = std::make_unique<CPU>();
    inputManager = std::make_unique<InputManager>();
    mem = std::make_unique<Memory>();
    psg = std::make_unique<PSG>();
    vdp = std::make_unique<VDP>();
    videoOutput = std::make_unique<VideoOutput>();

    wireUp();
}

ColecoVisionSystem::~ColecoVisionSystem()
{

}

void ColecoVisionSystem::reset()
{
    // Hardware resets
    mem->reset();
    cart->reset();
    cpu->reset();
    vdp->reset();
    psg->reset();

    // Reset controllers
    inputManager->reset();
}

void ColecoVisionSystem::run()
{
    if (biosPath.empty())
    {
        throw std::runtime_error("BIOS path has not been set.");
    }

    if (!loadBIOS(biosPath))
    {
        throw std::runtime_error("Failed to load BIOS: " + biosPath);
    }

    reset();

    constexpr int CPU_CLOCK_HZ = 3579545;
    constexpr int FPS = 60;
    constexpr int CYCLES_PER_FRAME = CPU_CLOCK_HZ / FPS;

    bool running = true;

    while (running)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }

            inputManager->handleEvent(event);
        }

        int frameCycles = 0;

        while (frameCycles < CYCLES_PER_FRAME)
        {
            const int cpuCycles = cpu->step();

            frameCycles += cpuCycles;

            vdp->tick(cpuCycles);

            // psg->tick(cpuCycles);
        }

        videoOutput->clear();

        vdp->renderFrame(*videoOutput);

        videoOutput->present();

        SDL_Delay(16);
    }
}

void ColecoVisionSystem::setBIOSPath(const std::string& path)
{
    biosPath = path;
}

bool ColecoVisionSystem::loadBIOS(const std::string& path)
{
    return mem->loadBIOS(path);
}

bool ColecoVisionSystem::loadCartridge(const std::string& path)
{
    return cart->loadROM(path);
}

void ColecoVisionSystem::wireUp()
{
    bus->attachController1Instance(controller1.get());
    bus->attachController2Instance(controller2.get());
    bus->attachMemoryInstance(mem.get());
    bus->attachPSGInstance(psg.get());
    bus->attachVDPInstance(vdp.get());

    cpu->attachBusInstance(bus.get());

    inputManager->attachController1(controller1.get());
    inputManager->attachController2(controller2.get());

    mem->attachCartridgeInstance(cart.get());
}

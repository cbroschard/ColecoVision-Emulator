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
    audioOutput = std::make_unique<AudioOutput>();
    bus = std::make_unique<Bus>();
    cartridge = std::make_unique<Cartridge>();
    controller1 = std::make_unique<Controller>();
    controller2 = std::make_unique<Controller>();
    cpu = std::make_unique<CPU>();
    inputManager = std::make_unique<InputManager>();
    irqLine = std::make_unique<IRQLine>();
    memory = std::make_unique<Memory>();
    mlMonitor = std::make_unique<MLMonitor>();
    monbackend = std::make_unique<MLMonitorBackend>();
    monitorController = std::make_unique<MonitorController>();
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
    memory->reset();
    cartridge->reset();
    cpu->reset();
    irqLine->reset();
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

    if (!audioOutput->playAudio())
    {
        throw std::runtime_error("Failed to start audio.");
    }

    while (running)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
                continue;
            }

            // F12 toggles the ML monitor window.
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F12)
            {
                monitorController->toggleMonitor();
                continue;
            }

            // Let monitor receive typing, close-window events, etc.
            if (monitorController->handleEvent(event))
                continue;

            inputManager->handleEvent(event);
        }

        /*
            If the monitor is open, pause the emulated machine.

            We still tick/render the monitor window so it remains responsive,
            but we do NOT step the CPU, tick the VDP, update IRQs, queue audio,
            or advance the emulated frame.
        */
        if (monitorController->isOpen())
        {
            monitorController->tick();
            SDL_Delay(16);
            continue;
        }

        int frameCycles = 0;

        while (frameCycles < CYCLES_PER_FRAME)
        {
            // Present current IRQ line state before the CPU decides
            // whether to service an interrupt.
            if (vdp->isIRQAsserted())
                irqLine->raiseIRQ(IRQSource::VDP);
            else
                irqLine->clearIRQ(IRQSource::VDP);

            cpu->setIRQ(irqLine->isAsserted());



            const int cpuCycles = cpu->step();

            frameCycles += cpuCycles;

            vdp->tick(cpuCycles);

            // VDP may have entered VBlank during this CPU instruction.
            // Update the line immediately for the next instruction.
            if (vdp->isIRQAsserted())
                irqLine->raiseIRQ(IRQSource::VDP);
            else
                irqLine->clearIRQ(IRQSource::VDP);

            cpu->setIRQ(irqLine->isAsserted());
        }

        videoOutput->clear();

        audioOutput->queueSamples();

        vdp->renderFrame(*videoOutput);

        videoOutput->present();

        monitorController->tick();

        SDL_Delay(16);
    }
}

void ColecoVisionSystem::setBIOSPath(const std::string& path)
{
    biosPath = path;
}

bool ColecoVisionSystem::loadBIOS(const std::string& path)
{
    return memory->loadBIOS(path);
}

bool ColecoVisionSystem::loadCartridge(const std::string& path)
{
    return cartridge->loadROM(path);
}

void ColecoVisionSystem::wireUp()
{
    audioOutput->attachPSGInstance(psg.get());

    bus->attachController1Instance(controller1.get());
    bus->attachController2Instance(controller2.get());
    bus->attachMemoryInstance(memory.get());
    bus->attachPSGInstance(psg.get());
    bus->attachVDPInstance(vdp.get());

    cpu->attachBusInstance(bus.get());

    inputManager->attachController1(controller1.get());
    inputManager->attachController2(controller2.get());

    memory->attachCartridgeInstance(cartridge.get());

    monbackend->attachBusInstance(bus.get());
    monbackend->attachCartridgeInstance(cartridge.get());
    monbackend->attachController1Instance(controller1.get());
    monbackend->attachController2Instance(controller2.get());
    monbackend->attachCPUInstance(cpu.get());
    monbackend->attachMemoryInstance(memory.get());
    monbackend->attachPSGInstance(psg.get());
    monbackend->attachVDPInstance(vdp.get());

    mlMonitor->attachMLMonitorBackendInstance(monbackend.get());

    monitorController->attachMonitorInstance(mlMonitor.get());
}

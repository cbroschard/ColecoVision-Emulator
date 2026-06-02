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
#include <string>
#include "AudioOutput.h"
#include "Bus.h"
#include "Cartridge.h"
#include "Controller.h"
#include "CPU.h"
#include "Debug/MlMonitorBackend.h"
#include "InputManager.h"
#include "IRQLine.h"
#include "Memory.h"
#include "MonitorController.h"
#include "SDL3/SDL.h"
#include "VDP.h"
#include "VideoOutput.h"

class ColecoVisionSystem
{
    public:
        ColecoVisionSystem();
        virtual ~ColecoVisionSystem();

        void reset();

        void run();

        void setBIOSPath(const std::string& path);

        bool loadBIOS(const std::string& path);
        bool loadCartridge(const std::string& path);

        // ML Monitor
        inline void enterMonitor() { monitorController->openMonitor(); }

    protected:

    private:
        std::unique_ptr<AudioOutput> audioOutput;
        std::unique_ptr<Bus> bus;
        std::unique_ptr<Cartridge> cartridge;
        std::unique_ptr<Controller> controller1;
        std::unique_ptr<Controller> controller2;
        std::unique_ptr<CPU> cpu;
        std::unique_ptr<InputManager> inputManager;
        std::unique_ptr<IRQLine> irqLine;
        std::unique_ptr<Memory> memory;
        std::unique_ptr<MLMonitorBackend> monbackend;
        std::unique_ptr<MonitorController> monitorController;
        std::unique_ptr<PSG> psg;
        std::unique_ptr<VDP> vdp;
        std::unique_ptr<VideoOutput> videoOutput;

        static constexpr int CPU_CLOCK_HZ = 3579545;
        static constexpr int FPS = 60;
        static constexpr int CYCLES_PER_FRAME = CPU_CLOCK_HZ / FPS;

        std::string biosPath;

        void wireUp();};

#endif // COLECOVISIONSYSTEM_H

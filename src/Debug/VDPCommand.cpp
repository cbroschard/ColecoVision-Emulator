// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Debug/MLMonitor.h"
#include "Debug/VDPCommand.h"

static const char* decodeVDPModeName(VDPMode mode)
{
    switch(mode)
    {
        case VDPMode::GraphicsI:
            return "Graphics I";
        case VDPMode::GraphicsII:
            return "Graphics II";
        case VDPMode::Text:
            return "Text";
        case VDPMode::Multicolor:
            return "Multicolor";
        default:
            return "Unsupported Mode";
    }
}

VDPCommand::VDPCommand() = default;

VDPCommand::~VDPCommand() = default;

int VDPCommand::order() const
{
    return 5;
}

std::string VDPCommand::name() const
{
    return "vdp";
}

std::string VDPCommand::category() const
{
    return "Chip/TMS9918A VDP";
}

std::string VDPCommand::shortHelp() const
{
    return "vdp       - VDP operations (use 'vdp help')";
}

std::string VDPCommand::help() const
{
    return
        "vdp <subcommand>:\n"
        "    mode                   Show current VIC-II graphics mode\n";
}

void VDPCommand::execute(MLMonitor& mlMonitor, const std::vector<std::string>& args)
{
    if (args.size() < 2)
    {
        std::cout << help() << std::endl;
        return;
    }

    const std::string& sub = args[1];

    if (isHelp(sub))
    {
        std::cout << "Usage:\n" << help() << std::endl;
        return;
    }
    else if (sub == "mode")
    {
        VDPMode currentMode = mlMonitor.getMLMonitorBackend()->getVDPMode();
        std::cout << "Current VDP mode: "
                  << decodeVDPModeName(currentMode)
                  << std::endl;
        return;
    }
    else
    {
        std::cout << "Unknown vdp subcommand: " << sub << std::endl;
        std::cout << help() << std::endl;
        return;
    }
}

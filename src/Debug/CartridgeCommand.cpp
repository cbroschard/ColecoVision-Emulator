// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Debug/CartridgeCommand.h"
#include "Debug/MLMonitor.h"

CartridgeCommand::CartridgeCommand() = default;

CartridgeCommand::~CartridgeCommand() = default;

int CartridgeCommand::order() const
{
    return 5;
}

std::string CartridgeCommand::name() const
{
    return "cart";
}

std::string CartridgeCommand::category() const
{
    return "Cartridge";
}

std::string CartridgeCommand::shortHelp() const
{
    return "cart      - Cartridge information";
}

std::string CartridgeCommand::help() const
{
    return
        "Cartridge command\n"
        "\n"
        "Usage:\n"
        "  cart info        Show loaded cartridge information\n"
        "  cart help        Show this help\n";
}

void CartridgeCommand::execute(MLMonitor& mlMonitor, const std::vector<std::string>& args)
{
    MLMonitorBackend* backend = mlMonitor.getMLMonitorBackend();

    if (backend == nullptr)
    {
        std::cout << "Monitor backend is not attached." << std::endl;
        return;
    }

    // cart
    if (args.size() < 2)
    {
        CartridgeInfo info = backend->getCartridgeInfo();

        std::cout << "Cartridge:" << std::endl;

        if (!info.loaded)
        {
            std::cout << "  Loaded: no" << std::endl;
            return;
        }

        std::cout << "  Loaded: yes" << std::endl;
        std::cout << "  File: " << info.fileName << std::endl;
        std::cout << "  Size: " << info.romSizeBytes << " bytes";

        if (info.romSizeKB > 0)
            std::cout << " / " << info.romSizeKB << " KB";

        std::cout << std::endl;

        std::cout << "  CRC32: $" << hex8(info.crc32) << std::endl;

        std::cout << std::endl;
        std::cout << "Header:" << std::endl;
        std::cout << "  Signature: $"
                  << hex2(info.signature0)
                  << " $"
                  << hex2(info.signature1)
                  << std::endl;

        std::cout << "  Signature valid: "
                  << (info.validColecoSignature ? "yes" : "no")
                  << std::endl;

        if (info.hasEntryPoint)
        {
            std::cout << "  Entry/start: $"
                      << hex4(info.entryPoint)
                      << std::endl;
        }

        std::cout << std::endl;
        std::cout << "Mapping:" << std::endl;
        std::cout << "  Type: " << info.mappingType << std::endl;
        std::cout << "  CPU range: $8000-$FFFF" << std::endl;
        std::cout << "  Mirroring: " << info.mirrorDescription << std::endl;

        return;
    }

    // cart help / cart ?
    if (isHelp(args[1]))
    {
        std::cout << "Usage:\n" << help() << std::endl;
        return;
    }

    // cart info
    if (args[1] == "info")
    {
        CartridgeInfo info = backend->getCartridgeInfo();

        std::cout << "Cartridge:" << std::endl;

        if (!info.loaded)
        {
            std::cout << "  Loaded: no" << std::endl;
            return;
        }

        std::cout << "  Loaded: yes" << std::endl;
        std::cout << "  File: " << info.fileName << std::endl;
        std::cout << "  Size: " << info.romSizeBytes << " bytes";

        if (info.romSizeKB > 0)
            std::cout << " / " << info.romSizeKB << " KB";

        std::cout << std::endl;

        std::cout << "  CRC32: $" << hex8(info.crc32) << std::endl;

        std::cout << std::endl;
        std::cout << "Header:" << std::endl;
        std::cout << "  Signature: $"
                  << hex2(info.signature0)
                  << " $"
                  << hex2(info.signature1)
                  << std::endl;

        std::cout << "  Signature valid: "
                  << (info.validColecoSignature ? "yes" : "no")
                  << std::endl;

        if (info.hasEntryPoint)
        {
            std::cout << "  Entry/start: $"
                      << hex4(info.entryPoint)
                      << std::endl;
        }

        std::cout << std::endl;
        std::cout << "Mapping:" << std::endl;
        std::cout << "  Type: " << info.mappingType << std::endl;
        std::cout << "  CPU range: $8000-$FFFF" << std::endl;
        std::cout << "  Mirroring: " << info.mirrorDescription << std::endl;

        return;
    }

    std::cout << "Unknown cart command: " << args[1] << std::endl;
    std::cout << "Usage:\n" << help() << std::endl;
}

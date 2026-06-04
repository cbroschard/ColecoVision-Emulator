// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Debug/GoCommand.h"
#include "Debug/MLMonitor.h"

GoCommand::GoCommand() = default;

GoCommand::~GoCommand() = default;

int GoCommand::order() const
{
    return 5;
}

std::string GoCommand::name() const
{
    return "g";
}

std::string GoCommand::category() const
{
    return "Execution";
}

std::string GoCommand::shortHelp() const
{
    return "g [addr|force] - Start execution";
}

std::string GoCommand::help() const
{
    return
        "g                - Start execution\n"
        "g <addr>         - Start execution at $addr\n";
}

void GoCommand::execute(MLMonitor& mlMonitor, const std::vector<std::string>& args)
{
    if (args.size() >= 2 && isHelp(args[1]))
    {
        std::cout << "Usage:\n" << help();
        return;
    }

    if (args.size() > 3)
    {
        std::cout << "Usage:\n" << help();
        return;
    }

    bool haveAddress = false;
    uint16_t address = 0;

    for (size_t i = 1; i < args.size(); ++i)
    {
        if (haveAddress)
        {
            std::cout << "Usage:\n" << help();
            return;
        }

        try
        {
            address = parseAddress(args[i]);
            haveAddress = true;
        }
        catch (...)
        {
            std::cout << "Invalid argument: " << args[i] << "\n";
            std::cout << "Usage:\n" << help();
            return;
        }
    }

     if (haveAddress)
        mlMonitor.getMLMonitorBackend()->setPC(address);

    mlMonitor.setRunningFlag(false);
}

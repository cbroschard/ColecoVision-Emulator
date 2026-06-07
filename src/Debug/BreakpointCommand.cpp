// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Debug/BreakpointCommand.h"
#include "Debug/MLMonitor.h"

BreakpointCommand::BreakpointCommand() = default;

BreakpointCommand::~BreakpointCommand() = default;

int BreakpointCommand::order() const
{
    return 5;
}

std::string BreakpointCommand::name() const
{
    return "bp";
}

std::string BreakpointCommand::category() const
{
    return "Debugging";
}

std::string BreakpointCommand::shortHelp() const
{
    return "bp        - Manage breakpoints (set / clear / list)";
}

std::string BreakpointCommand::help() const
{
    return
  "bp - Manage breakpoints (set | list | clear)\n"
  "\n"
  "USAGE\n"
  "  bp <address>\n"
  "      Set a breakpoint at the given address.\n"
  "  bp list\n"
  "      List all currently set breakpoints.\n"
  "  bp clear [<address>|all]\n"
  "      Clear a breakpoint at <address>, or all if omitted/\"all\".\n"
  "\n"
  "ARGS\n"
  "  <address>    Hex address (e.g., $8000 or 8000).\n"
  "\n"
  "NOTES\n"
  "  - Multiple breakpoints are supported; use 'bp list' to view them.\n"
  "  - Use 'bp clear <address>' to remove a specific breakpoint, or 'bp clear all'.\n"
  "\n"
  "EXAMPLES\n"
  "  bp $8000           Set a breakpoint at $8000\n"
  "  bp list            Show active breakpoints\n"
  "  bp clear $8000     Remove breakpoint at $8000\n"
  "  bp clear all       Remove all breakpoints\n";
}

void BreakpointCommand::execute(MLMonitor& mlMonitor, const std::vector<std::string>& args)
{
    // help
    if (args.size() == 1 || (args.size() >= 2 && isHelp(args[1])))
    {
        std::cout << help();
        return;
    }

    // Convenience: "bp <addr>" means "bp set <addr>"
    if (args.size() == 2 && args[1] != "set" && args[1] != "list" && args[1] != "clear")
    {
        try {
            uint16_t address = parseAddress(args[1]);
            mlMonitor.addBreakpoint(address);
            std::cout << "Breakpoint set at $"
                      << std::uppercase << std::hex << std::setw(4) << std::setfill('0')
                      << address << std::dec << "\n";
        } catch (...) {
            std::cout << "Error: invalid address.\n" << help();
        }
        return;
    }

    const std::string sub = args[1];

    if (sub == "set")
    {
        if (args.size() < 3) { std::cout << "Usage: bp set <address>\n"; return; }
        try {
            uint16_t address = parseAddress(args[2]);
            mlMonitor.addBreakpoint(address);
            std::cout << "Breakpoint set at $"
                      << std::uppercase << std::hex << std::setw(4) << std::setfill('0')
                      << address << std::dec << "\n";
        } catch (...) {
            std::cout << "Error: invalid address.\n";
        }
    }
    else if (sub == "clear")
    {
        // Forms: bp clear <address>  |  bp clear all
        if (args.size() < 3) { std::cout << "Usage: bp clear <address>|all\n"; return; }

        if (args[2] == "all")
        {
            mlMonitor.clearAllBreakpoints();
            std::cout << "All breakpoints cleared.\n";
            return;
        }

        try {
            uint16_t addr = parseAddress(args[2]);
            mlMonitor.clearBreakpoint(addr);
            std::cout << "Breakpoint cleared at $"
                      << std::uppercase << std::hex << std::setw(4) << std::setfill('0')
                      << addr << std::dec << "\n";
        } catch (...) {
            std::cout << "Error: invalid address.\n";
        }
    }
    else if (sub == "list")
    {
        if (mlMonitor.breakpointsEmpty())
        {
            std::cout << "No active breakpoints.\n";
            return;
        }
        std::cout << "Active breakpoints:\n";
        mlMonitor.listBreakpoints();
    }
    else
    {
        std::cout << "Invalid command.\n" << help();
    }
}

// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Debug/MLMonitor.h"
#include "Debug/WatchCommand.h"

namespace
{
    struct WatchRange
    {
        uint16_t start = 0;
        uint16_t end = 0;
    };

    WatchRange parseAddressOrRange(const std::string& s)
    {
        const auto dash = s.find('-');
        if (dash == std::string::npos)
        {
            uint16_t a = parseAddress(s);
            return { a, a };
        }

        const std::string lhs = s.substr(0, dash);
        const std::string rhs = s.substr(dash + 1);

        uint16_t start = parseAddress(lhs);
        uint16_t end   = parseAddress(rhs);

        if (end < start)
            std::swap(start, end);

        return { start, end };
    }

    void applyAddRange(MLMonitor& mon, uint8_t bits, uint16_t start, uint16_t end)
    {
        for (uint32_t a = start; a <= end; ++a)
        {
            if (bits & 1) mon.addReadWatch(static_cast<uint16_t>(a));
            if (bits & 2) mon.addWriteWatch(static_cast<uint16_t>(a));
        }
    }

    void applyClearRange(MLMonitor& mon, uint8_t bits, uint16_t start, uint16_t end)
    {
        for (uint32_t a = start; a <= end; ++a)
        {
            if (bits & 1) mon.clearReadWatch(static_cast<uint16_t>(a));
            if (bits & 2) mon.clearWriteWatch(static_cast<uint16_t>(a));
        }
    }
}

WatchCommand::WatchCommand() = default;

WatchCommand::~WatchCommand() = default;

int WatchCommand::order() const
{
    return 20;
}

std::string WatchCommand::name() const
{
    return "watch";
}

std::string WatchCommand::category() const
{
    return "Debugging";
}

std::string WatchCommand::shortHelp() const
{
    return "watch [read|write|both] [addr|start-end] - Manage watchpoints";
}

std::string WatchCommand::help() const
{
    return R"(watch - manage memory watchpoints (reads, writes, or both)

    Watchpoints stop in the monitor when the CPU reads or writes a watched
    memory address.

ColecoVision memory map:
    $0000-$1FFF   BIOS ROM
    $2000-$5FFF   Unmapped / expansion area
    $6000-$7FFF   System RAM, mirrored through this range
    $8000-$FFFF   Cartridge ROM

Listing:
    watch                         List all watchpoints in a single table (R/W/RW)
    watch list                    Same as above

Adding:
    watch <addr>                  Add WRITE watch at <addr>
    watch <start>-<end>           Add WRITE watch across range
    watch write <addr>            Add WRITE watch
    watch write <start>-<end>     Add WRITE watch across range
    watch read  <addr>            Add READ watch
    watch read  <start>-<end>     Add READ watch across range
    watch both  <addr>            Add BOTH read & write watches
    watch both  <start>-<end>     Add BOTH read & write watches across range
    watch rw    <addr>            Alias for both
    watch rw    <start>-<end>     Alias for both across range

Clearing:
    watch clear                         Clear ALL watches
    watch clear <addr>                  Clear BOTH at <addr>
    watch clear <start>-<end>           Clear BOTH across range
    watch clear read  <addr>            Clear READ at <addr>
    watch clear read  <start>-<end>     Clear READ across range
    watch clear write <addr>            Clear WRITE at <addr>
    watch clear write <start>-<end>     Clear WRITE across range

Examples:
    watch write $72DD            Watch a game RAM variable for writes
    watch read  $72DD            Watch a game RAM variable for reads
    watch both  $7000-$70FF      Watch a RAM range for reads and writes
    watch read  $8000-$80FF      Watch cartridge ROM fetch/data reads
    watch write $6000-$7FFF      Watch writes to ColecoVision RAM

Notes:
    CPU memory watchpoints observe CPU memory accesses through the normal
    memory map. They do not watch VDP VRAM, PSG ports, controller ports, or
    other I/O port activity.

    For VDP VRAM debugging, use VDP-specific commands such as vdp vram.
)";
}

void WatchCommand::execute(MLMonitor& mon, const std::vector<std::string>& args)
{
    auto isMode = [](std::string m)
    {
        std::transform(m.begin(), m.end(), m.begin(), ::tolower);
        return m == "read" || m == "r" ||
               m == "write" || m == "w" ||
               m == "both" || m == "rw";
    };

    auto toBits = [](std::string m)->uint8_t
    {
        std::transform(m.begin(), m.end(), m.begin(), ::tolower);
        if (m == "read" || m == "r")   return 1; // R
        if (m == "write" || m == "w")  return 2; // W
        return 3; // both/rw
    };

    if (args.size() > 1 && isHelp(args[1]))
    {
        std::cout << help();
        return;
    }

    // LIST
    if (args.size() == 1 || args[1] == "list")
    {
        std::unordered_map<uint16_t, uint8_t> modes;

        for (auto a : mon.getReadWatchAddresses())  modes[a] |= 1;
        for (auto a : mon.getWriteWatchAddresses()) modes[a] |= 2;

        std::vector<uint16_t> addrs;
        addrs.reserve(modes.size());
        for (auto& kv : modes) addrs.push_back(kv.first);
        std::sort(addrs.begin(), addrs.end());

        if (addrs.empty())
        {
            std::cout << "(no watchpoints)\n";
            return;
        }

        for (auto a : addrs)
        {
            uint8_t m = modes[a];
            std::cout << "$" << std::hex << std::uppercase
                      << std::setw(4) << std::setfill('0') << a
                      << "   " << ((m & 1) ? "R" : "-")
                      << ((m & 2) ? "W" : "-") << "\n";
        }
        return;
    }

    // CLEAR
    if (args[1] == "clear")
    {
        if (args.size() == 2)
        {
            mon.clearAllReadWatches();
            mon.clearAllWriteWatches();
            return;
        }

        // watch clear <addr|range> => both
        if (args.size() == 3 && !isMode(args[2]))
        {
            try
            {
                WatchRange r = parseAddressOrRange(args[2]);
                applyClearRange(mon, 3, r.start, r.end);
            }
            catch (...)
            {
                std::cout << "Error: invalid address or range\n";
            }
            return;
        }

        // watch clear <mode> <addr|range>
        if (args.size() == 4 && isMode(args[2]))
        {
            try
            {
                uint8_t bits = toBits(args[2]);
                WatchRange r = parseAddressOrRange(args[3]);
                applyClearRange(mon, bits, r.start, r.end);
            }
            catch (...)
            {
                std::cout << "Error: invalid address or range\n";
            }
            return;
        }

        std::cout << help();
        return;
    }

    // ADD
    try
    {
        // watch <mode> <addr|range>
        if (isMode(args[1]))
        {
            if (args.size() < 3)
            {
                std::cout << "Error: missing address\n" << help();
                return;
            }

            uint8_t bits = toBits(args[1]);
            WatchRange r = parseAddressOrRange(args[2]);
            applyAddRange(mon, bits, r.start, r.end);
            return;
        }

        // watch <addr|range> (default WRITE for back-compat)
        WatchRange r = parseAddressOrRange(args[1]);
        applyAddRange(mon, 2, r.start, r.end);
    }
    catch (...)
    {
        std::cout << "Error: invalid address or range\n" << help();
    }
}

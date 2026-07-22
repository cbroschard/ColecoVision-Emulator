// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include <iomanip>
#include <stdexcept>
#include "Debug/AssembleCommand.H"
#include "Debug/BreakpointCommand.h"
#include "Debug/CartridgeCommand.h"
#include "Debug/CPUCommand.h"
#include "Debug/DisassembleCommand.h"
#include "Debug/GoCommand.h"
#include "Debug/MemoryDumpCommand.h"
#include "Debug/MemoryEditCommand.h"
#include "Debug/MLMonitor.h"
#include "Debug/StepCommand.h"
#include "Debug/VDPCommand.h"
#include "Debug/WatchCommand.h"

MLMonitor::MLMonitor() :
    running(false),
    breakRequested(false),
    outputFileEnabled(false)
{
    registerCommand(std::make_unique<AssembleCommand>());
    registerCommand(std::make_unique<BreakpointCommand>());
    registerCommand(std::make_unique<CartridgeCommand>());
    registerCommand(std::make_unique<CPUCommand>());
    registerCommand(std::make_unique<DisassembleCommand>());
    registerCommand(std::make_unique<GoCommand>());
    registerCommand(std::make_unique<MemoryDumpCommand>());
    registerCommand(std::make_unique<MemoryEditCommand>());
    registerCommand(std::make_unique<StepCommand>());
    registerCommand(std::make_unique<VDPCommand>());
    registerCommand(std::make_unique<WatchCommand>());
}

MLMonitor::~MLMonitor()
{
    if (outputFile.is_open())
        outputFile.close();
}

std::string MLMonitor::executeAndCapture(const std::string& cmdLine)
{
    std::ostringstream buffer;
    auto* old = std::cout.rdbuf(buffer.rdbuf()); // Redirect std::cout to buffer

    try
    {
        handleCommand(cmdLine);
    }
    catch (const std::exception& ex)
    {
        std::cout << "Error executing command: " << ex.what() << "\n";
    }
    catch (...)
    {
        std::cout << "Error executing command: unknown exception\n";
    }

    std::cout.rdbuf(old); // Restore std::cout

    const std::string out = buffer.str();

    // Tee monitor command + output to file if enabled.
    writeOutputFileBlock(cmdLine, out);

    return out;
}

void MLMonitor::enterMonitor()
{
    if (mlmonitorBackend) mlmonitorBackend->enterMonitor();
}

std::string MLMonitor::getPrompt() const
{
    const auto iterator = commands.find("a");

    if (iterator != commands.end())
    {
        const auto* assembleCommand =
            dynamic_cast<const AssembleCommand*>(
                iterator->second.get());

        if (assembleCommand != nullptr &&
            assembleCommand->isInteractiveActive())
        {
            return assembleCommand->currentPrompt();
        }
    }

    return "> ";
}

void MLMonitor::clearBreakpoint(uint16_t bp)
{
    auto record = breakpoints.find(bp);
    if (record != breakpoints.end())
    {
        breakpoints.erase(bp);
    }
}

void MLMonitor::listBreakpoints() const
{
    std::vector<uint16_t> sortedBreakpoints(
        breakpoints.begin(),
        breakpoints.end());

    std::sort(
        sortedBreakpoints.begin(),
        sortedBreakpoints.end());

    for (std::size_t index = 0;
         index < sortedBreakpoints.size();
         ++index)
    {
        std::cout
            << "[" << index << "]  $"
            << std::uppercase
            << std::hex
            << std::setw(4)
            << std::setfill('0')
            << sortedBreakpoints[index]
            << std::dec
            << std::nouppercase
            << std::setfill(' ')
            << "\n";
    }
}

void MLMonitor::addWriteWatch(uint16_t address)
{
    uint8_t value = mlmonitorBackend->readRAM(address);
    writeWatches[address] = value;
    std::cout << "Watchpoint set at $" << std::hex << std::setw(4) << std::setfill('0') << address
              << " (initial value = $" << std::setw(2) << static_cast<int>(value) << ")\n";
}

void MLMonitor::clearWriteWatch(uint16_t address)
{
    if (writeWatches.erase(address))
        std::cout << "Watchpoint cleared at $" << std::hex << std::setw(4) << std::setfill('0') << address << "\n";
    else
        std::cout << "No watchpoint found at $" << std::hex << std::setw(4) << std::setfill('0') << address << "\n";
}

void MLMonitor::clearAllWriteWatches()
{
    for (auto it = writeWatches.begin(); it != writeWatches.end(); )
    {
        uint16_t address = it->first;
        std::cout << "Watchpoint cleared at $"
                  << std::hex << std::setw(4) << std::setfill('0')
                  << address << "\n";

        it = writeWatches.erase(it); // erase returns next valid iterator
    }

    std::cout << "All writeWatches cleared.\n";
}

void MLMonitor::listWriteWatches() const
{
    int index = 0;
    for (const auto& [address, value] : writeWatches)
    {
        std::cout << "[" << index << "]  $" << std::hex << std::setw(4) << std::setfill('0') << address
                  << " (last value=$" << std::setw(2) << static_cast<int>(value) << ")\n";
        index++;
    }
}

bool MLMonitor::checkWatchWrite(uint16_t address, uint8_t newVal)
{
    auto it = writeWatches.find(address);
    if (it != writeWatches.end())
    {
        if (it->second != newVal)
        {
            uint8_t oldVal = it->second;
            it->second = newVal;

            std::ostringstream oss;
            oss << ">>> Watchpoint hit at $"
                << std::hex << std::setw(4) << std::setfill('0') << address
                << ": old=$" << std::setw(2) << static_cast<int>(oldVal)
                << " new=$" << std::setw(2) << static_cast<int>(newVal);

            queueAsyncLine(oss.str());
            return true;
        }
    }
    return false;
}

void MLMonitor::addReadWatch(uint16_t address)
{
    readWatches.insert(address);
    std::cout << "Read watchpoint set at $" << std::hex << std::setw(4) << std::setfill('0') << address << "\n";
}

std::vector<uint16_t> MLMonitor::getWriteWatchAddresses() const
{
    std::vector<uint16_t> out;
    out.reserve(writeWatches.size());
    for (const auto& kv : writeWatches) out.push_back(kv.first);
    return out;
}

void MLMonitor::clearReadWatch(uint16_t address)
{
    if (readWatches.erase(address))
        std::cout << "Read watchpoint cleared at $" << std::hex << std::setw(4) << std::setfill('0') << address << "\n";
    else
        std::cout << "No read watchpoint found at $" << std::hex << std::setw(4) << std::setfill('0') << address << "\n";
}

void MLMonitor::clearAllReadWatches()
{
    for (auto it = readWatches.begin(); it != readWatches.end(); )
    {
        uint16_t addr = *it;
        std::cout << "Read watchpoint cleared at $" << std::hex << std::setw(4) << std::setfill('0') << addr << "\n";
        it = readWatches.erase(it);
    }
    std::cout << "All read watchpoints cleared.\n";
}

void MLMonitor::listReadWatches() const
{
    int index = 0;
    for (auto addr : readWatches)
    {
        std::cout << "[" << index++ << "]  $" << std::hex << std::setw(4) << std::setfill('0') << addr << "\n";
    }
}

bool MLMonitor::checkWatchRead(uint16_t address, uint8_t value)
{
    if (readWatches.find(address) != readWatches.end())
    {
        std::ostringstream oss;
        oss << ">>> Read watchpoint hit at $"
            << std::hex << std::setw(4) << std::setfill('0') << address
            << " (value=$" << std::setw(2) << static_cast<int>(value) << ")";

        queueAsyncLine(oss.str());
        return true;
    }
    return false;
}

std::vector<uint16_t> MLMonitor::getReadWatchAddresses() const
{
    return std::vector<uint16_t>(readWatches.begin(), readWatches.end());
}

bool MLMonitor::consumeBreakRequested()
{
    if (!breakRequested)
        return false;

    breakRequested = false;
    return true;
}

std::vector<std::string> MLMonitor::drainAsyncLines()
{
    std::lock_guard<std::mutex> lock(asyncMutex);
    std::vector<std::string> out;
    out.swap(asyncLines);
    return out;
}

void MLMonitor::handleCommand(const std::string& line)
{
    /*
     * Interactive assembler input must be handled before normal command
     * parsing. Otherwise a line such as:
     *
     *     LD A,$01
     *
     * would be interpreted as a monitor command named "ld".
     *
     * Blank input and "." are also passed through so AssembleCommand can
     * terminate interactive mode.
     */
    const auto assembleIterator =
        commands.find("a");

    if (assembleIterator != commands.end())
    {
        auto* assembleCommand =
            dynamic_cast<AssembleCommand*>(
                assembleIterator->second.get());

        if (assembleCommand != nullptr &&
            assembleCommand->isInteractiveActive())
        {
            assembleCommand->handleInteractiveLine(
                *this,
                line);

            return;
        }
    }

    std::istringstream iss(line);

    std::string cmd;
    iss >> cmd;

    // Blank command outside interactive assembler mode does nothing.
    if (cmd.empty())
        return;

    // Normalize the command name to lowercase.
    std::transform(
        cmd.begin(),
        cmd.end(),
        cmd.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(
                std::tolower(character));
        });

    if (cmd == "exit" ||
        cmd == "q" ||
        cmd == "quit")
    {
        running = false;
        return;
    }

    std::vector<std::string> args;
    args.push_back(cmd);

    std::string token;

    while (iss >> token)
        args.push_back(token);

    if (cmd == "out" ||
        cmd == "capture")
    {
        handleOutputFileCommand(args);
        return;
    }

    if (cmd == "help" ||
        cmd == "h" ||
        cmd == "?")
    {
        // help <command>
        if (args.size() >= 2)
        {
            std::string topic =
                args[1];

            std::transform(
                topic.begin(),
                topic.end(),
                topic.begin(),
                [](unsigned char character)
                {
                    return static_cast<char>(
                        std::tolower(character));
                });

            const auto commandIterator =
                commands.find(topic);

            if (commandIterator != commands.end())
            {
                const std::string text =
                    commandIterator->second->help();

                std::cout << text;

                if (!text.empty() &&
                    text.back() != '\n')
                {
                    std::cout << "\n";
                }
            }
            else
            {
                std::cout
                    << "Unknown command: "
                    << topic
                    << "\n";
            }

            return;
        }

        // Plain help command.
        std::map<
            std::string,
            std::vector<const MonitorCommand*>>
            groupedCommands;

        for (const auto& commandEntry : commands)
        {
            groupedCommands[
                commandEntry.second->category()]
                .push_back(
                    commandEntry.second.get());
        }

        std::vector<std::string> categories;

        for (const auto& categoryEntry :
             groupedCommands)
        {
            categories.push_back(
                categoryEntry.first);
        }

        std::sort(
            categories.begin(),
            categories.end(),
            [&groupedCommands](
                const std::string& left,
                const std::string& right)
            {
                const auto& leftCommands =
                    groupedCommands[left];

                const auto& rightCommands =
                    groupedCommands[right];

                const auto leftMinimum =
                    std::min_element(
                        leftCommands.begin(),
                        leftCommands.end(),
                        [](
                            const MonitorCommand* lhs,
                            const MonitorCommand* rhs)
                        {
                            return lhs->order() <
                                   rhs->order();
                        });

                const auto rightMinimum =
                    std::min_element(
                        rightCommands.begin(),
                        rightCommands.end(),
                        [](
                            const MonitorCommand* lhs,
                            const MonitorCommand* rhs)
                        {
                            return lhs->order() <
                                   rhs->order();
                        });

                const int leftOrder =
                    (*leftMinimum)->order();

                const int rightOrder =
                    (*rightMinimum)->order();

                if (leftOrder != rightOrder)
                    return leftOrder < rightOrder;

                return left < right;
            });

        std::cout << "Available commands:\n";

        for (const std::string& category :
             categories)
        {
            auto& categoryCommands =
                groupedCommands[category];

            std::sort(
                categoryCommands.begin(),
                categoryCommands.end(),
                [](
                    const MonitorCommand* left,
                    const MonitorCommand* right)
                {
                    if (left->order() !=
                        right->order())
                    {
                        return left->order() <
                               right->order();
                    }

                    return left->name() <
                           right->name();
                });

            std::cout
                << "  "
                << category
                << ":\n";

            for (const MonitorCommand* command :
                 categoryCommands)
            {
                std::cout
                    << "    "
                    << command->shortHelp()
                    << "\n";
            }
        }

        return;
    }

    const auto commandIterator = commands.find(cmd);

    if (commandIterator != commands.end())
    {
        commandIterator->second->execute(
            *this,
            args);

        return;
    }

    std::cout
        << "Unknown command: "
        << cmd
        << "\n";
}

void MLMonitor::writeOutputFileBlock(const std::string& cmdLine, const std::string& output)
{
    if (!outputFileEnabled || !outputFile.is_open())
        return;

    outputFile << "> " << cmdLine << "\n";

    if (!output.empty())
    {
        outputFile << output;

        if (output.back() != '\n')
            outputFile << "\n";
    }

    outputFile << "\n";
    outputFile.flush();
}

void MLMonitor::queueAsyncLine(const std::string& s)
{
    std::lock_guard<std::mutex> lock(asyncMutex);
    asyncLines.push_back(s);
}

void MLMonitor::handleOutputFileCommand(const std::vector<std::string>& args)
{
    if (args.size() < 2)
    {
        std::cout <<
            "Usage:\n"
            "  out on <file>\n"
            "  out off\n"
            "  out status\n";
        return;
    }

    std::string sub = args[1];
    std::transform(sub.begin(), sub.end(), sub.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (sub == "on")
    {
        std::string path = "mlmonitor_output.txt";

        if (args.size() >= 3)
            path = args[2];

        // Close previous file if one is already open.
        if (outputFile.is_open())
            outputFile.close();

        outputFile.open(path, std::ios::out | std::ios::app);

        if (!outputFile.is_open())
        {
            outputFileEnabled = false;
            outputFilePath.clear();
            std::cout << "Unable to open output file: " << path << "\n";
            return;
        }

        outputFileEnabled = true;
        outputFilePath = path;

        std::cout << "Monitor output file enabled: " << outputFilePath << "\n";
        return;
    }

    if (sub == "off")
    {
        if (outputFile.is_open())
            outputFile.close();

        outputFileEnabled = false;

        std::cout << "Monitor output file disabled";

        if (!outputFilePath.empty())
            std::cout << ": " << outputFilePath;

        std::cout << "\n";
        return;
    }

    if (sub == "status")
    {
        if (outputFileEnabled && outputFile.is_open())
            std::cout << "Monitor output file is ON: " << outputFilePath << "\n";
        else
            std::cout << "Monitor output file is OFF\n";

        return;
    }

    std::cout << "Unknown out command: " << sub << "\n";
}


// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef MLMONITOR_H
#define MLMONITOR_H

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "Debug/MLMonitorBackend.h"
#include "Debug/MonitorCommand.h"

class MLMonitor
{
    public:
        MLMonitor();
        virtual ~MLMonitor();

        inline void attachMLMonitorBackendInstance(MLMonitorBackend* mlmonitorBackend) { this->mlmonitorBackend = mlmonitorBackend; }
        inline MLMonitorBackend* getMLMonitorBackend() const { return mlmonitorBackend; }

        std::string executeAndCapture(const std::string& cmdLine);

        inline void setRunningFlag(bool flag) { running = flag; }
        inline bool getRunningFlag() const { return running; }

        void enterMonitor();
        std::string getPrompt() const;

        std::vector<std::string> drainAsyncLines();

    protected:

    private:
        // Non-owning pointers
        MLMonitorBackend* mlmonitorBackend;

        bool running;

        // std::cout queue
        std::mutex asyncMutex;
        std::vector<std::string> asyncLines;

        // Console output to file
        std::ofstream outputFile;
        std::string outputFilePath;
        bool outputFileEnabled;

        std::unordered_map<std::string, std::unique_ptr<MonitorCommand>> commands;

        // Command registration
        inline void registerCommand(std::unique_ptr<MonitorCommand> cmd) { commands[cmd->name()] = std::move(cmd); }

        // Process commands
        void handleCommand(const std::string& line);
        void handleOutputFileCommand(const std::vector<std::string>& args);
        void writeOutputFileBlock(const std::string& cmdLine, const std::string& output);
};

#endif // MLMONITOR_H

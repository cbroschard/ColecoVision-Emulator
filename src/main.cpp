// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>

namespace po = boost::program_options;

po::options_description get_config_file_options()
{
    po::options_description desc("Configuration File Options");
    desc.add_options()
        ("BIOS", po::value<std::string>()->required(), "ColecoVision BIOS");
               ;
    return desc;
}

int main()
{
    // Process configuration file, exit if there are any errors as we won't know how to boot the system
    std::ifstream configFile("colecovision.cfg");
    if (!configFile)
    {
        std::cerr << "Error: Unable to open configuration file colecovision.cfg exiting!" << std::endl;
        return 1;
    }
    return 0;
}

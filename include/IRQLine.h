// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef IRQLINE_H
#define IRQLINE_H

#include <cstdint>

enum class IRQSource : uint8_t
{
    VDP = 0,
    Expansion,
    Count
};

class IRQLine
{
    public:
        IRQLine();
        virtual ~IRQLine();

        void reset();

        void raiseIRQ(IRQSource source);
        void clearIRQ(IRQSource source);

        bool isAsserted() const;

    protected:

    private:
        uint8_t activeSources = 0;
};

#endif // IRQLINE_H

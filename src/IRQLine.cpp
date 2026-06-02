// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "IRQLine.h"

IRQLine::IRQLine()
{

}

IRQLine::~IRQLine() = default;

void IRQLine::reset()
{
    activeSources = 0;
}

void IRQLine::raiseIRQ(IRQSource source)
{
    activeSources |= (1u << static_cast<uint8_t>(source));
}

void IRQLine::clearIRQ(IRQSource source)
{
    activeSources &= ~(1u << static_cast<uint8_t>(source));
}

bool IRQLine::isAsserted() const
{
    return activeSources != 0;
}

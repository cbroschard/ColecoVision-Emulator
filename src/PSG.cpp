#include "PSG.h"

PSG::PSG()
{
    reset();
}

PSG::~PSG() = default;

void PSG::reset()
{
    tonePeriod[0] = 0;
    tonePeriod[1] = 0;
    tonePeriod[2] = 0;

    volume[0] = 0x0F;
    volume[1] = 0x0F;
    volume[2] = 0x0F;
    volume[3] = 0x0F;

    noiseControl = 0;
    latchedRegister = 0;
}

void PSG::write(uint8_t value)
{
    if (value & 0x80)
    {
        // Latch/data byte: 1 rrr dddd
        latchedRegister = (value >> 4) & 0x07;
        uint8_t data = value & 0x0F;

        switch (latchedRegister)
        {
            case 0:
                tonePeriod[0] = (tonePeriod[0] & 0x3F0) | data;
                break;

            case 1:
                volume[0] = data & 0x0F;
                break;

            case 2:
                tonePeriod[1] = (tonePeriod[1] & 0x3F0) | data;
                break;

            case 3:
                volume[1] = data & 0x0F;
                break;

            case 4:
                tonePeriod[2] = (tonePeriod[2] & 0x3F0) | data;
                break;

            case 5:
                volume[2] = data & 0x0F;
                break;

            case 6:
                noiseControl = data & 0x0F;
                break;

            case 7:
                volume[3] = data & 0x0F;
                break;
        }
    }
    else
    {
        // Data byte for previously latched register.
        uint8_t data = value & 0x3F;

        switch (latchedRegister)
        {
            case 0:
                tonePeriod[0] = (tonePeriod[0] & 0x00F) | (data << 4);
                break;

            case 2:
                tonePeriod[1] = (tonePeriod[1] & 0x00F) | (data << 4);
                break;

            case 4:
                tonePeriod[2] = (tonePeriod[2] & 0x00F) | (data << 4);
                break;

            case 1:
                volume[0] = value & 0x0F;
                break;

            case 3:
                volume[1] = value & 0x0F;
                break;

            case 5:
                volume[2] = value & 0x0F;
                break;

            case 6:
                noiseControl = value & 0x0F;
                break;

            case 7:
                volume[3] = value & 0x0F;
                break;
        }
    }
}

#include "PSG.h"

PSG::PSG() :
    noiseControl(0),
    latchedRegister(0),
    noiseCounter(0),
    noiseOutput(false),
    noiseLfsr(0x8000)
{
    reset();
}

PSG::~PSG() = default;

void PSG::reset()
{
    tonePeriod[0]   = 0;
    tonePeriod[1]   = 0;
    tonePeriod[2]   = 0;

    volume[0]       = 0x0F;
    volume[1]       = 0x0F;
    volume[2]       = 0x0F;
    volume[3]       = 0x0F;

    toneCounter[0]  = 0;
    toneCounter[1]  = 0;
    toneCounter[2]  = 0;

    noiseCounter    = 0;
    noiseOutput     = 0;
    noiseLfsr       = 0x8000;
    noiseControl    = 0;

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

float PSG::sample()
{
    constexpr double clocksPerSample = PSG_CLOCK / SAMPLE_RATE;

    float mix = 0.0f;

    for (int ch = 0; ch < 3; ++ch)
    {
        uint16_t period = tonePeriod[ch];

        if (period == 0)
            period = 1;

        toneCounter[ch] -= clocksPerSample;

        while (toneCounter[ch] <= 0.0)
        {
            toneCounter[ch] += static_cast<double>(period) * 16.0;
            toneOutput[ch] = !toneOutput[ch];
        }

        const float amp = volumeToFloat(volume[ch]);
        mix += toneOutput[ch] ? amp : -amp;
    }

    uint16_t noisePeriod = 512;

    switch (noiseControl & 0x03)
    {
        case 0: noisePeriod = 512; break;
        case 1: noisePeriod = 1024; break;
        case 2: noisePeriod = 2048; break;
        case 3:
        {
            noisePeriod = tonePeriod[2];
            if (noisePeriod == 0)
                noisePeriod = 1;
            noisePeriod *= 16;
            break;
        }
    }

    noiseCounter -= clocksPerSample;

    while (noiseCounter <= 0.0)
    {
        noiseCounter += noisePeriod;
        stepNoise();
    }

    const float noiseAmp = volumeToFloat(volume[3]);
    mix += noiseOutput ? noiseAmp : -noiseAmp;

    return mix * 0.20f;
}

float PSG::volumeToFloat(uint8_t v) const
{
    static constexpr float table[16] =
    {
        1.0000f,
        0.7943f,
        0.6310f,
        0.5012f,
        0.3981f,
        0.3162f,
        0.2512f,
        0.1995f,
        0.1585f,
        0.1259f,
        0.1000f,
        0.0794f,
        0.0631f,
        0.0501f,
        0.0398f,
        0.0000f
    };

    return table[v & 0x0F];
}

void PSG::stepNoise()
{
    const bool whiteNoise = (noiseControl & 0x04) != 0;

    uint16_t feedback = 0;

    if (whiteNoise)
        feedback = (noiseLfsr ^ (noiseLfsr >> 1)) & 0x01;
    else
        feedback = noiseLfsr & 0x01;

    noiseLfsr >>= 1;

    if (feedback)
        noiseLfsr |= 0x8000;

    noiseOutput = (noiseLfsr & 0x01) != 0;
}

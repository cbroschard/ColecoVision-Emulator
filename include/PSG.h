#ifndef PSG_H
#define PSG_H

#include <cstdint>

class PSG
{
    public:
        PSG();
        virtual ~PSG();

        void reset();

        void write(uint8_t value);

        float sample();

    protected:

    private:
        static constexpr double PSG_CLOCK = 3579545.0;
        static constexpr double SAMPLE_RATE = 44100.0;

        uint16_t tonePeriod[3];   // channels 0, 1, 2
        uint8_t volume[4];        // channels 0, 1, 2, noise

        uint8_t noiseControl;     // noise register
        uint8_t latchedRegister;  // last selected register

        double toneCounter[3]{};
        bool toneOutput[3]{};

        double noiseCounter;
        bool noiseOutput;
        uint16_t noiseLfsr;

        float volumeToFloat(uint8_t v) const;
        void stepNoise();
};

#endif // PSG_H

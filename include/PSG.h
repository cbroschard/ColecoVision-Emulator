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

    protected:

    private:
        uint16_t tonePeriod[3];   // channels 0, 1, 2
        uint8_t volume[4];        // channels 0, 1, 2, noise

        uint8_t noiseControl;     // noise register
        uint8_t latchedRegister;  // last selected register
};

#endif // PSG_H

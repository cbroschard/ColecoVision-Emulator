#include "CPU.h"

CPU::CPU() :
    mem(nullptr),
    A(0),
    F(0),
    B(0),
    C(0),
    D(0),
    E(0),
    H(0),
    L(0),
    A_(0),
    F_(0),
    B_(0),
    C_(0),
    D_(0),
    E_(0),
    H_(0),
    L_(0),
    IX(0),
    IY(0),
    PC(0),
    SP(0),
    I(0),
    R(0),
    IFF1(false),
    IFF2(false),
    IM(0),
    halted(false),
    irqPending(false),
    nmiPending(false),
    eiDelay(false),
    cycles(0)
{

}

CPU::~CPU()
{

}

void CPU::reset()
{
    // Main registers
    A       = 0;
    F       = 0;

    B       = 0;
    C       = 0;
    D       = 0;
    E       = 0;
    H       = 0;
    L       = 0;

    // Shadow registers
    A_      = 0;
    F_      = 0;

    B_      = 0;
    C_      = 0;
    D_      = 0;
    E_      = 0;
    H_      = 0;
    L_      = 0;

    // Index registers
    IX      = 0;
    IY      = 0;

    // Program Counter + Stack Pointer
    PC = 0x0000;
    SP = 0xFFFF;

    // Interrupts
    I       = 0;
    R       = 0;
}

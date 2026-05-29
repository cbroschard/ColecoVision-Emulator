# ColecoVision Emulator

A work-in-progress ColecoVision emulator written in C++.

This project is currently in the early foundation stage. The goal is to build a reasonably accurate ColecoVision emulator with a Z80 CPU core, ColecoVision memory map, cartridge loading, video/audio/input support, and eventually full game compatibility.

## Current Status

Implemented so far:

- Initial Z80 CPU class structure
- Z80 register layout
- Z80 flag definitions
- Base opcode cycle table
- CB / ED / DD / FD prefixed cycle tables
- DD CB / FD CB indexed bit-operation cycle tables
- Initial ColecoVision memory class
- BIOS ROM storage area
- 1 KB system RAM storage
- Cartridge pointer support
- Basic reset structure

Still to be implemented:

- CPU instruction execution
- Opcode handlers
- Memory read/write mapping
- BIOS loading
- Cartridge loading
- VDP emulation
- SN76489 sound chip emulation
- Controller/keypad input
- Main emulator loop
- ROM loading UI or command-line support

## Target System

The ColecoVision uses:

- CPU: Z80-compatible processor
- CPU clock: approximately 3.58 MHz
- RAM: 1 KB system RAM
- BIOS ROM: 8 KB
- Video: Texas Instruments TMS9918A-style VDP
- Sound: SN76489-compatible PSG
- Cartridge ROM mapped into the upper address space

## Memory Map

Initial memory map target:

```text
0x0000 - 0x1FFF   BIOS ROM, 8 KB
0x2000 - 0x5FFF   Expansion / unused area for now
0x6000 - 0x7FFF   1 KB RAM mirrored through this range
0x8000 - 0xFFFF   Cartridge ROM

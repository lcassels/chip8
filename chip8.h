#ifndef CPU_H
#define CPU_H

#include "gpu.h"
#include <SDL2/SDL.h>      // SDL2
#include <string>
using namespace std;

class Chip8
{
private:
  // 2 byte opcode
  unsigned short opcode;

  /* 4k memory
  0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
  0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
  0x200-0xFFF - Program ROM and work RAM */
  unsigned char memory[4096];

  // 8 bit registers (16th is carry)
  unsigned char V[16];

  // index register and program counter (both 0x000 - 0xFFF)
  unsigned short I;
  unsigned short pc;

  // 64 x 32 black and white display
  unsigned char gfx[64 * 32];

  // interrupts - when set above zero, count to zero
  unsigned char delay_timer;
  unsigned char sound_timer;

  // stack
  unsigned short stack[16];
  unsigned short sp;

  // hex-based keypad
  unsigned char keypad[16];

  // testing function
  void runOpcode(unsigned short op);

  // debug function
  void unknownOpcode();
 
public:
  bool drawFlag;

  void initialize();
  bool emulateCycle();
  void render(Gpu gpu);
  void loadGame(string name);
  void setKeys();
  void debugRender();
  void shutdown();

  void selfTest();
};
 
#endif
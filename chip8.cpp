#include "chip8.h"
#include "gpu.h"
#include <algorithm>    // fill
#include <cassert>      // assert
#include <fstream>
#include <iostream>     // cout
#include <SDL.h>        // SDL2
#include <stdio.h>      // printf, NULL
#include <stdlib.h>     // srand, rand
#include <string>
#include <time.h>       // time
using namespace std;

unsigned char chip8Fontset[80] =
{ 
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void Chip8::initialize()
{
  // Initialize random seed
  srand(time(NULL));

  pc     = 0x200;  // Program counter starts at 0x200
  opcode = 0;      // Reset current opcode  
  I      = 0;      // Reset index register
  sp     = 0;      // Reset stack pointer

  // Clear display
  fill(gfx, gfx + sizeof(gfx), 0);

  // Clear stack
  fill(stack, stack + sizeof(stack), 0);
  // Clear registers V0-VF
  fill(V, V + sizeof(V), 0);
  // Clear memory
  fill(memory, memory + sizeof(memory), 0);
 
  // Load fontset
  for(int i = 0; i < 80; ++i){
    memory[i] = chip8Fontset[i];
  }

  // Reset timers
  delay_timer = 0;
  sound_timer = 0;
};

void Chip8::unknownOpcode(){
  if (opcode != 0x0000){ // Empty bytes in memory after program finishes
    printf("Unknown opcode 0x%.4X\n", opcode);
  }
};

bool Chip8::emulateCycle(){
  //Fetch opcode
  if (pc >= 4096)
    return false;
  opcode = memory[pc] << 8 | memory[pc + 1];
  drawFlag = false;

  //Decode opcode
  switch(opcode & 0xF000){
    case 0x0000:
      switch(opcode & 0x00FF){
      case 0x00E0: // 0x00E0: Clears the screen        
        fill(gfx, gfx + sizeof(gfx), 0);
        drawFlag = true;
        pc += 2;
        break;
 
      case 0x00EE: // 0x00EE: Returns from subroutine          
        sp--;
        pc = stack[sp] + 2;
        break;

      default: // 0000 or 0NNN (Hopefully we don't need either...)
        unknownOpcode();
        pc += 2;
        break;
      }
      break;

    case 0x1000: // 1NNN: Jumps to address NNN
      pc = opcode & 0x0FFF;
      break;

    case 0x2000: // 2NNN: Calls subroutine at address NNN
      stack[sp] = pc;
      sp++;
      pc = opcode & 0x0FFF;
      break;

    case 0x3000: // 3XNN: Skips the next instruction if VX equals NN
      if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
        pc += 4;
      else
        pc += 2;
      break;

    case 0x4000: // 4XNN: Skips the next instruction if VX doesn't equal NN
      if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
        pc += 4;
      else
        pc += 2;
      break;

    case 0x5000:
      switch (opcode & 0xF00F){ // Check last byte
        case 0x5000: // 5XY0: Skips the next instruction if VX equals VY
          if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
            pc += 4;
          else
            pc += 2;
          break;

        default:
          unknownOpcode();
          pc += 2;
          break;
      }
      break;

    case 0x6000: // 6XNN: Sets VX to NN
      V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
      pc += 2;
      break;

    case 0x7000: // 7XNN: Adds NN to VX
      V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
      pc += 2;
      break;

    case 0x8000:
      switch (opcode & 0xF00F){
        case 0x8000: // 8XY0: Sets VX to the value of VY
          V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
          pc += 2;
          break;

        case 0x8001: // 8XY1: Sets VX to VX or VY
          V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
          pc += 2;
          break;

        case 0x8002: // 8XY2: Sets VX to VX and VY
          V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
          pc += 2;
          break;

        case 0x8003: // 8XY3: Sets VX to VX xor VY
          V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
          pc += 2;
          break;

        // 8XY4:  Adds VY to VX. VF is set to 1 when there's a carry, and to 0
        // when there isn't
        case 0x8004:
          if(V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
            V[0xF] = 1; //carry
          else
            V[0xF] = 0;
          V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
          pc += 2;          
          break;

        // 8XY5: VY is subtracted from VX. VF is set to 0 when there's a borrow,
        // and 1 when there isn't
        case 0x8005:
          if(V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
            V[0xF] = 0; //borrow
          else
            V[0xF] = 1;
          V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
          pc += 2;          
          break;

        // 8XY6: Shifts VX right by one. VF is set to the value of the least
        // significant bit of VX before the shift
        case 0x8006:
          V[0xF] = V[(opcode & 0x0F00) >> 8] & 1;
          V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] >> 1;
          pc += 2;
          break;

        // 8XY7: Sets VX to VY minus VX. VF is set to 0 when there's a borrow,
        // and 1 when there isn't
        case 0x8007:
          if(V[(opcode & 0x00F0) >> 4] < V[(opcode & 0x0F00) >> 8])
            V[0xF] = 0; //borrow
          else
            V[0xF] = 1;
          V[(opcode & 0x0F00) >> 8] =
            V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
          pc += 2;          
          break;

        // 8XYE: Shifts VX left by one. VF is set to the value of the most
        // significant bit of VX before the shift
        case 0x800E:
          V[0xF] = (V[(opcode & 0x0F00) >> 8] & (1 << 7)) >> 7;
          V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] << 1;
          pc += 2;
          break;

        default:
          unknownOpcode();
          pc += 2;
          break;
      }
      break;

    case 0x9000:
      switch (opcode & 0xF00F){ // check last byte
        case 0x9000: // 9XY0: Skips the next instruction if VX doesn't equal VY
          if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
            pc += 4;
          else
            pc += 2;
          break;

        default:
          unknownOpcode();
          pc += 2;
          break;
      }
      break;

    case 0xA000: // ANNN: Sets I to the address NNN
      I = opcode & 0x0FFF;
      pc += 2;
      break;

    case 0xB000: // BNNN: Jumps to the address NNN plus V0
      pc = (opcode & 0x0FFF) + V[0];
      break;

    // CXNN: Sets VX to the result of a bitwise and operation on a random number
    // and NN
    case 0xC000:
      V[(opcode & 0x0F00) >> 8] = (rand() % 256) & (opcode & 0x00FF);
      pc += 2;
      break;

    /* DXYN: Sprites stored in memory at location in index register (I), 8bits
       wide. Wraps around the screen. If when drawn, clears a pixel, register VF
       is set to 1 otherwise it is zero. All drawing is XOR drawing (i.e. it
       toggles the screen pixels). Sprites are drawn starting at position VX,
       VY. N is the number of 8bit rows that need to be drawn. If N is greater
       than 1, second line continues at position VX, VY+1, and so on. */
    case 0xD000: {
      unsigned short x = V[(opcode & 0x0F00) >> 8];
      unsigned short y = V[(opcode & 0x00F0) >> 4];
      unsigned short height = opcode & 0x000F;
      unsigned short pixel;
     
      V[0xF] = 0;
      for (int yline = 0; yline < height; yline++)
      {
        pixel = memory[I + yline];
        for (int xline = 0; xline < 8; xline++)
        {
          if ((pixel & (0x80 >> xline)) != 0)
          {
            if (gfx[(x + xline + ((y + yline) * 64))] == 1)
              V[0xF] = 1;                                 
            gfx[x + xline + ((y + yline) * 64)] ^= 1;
          }
        }
      }

      drawFlag = true;
      pc += 2;
      break;
    }

    case 0xE000:
      switch (opcode & 0xF0FF){
        // EX9E: Skips the next instruction if the key stored in VX is pressed
        case 0xE09E:
          if (keypad[V[(opcode & 0x0F00) >> 8]] == 1)
            pc += 4;
          else
            pc += 2;
          break;

        // EXA1: Skips the next instruction if the key stored in VX isn't
        // pressed
        case 0xE0A1:
          if (keypad[V[(opcode & 0x0F00) >> 8]] == 0)
            pc += 4;
          else
            pc += 2;
          break;

        default:
          unknownOpcode();
          pc += 2;
          break;
      }
      break;

    case 0xF000:
      switch(opcode & 0xF0FF){
        case 0xF007: // FX07: Sets VX to the value of the delay timer
          V[(opcode & 0x0F00) >> 8] = delay_timer;
          pc += 2;
          break;

        case 0xF00A: // FX0A: A key press is awaited, and then stored in VX
          for (unsigned char i = 0; i < 16; i++){
            if (keypad[i] == 1){
              V[(opcode & 0x0F00) >> 8] = i;
              pc += 2;
              break;
            }
          }
          break;

        case 0xF015: // FX15: Sets the delay timer to VX
          delay_timer = V[(opcode & 0x0F00) >> 8] + 1;
          pc += 2;
          break;

        case 0xF018: // FX18: Sets the sound timer to VX
          sound_timer = V[(opcode & 0x0F00) >> 8] + 1;
          pc += 2;
          break;

        case 0xF01E: // FX1E: Adds VX to I
          I += V[(opcode & 0x0F00) >> 8];
          pc += 2;
          break;

        // FX29: Sets I to the location of the sprite for the character in VX.
        // Characters 0-F (in hexadecimal) are represented by a 4x5 font
        case 0xF029:
          I = ((opcode & 0x0F00) >> 8) * 5;
          pc += 2;
          break;

        // FX33: Stores the Binary-coded decimal representation of VX, with the
        // most significant of three digits at the address in I, the middle
        // digit at I plus 1, and the least significant digit at I plus 2. (In
        // other words, take the decimal representation of VX, place the
        // hundreds digit in memory at location in I, the tens digit at location
        // I+1, and the ones digit at location I+2.)
        case 0xF033:
          memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
          memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
          memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
          pc += 2;
          break;

        // FX55: Stores V0 to VX (including VX) in memory starting at address I
        case 0xF055:
          for (unsigned char i = 0; i <= (opcode & 0x0F00) >> 8; i++){
            memory[I + i] = V[i];
          }
          pc += 2;
          break;

        // FX65: Fills V0 to VX (including VX) with values from memory starting
        // at address I
        case 0xF065:
          for (unsigned char i = 0; i <= (opcode & 0x0F00) >> 8; i++){
            V[i] = memory[I + i];
          }
          pc += 2;
          break;
      }
      break;

    default:
      unknownOpcode();
      pc += 2;
      break;
  }

  if(delay_timer > 0){
    --delay_timer;
  }
 
  if(sound_timer > 0){
    if(sound_timer == 1)
      printf("BEEP!\n");
    --sound_timer;
  }

  return true;
};

void Chip8::render(Gpu gpu){
  gpu.render(gfx);
};

void Chip8::loadGame(string name){
  ifstream file;
  file.open(name, ios::in|ios::binary|ios::ate);
  assert(file.is_open());

  streampos size;
  size = file.tellg();
  printf("Loading rom '%s' (%i bytes)...\n", name.c_str(), int(size));

  file.seekg (0, ios::beg);
  char * memblock;

  for (int i = 0; file.tellg() != size; i++) {
    memblock = new char;
    file.read(memblock, 1);
    memory[i + 0x200] = (unsigned char)(*memblock);
  }

  file.close();
};

/* Keymapping:

Keypad                 Keyboard
1 2 3 C                1 2 3 4
4 5 6 D                Q W E R
7 8 9 E                A S D F
A 0 B F                Z X C V

keymap[x] = keypad[x] - so keymap[0xA] is the scancode for button A
(scancodes: https://wiki.libsdl.org/SDL_Scancode) */

Uint8 keymap[16] =
{
  SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, // 0-3
  SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A, // 4-7
  SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C, // 8-B
  SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V  // C-F
};


void Chip8::setKeys(){
  // SDL keypress states (updated by event loop in main.cpp)
  const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);

  for(unsigned char i = 0; i < 16; i++){
    if(currentKeyStates[keymap[i]])
      keypad[i] = 1;
    else
      keypad[i] = 0;
  }
  keypad[5] = 1;
};


void Chip8::shutdown(){
  //quit program
  exit(0);
};


void Chip8::debugRender(){
  cout << "+";
  for(unsigned char i = 0; i < 64; i++){
    cout << "-";
  }
  cout << "+\n";

  for(unsigned char y = 0; y < 32; y++){
    cout << "|";
    for(unsigned char x = 0; x < 64; x++){
      if (gfx[y * 64 + x] == 1)
        cout << "#";
      else
        cout << " ";
    }
    cout << "|\n";
  }

  cout << "+";
  for(unsigned char i = 0; i < 64; i++){
    cout << "-";
  }
  cout << "+\n";
  cout.flush();
};


void Chip8::runOpcode(unsigned short op){
  memory[pc] = (op & 0xFF00) >> 8;
  memory[pc + 1] = op & 0x00FF;
  emulateCycle();
};


void Chip8::selfTest(){
  printf("Running unit tests...\n");

  // 00EE: Return from subroutine
  initialize();
  sp = 5;
  stack[4] = 0x400;
  runOpcode(0x00EE);
  assert(sp == 4);
  assert(pc == 0x402);

  // 1NNN: Jump to address NNN
  runOpcode(0x1765);
  assert(pc == 0x765);

  // 2NNN: Calls subroutine at NNN
  initialize();
  runOpcode(0x2345);
  assert(pc == 0x345);
  assert(sp == 1);
  assert(stack[0] == 0x200);

  // 3XNN: Skips the next instruction if VX equals NN
  initialize();
  V[5] = 0xDC;
  runOpcode(0x35DC);
  assert(pc = 0x204);
  runOpcode(0x3511);
  assert(pc = 0x206);

  // 4XNN: Skips the next instruction if VX doesn't equal NN
  initialize();
  V[5] = 0xDC;
  runOpcode(0x45DC);
  assert(pc = 0x202);
  runOpcode(0x4511);
  assert(pc = 0x206);

  // 5XY0: Skips the next instruction if VX equals VY
  initialize();
  V[8] = 0xCC; V[6] = 0xBB; V[5] = 0xCC;
  runOpcode(0x5860);
  assert(pc == 0x202);
  runOpcode(0x5580);
  assert(pc == 0x206);
  runOpcode(0x5660);
  assert(pc == 0x20A);

  // 6XNN: Sets VX to NN
  initialize();
  runOpcode(0x63BC);
  assert(V[3] = 0xBC);

  // 7XNN: Adds NN to VX
  initialize();
  V[5] = 58;
  runOpcode(0x75AB);
  assert(V[5] = 229);

  // 8XY0: Sets the value of VX to the value of VY
  initialize();
  V[2] = 0xAF;
  runOpcode(0x8920);
  assert(V[9] = 0xAF);

  // 8XY1: Sets VX to VX or VY
  initialize();
  V[7] = 0xDE;
  V[4] = 0x47;
  runOpcode(0x8741);
  assert(V[7] == (0xDE | 0x47));

  // 8XY2: Sets VX to VX and VY
  initialize();
  V[7] = 0xDE;
  V[4] = 0x47;
  runOpcode(0x8742);
  assert(V[7] == (0xDE & 0x47));

  // 8XY3: Sets VX to VX xor VY
  initialize();
  V[7] = 0xDE;
  V[4] = 0x47;
  runOpcode(0x8743);
  assert(V[7] == (0xDE ^ 0x47));

  // 8XY4:  Adds VY to VX. VF is set to 1 when there's a carry, and to 0
  // when there isn't
  initialize();
  V[7] = 128; V[8] = 128;
  runOpcode(0x8874);
  assert(V[0xF] == 1);
  assert(V[8] == 0);
  V[7] = 79; V[8] = 115;
  runOpcode(0x8874);
  assert(V[0xF] == 0);
  assert(V[8] == 79 + 115);

  // 8XY5: VY is subtracted from VX. VF is set to 0 when there's a borrow,
  // and 1 when there isn't
  initialize();
  V[7] = 128; V[8] = 128;
  runOpcode(0x8875);
  assert(V[0xF] == 1);
  assert(V[8] == 0);
  V[7] = 200; V[8] = 100;
  runOpcode(0x8875);
  assert(V[0xF] == 0);
  assert(V[8] == 156);

  // 8XY6: Shifts VX right by one. VF is set to the value of the least
  // significant bit of VX before the shift
  initialize();
  V[2] = 0b01010101;
  runOpcode(0x82F6);
  assert(V[2] == 0b00101010);
  assert(V[0xF] == 1);
  runOpcode(0x82F6);
  assert(V[2] == 0b00010101);
  assert(V[0xF] == 0);

  // 8XY7: Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1
  // when there isn't
  initialize();
  V[7] = 128; V[8] = 128;
  runOpcode(0x8877);
  assert(V[0xF] == 1);
  assert(V[8] == 0);
  V[7] = 100; V[8] = 200;
  runOpcode(0x8877);
  assert(V[0xF] == 0);
  assert(V[8] == 156);

  // 8XYE: Shifts VX left by one. VF is set to the value of the most significant
  // bit of VX before the shift
  initialize();
  V[2] = 0b10101010;
  runOpcode(0x82FE);
  assert(V[2] == 0b01010100);
  assert(V[0xF] == 1);
  runOpcode(0x82FE);
  assert(V[2] == 0b10101000);
  assert(V[0xF] == 0);

  // 9XY0: Skips the next instruction if VX doesn't equal VY
  initialize();
  V[4] = 34; V[1] = 54;
  runOpcode(0x9410);
  assert(pc == 0x204);
  V[1] = 34;
  runOpcode(0x9410);
  assert(pc == 0x206);

  // ANNN: Set I to NNN
  initialize();
  runOpcode(0xAABC);
  assert(I == 0xABC);

  // BNNN: Jumps to the address NNN plus V0
  V[0] = 0x43;
  runOpcode(0xB2BB);
  assert(pc == 0x43 + 0x2BB);

  // CXNN: Sets VX to the result of a bitwise and operation on a random number
  // and NN
  initialize();
  runOpcode(0xC300 | 0b10101010);
  assert((V[3] & 0b01010101) == 0);

  // DXYN: idk how to test this

  // EX9E: Skips the next instruction if the key stored in VX is pressed
  initialize();
  V[4] = 0xE; keypad[0xE] = 1;
  runOpcode(0xE49E);
  assert(pc == 0x204);
  keypad[0xE] = 0;
  runOpcode(0xE49E);
  assert(pc == 0x206);

  // EXA1: Skips the next instruction if the key stored in VX isn't pressed
  initialize();
  V[4] = 0xE; keypad[0xE] = 1;
  runOpcode(0xE4A1);
  assert(pc == 0x202);
  keypad[0xE] = 0;
  runOpcode(0xE4A1);
  assert(pc == 0x206);

  // FX07: Sets VX to the value of the delay timer
  initialize();
  delay_timer = 120;
  runOpcode(0xF207);
  assert(V[2] == 120);

  // FX0A: A key press is awaited, and then stored in VX
  initialize();
  runOpcode(0xF60A);
  assert(pc == 0x200);
  assert(V[6] == 0);
  keypad[0xC] = 1;
  runOpcode(0xF60A);
  assert(pc == 0x202);
  assert(V[6] == 0xC);
  keypad[0xC] = 0;

  // FX15: Sets the delay timer to VX
  initialize();
  V[8] = 123;
  runOpcode(0xF815);
  assert(delay_timer == 123);

  // FX18: Sets the sound timer to VX
  initialize();
  V[8] = 123;
  runOpcode(0xF818);
  assert(sound_timer == 123);

  // FX1E: Adds VX to I
  initialize();
  I = 23; V[5] = 149;
  runOpcode(0xF51E);
  assert(I = 23 + 149);

  // FX29: Sets I to the location of the sprite for the character in VX.
  // Characters 0-F (in hexadecimal) are represented by a 4x5 font
  initialize();
  runOpcode(0xF829);
  assert(I == 40);

  // FX33: Stores the Binary-coded decimal representation of VX
  // no idea how to test this

  // FX55: Stores V0 to VX (including VX) in memory starting at address I
  initialize();
  I = 0x300;
  V[0] = 0xAB; V[4] = 0xCB; V[5] = 0xDB;
  runOpcode(0xF455);
  assert(memory[I] == 0xAB);
  assert(memory[I + 4] == 0xCB);
  assert(memory[I + 5] == 0);

  // FX65: Fills V0 to VX (including VX) with values from memory starting
  // at address I
  initialize();
  I = 0x300;
  memory[I + 0] = 0xAB; memory[I + 4] = 0xCB; memory[I + 5] = 0xDB;
  runOpcode(0xF465);
  assert(V[0] == 0xAB);
  assert(V[4] == 0xCB);
  assert(V[5] == 0);

  printf("Completed successfully\n\n");
};

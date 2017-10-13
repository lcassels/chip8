#include "chip8.h"
#include <cstdlib>        // exit
#include <SDL.h>          // SDL2
 
int main(int argc, char **argv)
{
  if (argc != 2){
    printf("Incorrect arguments. Please run as: ./main rom/path\n");
    std::exit(0);
  }

  // ops per second, chip8 has no standard but this seems to make things run
  // at a nice speed
  const double hz = 500;
  Chip8 chip8;
  Gpu gpu;

  // Run unit tests before we do anything
  chip8.selfTest();

  // Set up render system and register input callbacks
  if (not gpu.initialize())
    std::exit(0);
 
  // Initialize the Chip8 system and load the game into the memory  
  chip8.initialize();
  chip8.loadGame(argv[1]);
 
  // Emulation loop
  printf("Finished loading, now running\n");
  bool running;
  SDL_Event e;
  Uint32 tStart;
  Uint32 frameTime;
  for(;;)
  {
    tStart = SDL_GetTicks();

    // Check for an SDL quit event
    while(SDL_PollEvent(&e) != 0)
    {
      //User requests quit
      if(e.type == SDL_QUIT)
      {
        gpu.shutdown();
        chip8.shutdown();
      }
    }

    // Emulate one cycle
    running = chip8.emulateCycle();
    if (!running){
      gpu.shutdown();
      chip8.shutdown();
    }

    // If the draw flag is set, update the screen
    if(chip8.drawFlag)
      chip8.render(gpu);
 
    // Store key press state (Press and Release)
    chip8.setKeys();

    // cap operations per second
    frameTime = SDL_GetTicks() - tStart;
    if (frameTime < 1000/hz) {
      SDL_Delay((1000/hz) - frameTime);
    }
  }
 
  return 0;
}
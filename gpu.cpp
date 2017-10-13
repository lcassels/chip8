#include "gpu.h"
#include <SDL2/SDL.h>        // SDL2
#include <stdlib.h>     // srand, rand

bool Gpu::initialize(){
  // Initialization flag
  bool success = true;

  // Initialize SDL
  if(SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    success = false;
  }
  else
  {
    // Create window
    window = SDL_CreateWindow("Chip 8 Emulator", SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED, 64 * scale, 32 * scale,
      SDL_WINDOW_SHOWN);
    if(window == NULL)
    {
      printf("Window could not be created! SDL_Error: %s\n",SDL_GetError());
      success = false;
    }
    else
    {
      // Get window renderer
      renderer = SDL_CreateRenderer(window, -1, 0);
      pixels = new Uint32[64 * 32];
      renderTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STATIC, 64, 32);
    }
  }

  return success;
};

void Gpu::render(unsigned char * gfx){
  memset(pixels, 255, 64 * 32 * sizeof(Uint32));

  for(unsigned int i = 0; i < 64 * 32; i++){
    if(gfx[i] == 1)
      pixels[i] = 0;
  }

  SDL_UpdateTexture(renderTexture, NULL, pixels, 64 * sizeof(Uint32));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, renderTexture, NULL, NULL);
  SDL_RenderPresent(renderer);
};

void Gpu::shutdown(){
  delete[] pixels;

  //Deallocate surface
  SDL_DestroyTexture(renderTexture);
  renderTexture = NULL;

  //Destroy window
  SDL_DestroyWindow(window);
  window = NULL;

  SDL_DestroyRenderer(renderer);
  renderer = NULL;

  //Quit SDL subsystems
  SDL_Quit();
};

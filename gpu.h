#ifndef GPU_H
#define GPU_H

#include <SDL2/SDL.h>      // SDL2

class Gpu
{
private:
  SDL_Window* window = NULL;

  SDL_Renderer* renderer = NULL;

  SDL_Texture* renderTexture = NULL;

  Uint32 * pixels = NULL;
 
public:
	const unsigned char scale = 10;
  bool initialize();
  void render(unsigned char * gfx);
  void shutdown();
};
 
#endif

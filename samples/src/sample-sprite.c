#include <SDL3/SDL.h>

#include "SDL_gp.h"

#include "context.h"

#include "sample-sprite.h"

static Context *_context;

static SDL_GPImage image;

void
sample_sprite_setup(Context *context)
{
  _context = context;

  SDL_Surface *surface = SDL_LoadSurface("../images/sprites.png");

  if (!surface) {
    SDL_Log("Failed to load image: %s", SDL_GetError());
  }

  image = SDL_GPCreateImage(surface);

  SDL_DestroySurface(surface);
}

void
sample_sprite_render(Uint64 delta_time_ms)
{
  int window_width, window_height;
  SDL_GetWindowSize(_context->window, &window_width, &window_height);

  static int tile_width  = 32;
  static int tile_height = 32;

  static SDL_GPRect tile_region[3] = {
    { 0, 0, 32, 32 },  // tile 1
    { 32, 0, 32, 32 }, // tile 2
    { 64, 0, 32, 32 }, // tile 3
  };

  static int max_sprite = 4096;

  SDL_GPSetBlendMode(SDL_GP_BLENDMODE_BLEND);
  SDL_GPSetColor((SDL_Color){ 255, 255, 255, 255 });
  SDL_GPSetImage(0, image);

  for (int i = 0; i < max_sprite; ++i) {
    int x = SDL_rand(window_width);
    int y = SDL_rand(window_height);

    SDL_GPRect src_rect = tile_region[i % 3];
    SDL_GPRect dst_rect = { x, y, tile_width * 2, tile_height * 2 };

    SDL_GPDrawTexturedRect(0,
                           (SDL_GPTexturedRect){
                               .src = src_rect,
                               .dst = dst_rect,
                           });
  }
}

void
sample_sprite_shutdown(void)
{
  SDL_GPDestroyImage(image);
}

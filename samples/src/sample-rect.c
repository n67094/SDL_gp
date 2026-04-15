#include <SDL3/SDL.h>

#include "SDL_gp.h"

#include "context.h"

#include "sample-rect.h"

static Context *_context;

SDL_GPImage image;

void
sample_rect_setup(Context *context)
{
  _context = context;

  SDL_Surface *surface = SDL_LoadSurface("../images/hello-world.png");

  if (!surface) {
    SDL_Log("Failed to load image: %s", SDL_GetError());
  }

  image = SDL_GPCreateImage(surface);

  SDL_DestroySurface(surface);
}

void
sample_rect_render(Uint64 delta_time_ms)
{
  // Draw a red filled rectangle.
  SDL_GPSetColor((SDL_Color){ 255, 0, 0, 255 });
  {
    SDL_GPDrawFilledRect((SDL_GPRect){ 10, 10, 100, 100 });
  }

  // Draw a textured rectangle keeping it's original color.
  SDL_GPSetColor((SDL_Color){ 255, 255, 255, 255 });
  {
    SDL_GPSetImage(0, image);

    int width  = SDL_GPGetImageWidth(image);
    int height = SDL_GPGetImageHeight(image);

    SDL_GPDrawTexturedRect(
        0,
        (SDL_GPTexturedRect){
            .src = (SDL_GPRect){ 0, 0, width, height },
            .dst = (SDL_GPRect){ 120, 10, width * 2, height * 2 },
        });
  }
}

void
sample_rect_shutdown(void)
{
  SDL_GPDestroyImage(image);
}

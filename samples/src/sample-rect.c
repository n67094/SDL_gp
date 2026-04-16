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
  int window_width, window_height;
  SDL_GetWindowSize(_context->window, &window_width, &window_height);

  int hw = window_width / 2;

  // Draw a red filled rectangle.
  {
    SDL_GPViewport(0, 0, hw, window_height);
    SDL_GPSetColor((SDL_Color){ 10, 10, 10, 255 });
    SDL_GPClear();

    SDL_GPPushTransform();
    {
      SDL_GPSetColor((SDL_Color){ 255, 0, 0, 255 });

      // Move to the left area of the viewport
      SDL_GPTranslate(hw * 0.5f, window_height * 0.5f);

      float half_shape = window_width * 0.15f; // 15% of the viewport width

      SDL_GPDrawFilledRect((SDL_GPRect){
          -half_shape, -half_shape, half_shape * 2, half_shape * 2 });
    }
    SDL_GPPopTransform();
  }

  // Draw a textured rectangle keeping it's original color.
  {
    SDL_GPViewport(hw, 0, hw, window_height);
    SDL_GPSetColor((SDL_Color){ 20, 20, 20, 255 });
    SDL_GPClear();

    SDL_GPPushTransform();
    {
      SDL_GPSetColor((SDL_Color){ 255, 255, 255, 255 });

      // Move to the right area of the viewport
      SDL_GPTranslate(hw * 0.5f, window_height * 0.5f);

      SDL_GPSetImage(0, image);

      int width  = SDL_GPGetImageWidth(image);
      int height = SDL_GPGetImageHeight(image);

      int width_scale  = width * 2;
      int height_scale = height * 2;

      SDL_GPDrawTexturedRect(0,
                             (SDL_GPTexturedRect){
                                 .src = (SDL_GPRect){ 0, 0, width, height },
                                 .dst = (SDL_GPRect){ -width_scale * 0.5f,
                                                      -height_scale * 0.5f,
                                                      width_scale,
                                                      height_scale },
                             });
    }
    SDL_GPPopTransform();
  }
}

void
sample_rect_shutdown(void)
{
  SDL_GPDestroyImage(image);
}

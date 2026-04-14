#include <SDL3/SDL.h>

#include "SDL_gp.h"

#include "context.h"

#include "sample-blend.h"

static Context *_context;

static void
draw_rects(int brightness, int alpha)
{
  // Red rectangle
  SDL_GPSetColor((SDL_Color){ brightness, 0, 0, alpha });
  SDL_GPDrawRectFilled((SDL_GPRect){ 10, 10, 50, 50 });

  SDL_GPTranslate(10, 10);

  // Green rectangle
  SDL_GPSetColor((SDL_Color){ 0, brightness, 0, alpha });
  SDL_GPDrawRectFilled((SDL_GPRect){ 10, 10, 50, 50 });

  SDL_GPTranslate(10, 10);

  // Blue rectangle
  SDL_GPSetColor((SDL_Color){ 0, 0, brightness, alpha });
  SDL_GPDrawRectFilled((SDL_GPRect){ 10, 10, 50, 50 });
}

static void
draw_checkboard(int width, int height)
{
  int size = 20;
  for (int y = 0; y < height; y += size) {
    for (int x = 0; x < width; x += size) {
      bool is_white   = ((x / size) + (y / size)) % 2 == 0;
      SDL_Color color = is_white ? (SDL_Color){ 150, 150, 150, 255 }
                                 : (SDL_Color){ 50, 50, 50, 255 };
      SDL_GPSetColor(color);
      SDL_GPDrawRectFilled((SDL_GPRect){ x, y, size, size });
    }
  }
}

void
sample_blend_setup(Context *context)
{
  _context = context;
}

void
sample_blend_render(Uint64 delta_time_ms)
{
  int brightness = 255;
  int alpha      = 128;

  int window_width, window_height;
  SDL_GetWindowSize(_context->window, &window_width, &window_height);

  SDL_GPSetColor((SDL_Color){ 0, 0, 0, 255 });
  SDL_GPClear();

  SDL_GPSetBlendMode(SDL_GP_BLENDMODE_NONE);

  draw_checkboard(window_width, window_height);

  SDL_GPSetBlendMode(SDL_GP_BLENDMODE_NONE);
  SDL_GPPushTransform();
  SDL_GPTranslate(0, 0);
  draw_rects(brightness, alpha);
  SDL_GPPopTransform();

  SDL_GPSetBlendMode(SDL_GP_BLENDMODE_BLEND);
  SDL_GPPushTransform();
  SDL_GPTranslate(80, 0);
  draw_rects(brightness, alpha);
  SDL_GPPopTransform();

  SDL_GPSetBlendMode(SDL_GP_BLENDMODE_BLEND_PREMULTIPLIED);
  SDL_GPPushTransform();
  SDL_GPTranslate(160, 0);
  draw_rects(brightness, alpha);
  SDL_GPPopTransform();

  SDL_GPSetBlendMode(SDL_GP_BLENDMODE_ADD);
  SDL_GPPushTransform();
  SDL_GPTranslate(80, 80);
  draw_rects(brightness, alpha);
  SDL_GPPopTransform();

  SDL_GPSetBlendMode(SDL_GP_BLENDMODE_ADD_PREMULTIPLIED);
  SDL_GPPushTransform();
  SDL_GPTranslate(160, 80);
  draw_rects(brightness, alpha);
  SDL_GPPopTransform();

  SDL_GPSetBlendMode(SDL_GP_BLENDMODE_MOD);
  SDL_GPPushTransform();
  SDL_GPTranslate(80, 160);
  draw_rects(brightness, alpha);
  SDL_GPPopTransform();

  SDL_GPSetBlendMode(SDL_GP_BLENDMODE_MUL);
  SDL_GPPushTransform();
  SDL_GPTranslate(160, 160);
  draw_rects(brightness, alpha);
  SDL_GPPopTransform();
}

void
sample_blend_shutdown(void)
{
}

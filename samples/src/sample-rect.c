#include <SDL3/SDL.h>

#include "SDL_gp.h"

#include "context.h"

#include "sample-rect.h"

static Context *_context;

void
sample_rect_setup(Context *context)
{
  _context = context;
}

void
sample_rect_render(Uint64 delta_time_ms)
{
  SDL_GPSetColor((SDL_Color){ 255, 255, 0, 255 });

  SDL_GPDrawPoint((SDL_GPPoint){ 100, 100 });

  SDL_GPDrawPoint((SDL_GPPoint){ 150, 150 });
}

void
sample_rect_shutdown(void)
{
}

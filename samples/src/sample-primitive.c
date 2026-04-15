#include <SDL3/SDL.h>

#include "SDL_gp.h"

#include "context.h"

#include "sample-primitive.h"

static Context *_context = NULL;

void
sample_primitive_setup(Context *context)
{
  _context = context;
}

void
sample_primitive_render(Uint64 delta_time_ms)
{
  int window_width, window_height;
  SDL_GetWindowSize(_context->window, &window_width, &window_height);

  int hw = window_width / 2;
  int hh = window_height / 2;

  // Seconds since start
  float time = (float)SDL_GetTicks() / 1000.0f;

  // Oscillate between -1 and 1 every second
  float osc_1 = SDL_sin(time * SDL_PI_F);

  // Quadrant 1
  // ===============================================================
  // Draw points
  {
    SDL_GPViewport(0, 0, hw, hh);
    SDL_GPSetColor((SDL_Color){ 10, 10, 10, 255 });
    SDL_GPClear();

    SDL_GPSetColor((SDL_Color){ 255, 255, 255, 255 });

    int step_w = hw / 10;
    int step_h = hh / 10;

    for (int y = step_h / 2; y < hh; y += step_h) {
      for (int x = step_w / 2; x < hw; x += step_w) {
        SDL_GPDrawPoint((SDL_GPPoint){ x, y });
      }
    }
  }

  // Quadrant 2
  // ===============================================================
  // Triangle
  {
    SDL_GPViewport(hw, 0, hw, hh);
    SDL_GPSetColor((SDL_Color){ 20, 20, 20, 255 });
    SDL_GPClear();

    SDL_GPPushTransform();
    {
      // Move the the center of the left sub-viewport
      SDL_GPTranslate(hw * 0.25, hh * 0.5);

      float half_shape = hw * 0.15; // 15% of the viewport width

      SDL_GPSetColor((SDL_Color){ 255, 0, 255, 255 });

      SDL_GPDrawFilledTriangle((SDL_GPTriangle){
          .a = (SDL_GPPoint){ 0, -half_shape },
          .b = (SDL_GPPoint){ half_shape, half_shape },
          .c = (SDL_GPPoint){ -half_shape, half_shape },
      });

      // Draw a triangle
    }
    SDL_GPPopTransform();

    SDL_GPPushTransform();
    {
      // Move the the center of the right sub-viewport
      SDL_GPTranslate(hw * 0.75, hh * 0.5);

      float half_shape = hw * 0.15; // 15% of the viewport width

      // Draw a colorful triangle strip
      SDL_Color colors[3] = {
        { 255, 100, 100, 255 },
        { 255, 180, 100, 255 },
        { 180, 100, 255, 255 },
      };

      SDL_GPVec2 positions[4] = {
        { 0, -half_shape },
        { half_shape, half_shape },
        { -half_shape, half_shape },
      };

      SDL_GPVertex vertex_buffer[3];

      for (int i = 0; i < 3; ++i) {
        vertex_buffer[i].position = positions[i];
        vertex_buffer[i].color    = colors[i];
      }

      SDL_GPDraw(SDL_GP_PRIMITIVE_TRIANGLE_STRIP, vertex_buffer, 3);
    }
    SDL_GPPopTransform();
  }

  // Quadrant 3
  // ===============================================================
  {
    SDL_GPViewport(0, hh, hw, hh);
    SDL_GPSetColor((SDL_Color){ 20, 20, 20, 255 });
    SDL_GPClear();
  }

  // Quadrant 4
  // ===============================================================
  {
    SDL_GPViewport(hw, hh, hw, hh);
    SDL_GPSetColor((SDL_Color){ 10, 10, 10, 255 });
    SDL_GPClear();
  }
}

void
sample_primitive_shutdown(void)
{
}

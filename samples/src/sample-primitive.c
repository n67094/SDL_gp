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

    // -31 instead of -32 to draw points at the edges of the viewport
    for (int y = 32; y < hh - 31; y += 8) {
      for (int x = 32; x < hw - 31; x += 8) {
        SDL_GPDrawPoint((SDL_GPPoint){ x, y });
      }
    }
  }

  // Quadrant 2
  // ===============================================================
  // Triangles
  {
    SDL_GPViewport(hw, 0, hw, hh);
    SDL_GPSetColor((SDL_Color){ 20, 20, 20, 255 });
    SDL_GPClear();

    SDL_GPPushTransform();
    {
      // Move to the center of the left area of the viewport
      SDL_GPTranslate(hw * 0.25, hh * 0.5);

      // Oscillate the scale between 0.75 and 1.25
      SDL_GPScale(1.0f + 0.25f * osc_1, 1.0f + 0.25f * osc_1);

      float half_shape = hw * 0.15; // 15% of the viewport width

      SDL_GPSetColor((SDL_Color){ 255, 0, 255, 255 });

      SDL_GPDrawFilledTriangle((SDL_GPTriangle){
          .a = (SDL_GPPoint){ 0, -half_shape },
          .b = (SDL_GPPoint){ half_shape, half_shape },
          .c = (SDL_GPPoint){ -half_shape, half_shape },
      });
    }
    SDL_GPPopTransform();

    SDL_GPPushTransform();
    {
      // Move to the center of the right area of the viewport
      SDL_GPTranslate(hw * 0.75, hh * 0.5);

      // Oscillate the scale between 0.75 and 1.25
      SDL_GPScale(1.0f + 0.25f * -osc_1, 1.0f + 0.25f * -osc_1);

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
  // Draw tiangles fans
  {
    SDL_GPViewport(0, hh, hw, hh);
    SDL_GPSetColor((SDL_Color){ 20, 20, 20, 255 });
    SDL_GPClear();

    // Hexagon
    SDL_GPPushTransform();
    {
      // Move the the center of the left area of the viewport
      SDL_GPTranslate(hw * 0.25, hh * 0.5);

      // Rotate 90 degrees clockwise and counter-clockwise every second
      SDL_GPRotate(osc_1 * SDL_PI_F * 0.5f);

      float half_shape = hw * 0.15; // 15% of the viewport width

      SDL_GPSetColor((SDL_Color){ 0, 255, 255, 255 });

      float step = (2.0f * SDL_PI_F) / 6.0f;

      int count = 0;
      SDL_GPVec2 points_buffer[7]; // 6 segments + 1 for the center vertex
                                   // (each 3 vertices)

      for (float angle = 0.0f; angle <= 2.0f * SDL_PI_F + step * 0.5f;
           angle += step) {

        points_buffer[count] = (SDL_GPVec2){ half_shape * SDL_cos(angle),
                                             half_shape * SDL_sin(angle) };
        count++;

        // Add a center vertex every 3 vertices
        if (count % 3 == 1) {
          points_buffer[count] = (SDL_GPVec2){ 0, 0 };
          count++;
        }
      }

      SDL_GPDrawFilledTrianglesStrip(points_buffer, count);
    }
    SDL_GPPopTransform();

    // Color wheel with 64 segments
    SDL_GPPushTransform();
    {
      // Move to the center of the right area of the viewport
      SDL_GPTranslate(hw * 0.75, hh * 0.5);

      float half_shape = hw * 0.15; // 15% of the viewport width

      float step = (2.0f * SDL_PI_F) / 64.0f;

      int count = 0;
      SDL_GPVertex vertex_buffer[98]; // 64 segments + 32 center vertices (each
                                      // 3 vertices)

      for (float angle = 0.0f; angle <= 2.0f * SDL_PI_F + step * 0.5f;
           angle += step) {

        vertex_buffer[count].position
            = (SDL_GPVec2){ half_shape * SDL_cos(angle),
                            half_shape * SDL_sin(angle) };

        vertex_buffer[count].color
            = (SDL_Color){ (SDL_sin(angle + time * 1) + 1.0f) * 0.5f * 255.0f,
                           (SDL_sin(angle + time * 2) + 1.0f) * 0.5f * 255.0f,
                           (SDL_sin(angle + time * 4) + 1.0f) * 0.5f * 255.0f,
                           255 };
        count++;

        // Add a center vertex every 3 vertices
        if (count % 3 == 1) {
          vertex_buffer[count].position = (SDL_GPVec2){ 0, 0 };
          vertex_buffer[count].color    = (SDL_Color){ 255, 255, 255, 255 };
          count++;
        }
      }

      SDL_GPDraw(SDL_GP_PRIMITIVE_TRIANGLE_STRIP, vertex_buffer, count);
    }
    SDL_GPPopTransform();
  }

  // Quadrant 4
  // ===============================================================
  // Draw lines
  {
    SDL_GPViewport(hw, hh, hw, hh);
    SDL_GPSetColor((SDL_Color){ 10, 10, 10, 255 });
    SDL_GPClear();

    SDL_GPPushTransform();
    {
      // Move to the center of the viewport
      SDL_GPTranslate(hw * 0.5f, hh * 0.5f);

      // Rotate indefinitely
      SDL_GPRotate(time * SDL_PI_F * 0.25f);

      float half_shape = hw * 0.15; // 15% of the viewport width

      SDL_GPSetColor((SDL_Color){ 255, 255, 0, 255 });

      SDL_GPDrawLine(
          (SDL_GPLine){ .a = (SDL_GPVec2){ -half_shape, -half_shape },
                        .b = (SDL_GPVec2){ half_shape, half_shape } });

      SDL_GPDrawLine(
          (SDL_GPLine){ .a = (SDL_GPVec2){ half_shape, -half_shape },
                        .b = (SDL_GPVec2){ -half_shape, half_shape } });
    }
    SDL_GPPopTransform();
  }
}

void
sample_primitive_shutdown(void)
{
}

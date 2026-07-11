#include <SDL3/SDL.h>

#include "SDL_gp.h"

#include "context.h"

#include "sample-load-images.h"

#define MAX_IMAGES 30
#define TRIGGER_FRAME 60

static Context *_context;

static SDL_GPImage _images_setup[MAX_IMAGES];
static SDL_GPImage _images_frame[MAX_IMAGES];

void
sample_load_images_setup(Context *context)
{
  _context = context;

  SDL_Surface *surface = SDL_LoadSurface("../images/hello-world.png");

  if (!surface) {
    SDL_Log("Failed to load image: %s", SDL_GetError());
  }

  for (int i = 0; i < MAX_IMAGES; ++i) {
    _images_setup[i] = SDL_GPCreateImage(surface);
  }

  SDL_DestroySurface(surface);
}

static size_t frame_count = 0;

void
sample_load_images_render(Uint64 delta_time_ms)
{
  SDL_Log("Frame count: %zu", frame_count);

  int window_width, window_height;
  SDL_GetWindowSize(_context->window, &window_width, &window_height);

  SDL_GPSetBlendMode(SDL_GP_BLENDMODE_BLEND);

  // trigger loading images after a certain number of frames
  if (frame_count == TRIGGER_FRAME) {
    SDL_Surface *surface = SDL_LoadSurface("../images/sprites.png");

    if (!surface) {
      SDL_Log("Failed to load image: %s", SDL_GetError());
    }

    for (int i = 0; i < MAX_IMAGES; ++i) {
      _images_frame[i] = SDL_GPCreateImage(surface);
    }

    SDL_DestroySurface(surface);
  }

  // Render the images loaded during the frame that triggered the loading
  // and onwards
  if (frame_count >= TRIGGER_FRAME) {

    static int tile_width  = 32;
    static int tile_height = 32;

    static SDL_GPRect tile_region[3] = {
      { 0, 0, 32, 32 },  // tile 1
      { 32, 0, 32, 32 }, // tile 2
      { 64, 0, 32, 32 }, // tile 3
    };

    static int max_sprite = 4096;

    SDL_GPSetColor((SDL_Color){ 255, 255, 255, 255 });

    // randomly select one of the loaded images to render
    SDL_GPSetImage(0, _images_frame[SDL_rand(MAX_IMAGES)]);

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

    SDL_GPResetImage(0);
  }

  // Render the images loaded during setup all the time
  SDL_GPImage image = _images_setup[SDL_rand(MAX_IMAGES)];

  SDL_GPSetImage(0, image);

  SDL_GPSetColor((SDL_Color){ 255, 255, 255, 255 });

  int width  = SDL_GPGetImageWidth(image);
  int height = SDL_GPGetImageHeight(image);

  SDL_GPRect src_rect = { 0, 0, width, height };
  SDL_GPRect dst_rect = { (window_width - width) * 0.5f,
                          (window_height - height) * 0.5f,
                          width,
                          height };

  SDL_GPDrawTexturedRect(0,
                         (SDL_GPTexturedRect){
                             .src = src_rect,
                             .dst = dst_rect,
                         });
  SDL_GPResetImage(0);

  SDL_GPResetBlendMode();

  frame_count++;
}

void
sample_load_images_shutdown(void)
{
  for (int i = 0; i < MAX_IMAGES; ++i) {
    SDL_GPDestroyImage(_images_setup[i]);
    SDL_GPDestroyImage(_images_frame[i]);
  }
}

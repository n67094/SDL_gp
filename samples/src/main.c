#include <SDL3/SDL.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#define SDL_GP_IMPLEMENTATION
#include "SDL_gp.h"

#include "context.h"

#include "sample-blend.h"
#include "sample-primitive.h"
#include "sample-rect.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define DELTA_TIME_MS 16 // ~60 FPS

typedef enum
{
  SAMPLE_RECT = 0,
  SAMPLE_PRIMITIVE,
  SAMPLE_BLEND,
  SAMPLE_COUNT
} SampleType;

static SampleType _current_test = SAMPLE_RECT;

static Context _context;

SDL_AppResult
SDL_AppInit(void **appstate, int argc, char **argv)
{
  // Init SDL

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  // Create a GPU device

  _context.gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV
                                                | SDL_GPU_SHADERFORMAT_DXIL
                                                | SDL_GPU_SHADERFORMAT_MSL,
                                            true,
                                            NULL);
  if (_context.gpu_device == NULL) {
    SDL_Log("GPUCreateDevice failed");
    return SDL_APP_FAILURE;
  }

  // Create a window

  int flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
  _context.window
      = SDL_CreateWindow("SDL_gp", WINDOW_WIDTH, WINDOW_HEIGHT, flags);
  if (_context.window == NULL) {
    SDL_Log("CreateWindow failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  // Claim the window for use with the GPU device

  if (!SDL_ClaimWindowForGPUDevice(_context.gpu_device, _context.window)) {
    SDL_Log("GPUClaimWindow failed");
    return SDL_APP_FAILURE;
  }

  // Set the swapchain parameters for the window

  SDL_GPUPresentMode present_mode = SDL_GPU_PRESENTMODE_VSYNC;
  if (SDL_WindowSupportsGPUPresentMode(_context.gpu_device,
                                       _context.window,
                                       SDL_GPU_PRESENTMODE_IMMEDIATE)) {
    present_mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
  } else if (SDL_WindowSupportsGPUPresentMode(_context.gpu_device,
                                              _context.window,
                                              SDL_GPU_PRESENTMODE_MAILBOX)) {
    present_mode = SDL_GPU_PRESENTMODE_MAILBOX;
  }

  SDL_SetGPUSwapchainParameters(_context.gpu_device,
                                _context.window,
                                SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                present_mode);

  // Acquire the first command buffer to use for setup

  _context.cmd_buffer = SDL_AcquireGPUCommandBuffer(_context.gpu_device);
  if (_context.cmd_buffer == NULL) {
    SDL_Log("Failed to acquire GPU command buffer (error: %s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_GPSetCommandBuffer(_context.cmd_buffer);

  // Setup SDL_gp

  SDL_GPDesc sdl_gp_desc = {
    .window     = _context.window,
    .gpu_device = _context.gpu_device,
  };

  SDL_GPSetup(&sdl_gp_desc);

  // Setup samples

  sample_rect_setup(&_context);
  sample_primitive_setup(&_context);
  sample_blend_setup(&_context);

  // Submit for resources uploads during setup

  SDL_SubmitGPUCommandBuffer(_context.cmd_buffer);

  return SDL_APP_CONTINUE;
}

SDL_AppResult
SDL_AppIterate(void *appstate)
{
  _context.cmd_buffer = SDL_AcquireGPUCommandBuffer(_context.gpu_device);
  if (_context.cmd_buffer == NULL) {
    SDL_Log("Failed to acquire GPU command buffer (error: %s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_GPSetCommandBuffer(_context.cmd_buffer);

  if (!SDL_WaitAndAcquireGPUSwapchainTexture(_context.cmd_buffer,
                                             _context.window,
                                             &_context.swapchain_texture,
                                             NULL,
                                             NULL)) {
    SDL_Log("Failed to acquire swapchain texture (error: %s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_GPSetSwapchainTexture(_context.swapchain_texture);

  SDL_GPBegin(WINDOW_WIDTH, WINDOW_HEIGHT);
  {
    SDL_GPSetColor((SDL_Color){ 0, 0, 0, 255 });
    SDL_GPClear();

    switch (_current_test) {
    case SAMPLE_RECT:
      sample_rect_render(DELTA_TIME_MS);
      break;
    case SAMPLE_PRIMITIVE:
      sample_primitive_render(DELTA_TIME_MS);
      break;
    case SAMPLE_BLEND:
      sample_blend_render(DELTA_TIME_MS);
      break;
    default:
      break;
    }

    SDL_GPFlush(_context.swapchain_texture);
  }
  SDL_GPEnd();

  SDL_SubmitGPUCommandBuffer(_context.cmd_buffer);

  SDL_Delay(DELTA_TIME_MS);

  return SDL_APP_CONTINUE;
}

SDL_AppResult
SDL_AppEvent(void *appstate, SDL_Event *event)
{
  switch (event->type) {
  case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    return SDL_APP_SUCCESS;
  case SDL_EVENT_QUIT:
    return SDL_APP_SUCCESS;
  case SDL_EVENT_KEY_DOWN:
    switch (event->key.key) {
    case SDLK_LEFT:
      _current_test = (_current_test - 1 + SAMPLE_COUNT) % SAMPLE_COUNT;
      break;
    case SDLK_RIGHT:
      _current_test = (_current_test + 1) % SAMPLE_COUNT;
      break;
    }
    break;
  }

  return SDL_APP_CONTINUE;
}

void
SDL_AppQuit(void *appstate, SDL_AppResult result)
{
  sample_rect_shutdown();
  sample_primitive_shutdown();
  sample_blend_shutdown();

  SDL_GPShutdown();

  if (_context.window) {
    SDL_DestroyWindow(_context.window);
    _context.window = NULL;
  }

  if (_context.gpu_device) {
    SDL_DestroyGPUDevice(_context.gpu_device);
    _context.gpu_device = NULL;
  }

  SDL_Quit();
}

#include <SDL3/SDL.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <float.h>

#include "SDL_gp.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

static SDL_GPUDevice *_gpu_device         = NULL;
static SDL_Window *_window                = NULL;
static SDL_GPUCommandBuffer *_cmd_buffer  = NULL;
static SDL_GPUTexture *_swapchain_texture = NULL;

SDL_AppResult
SDL_AppInit(void **appstate, int argc, char **argv)
{
  // Init SDL
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  // Create a GPU device
  _gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV
                                        | SDL_GPU_SHADERFORMAT_DXIL
                                        | SDL_GPU_SHADERFORMAT_MSL,
                                    true,
                                    NULL);
  if (_gpu_device == NULL) {
    SDL_Log("GPUCreateDevice failed");
    return SDL_APP_FAILURE;
  }

  // Create a window
  int flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
  _window   = SDL_CreateWindow("SDL_gp", WINDOW_WIDTH, WINDOW_HEIGHT, flags);
  if (_window == NULL) {
    SDL_Log("CreateWindow failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  // Claim the window for use with the GPU device
  if (!SDL_ClaimWindowForGPUDevice(_gpu_device, _window)) {
    SDL_Log("GPUClaimWindow failed");
    return SDL_APP_FAILURE;
  }

  // Set the swapchain parameters for the window
  SDL_GPUPresentMode present_mode = SDL_GPU_PRESENTMODE_VSYNC;
  if (SDL_WindowSupportsGPUPresentMode(
          _gpu_device, _window, SDL_GPU_PRESENTMODE_IMMEDIATE)) {
    present_mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
  } else if (SDL_WindowSupportsGPUPresentMode(
                 _gpu_device, _window, SDL_GPU_PRESENTMODE_MAILBOX)) {
    present_mode = SDL_GPU_PRESENTMODE_MAILBOX;
  }

  SDL_SetGPUSwapchainParameters(
      _gpu_device, _window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, present_mode);

  return SDL_APP_CONTINUE;
}

SDL_AppResult
SDL_AppIterate(void *appstate)
{
  _cmd_buffer = SDL_AcquireGPUCommandBuffer(_gpu_device);
  if (_cmd_buffer == NULL) {
    SDL_Log("Failed to acquire GPU command buffer (error: %s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_WaitAndAcquireGPUSwapchainTexture(
          _cmd_buffer, _window, &_swapchain_texture, NULL, NULL)) {
    SDL_Log("Failed to acquire swapchain texture (error: %s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  // Render

  SDL_SubmitGPUCommandBuffer(_cmd_buffer);

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
  }

  return SDL_APP_CONTINUE;
}

void
SDL_AppQuit(void *appstate, SDL_AppResult result)
{
  if (_window) {
    SDL_DestroyWindow(_window);
    _window = NULL;
  }

  if (_gpu_device) {
    SDL_DestroyGPUDevice(_gpu_device);
    _gpu_device = NULL;
  }

  SDL_Quit();
}

#include <SDL3/SDL.h>

#define SDL_GP_IMPLEMENTATION
#include "SDL_gp.h"

static void
test_pool_overflow(void)
{
  // Internally SDL_gp use 8 pipelines for its own purposes, so we can only
  // create SDL_GP_PIPELINE_MAX - 8 pipelines before the pool is full.
  // Using the pipeline pool is arbitrary, it's just to test the pool overflow
  // behavior.
  SDL_GPPipeline pipelines[SDL_GP_PIPELINE_MAX - 8];
  int count = 0;

  for (int i = 0; i < SDL_GP_PIPELINE_MAX - 8; ++i) {
    SDL_GPPipeline pipeline = SDL_GPCreatePipeline(_gp.shader_vert,
                                                   _gp.shader_frag,
                                                   SDL_GP_PRIMITIVE_TRIANGLES,
                                                   SDL_GP_BLENDMODE_NONE);
    if (pipeline.id == SDL_GP_INVALID_ID) {
      break;
    }
    SDL_Log("pipeline #%d -> slot %d", count, SDL_GPPoolIdToSlot(pipeline.id));
    pipelines[count++] = pipeline;
  }

  SDL_Log("pipeline pool full after %d pipelines", count);

  for (int i = 0; i < count; ++i) {
    SDL_GPDestroyPipeline(pipelines[i]);
  }
}

int
main(int argc, char **argv)
{
  // Init SDL

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s\n", SDL_GetError());
    return 1;
  }

  // Create a GPU device

  SDL_GPUDevice *gpu_device = SDL_CreateGPUDevice(
      SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL
          | SDL_GPU_SHADERFORMAT_MSL,
      true,
      NULL);
  if (gpu_device == NULL) {
    SDL_Log("GPUCreateDevice failed");
    return 1;
  }

  // Create a hidden window

  SDL_Window *window
      = SDL_CreateWindow("SDL_gp tests", 64, 64, SDL_WINDOW_HIDDEN);
  if (window == NULL) {
    SDL_Log("CreateWindow failed: %s", SDL_GetError());
    return 1;
  }

  // Claim the window for use with the GPU device

  if (!SDL_ClaimWindowForGPUDevice(gpu_device, window)) {
    SDL_Log("GPUClaimWindow failed");
    return 1;
  }

  // Setup SDL_gp

  SDL_GPDesc sdl_gp_desc = {
    .window     = window,
    .gpu_device = gpu_device,
  };

  if (!SDL_GPSetup(&sdl_gp_desc)) {
    SDL_Log("SDL_GPSetup failed: %s",
            SDL_GPGetErrorMessage(SDL_GPGetLastError()));
    return 1;
  }

  // Run tests

  test_pool_overflow();

  // Shutdown

  SDL_GPShutdown();
  SDL_DestroyWindow(window);
  SDL_DestroyGPUDevice(gpu_device);
  SDL_Quit();

  return 0;
}

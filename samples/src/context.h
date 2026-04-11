#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <SDL3/SDL.h>

typedef struct
{
  SDL_GPUDevice *gpu_device;
  SDL_Window *window;
  SDL_GPUCommandBuffer *cmd_buffer;
  SDL_GPUTexture *swapchain_texture;
} Context;

#endif // CONTEXT_H_

/**
 * TODO re-organize structures to take less memory.
 * https://github.com/n67094/SDL_gp
 *
 * SDL_gp: A minimal and efficient 2D (g)raphics (p)ainter for SDL3.
 *
 * # SDL_gp
 *
 * This is a port of sokol_gp (https://github.com/edubart/sokol_gp) to SDL3,
 * with one main difference:
 *
 * sokol_gp relies on sokol, which manages resources internally. SDL_gpu is
 * lower-level and does not; So `SDL_gp` provides a simple resource management
 * system very similar to sokol's.
 *
 * # Acknowledgements
 *
 * Thanks to [Edubart](https://github.com/edubart) for his work on `sokol_gp`.
 * Thanks to [The SDL team](https://github.com/libsdl-org) for their work.
 *
 * # Sponsors
 *
 * Hi guys, I'm nsix and I'm trying to make a living as an indie game developer
 * and open source contributor.
 *
 * If you like my work and want to support me, well
 * thanks a lot I really appreciate it!
 *
 * You can sponsor me on [GitHub Sponsors](https://github.com/sponsors/n67094)
 *
 * # License
 *
 * Copyright (c) 2026 nsix. All rights reserved. (n67094@proton.me)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef SDL_GP_H_
#define SDL_GP_H_

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

// Resource limits
// ----------------------------------------------------------------------------

#ifndef SDL_GP_PATH_MAX
#define SDL_GP_PATH_MAX 512
#endif

// Max texture dimension in pixels
#ifndef SDL_GP_TEXTURE_DIMENSION_MAX
#define SDL_GP_TEXTURE_DIMENSION_MAX 4096
#endif

// Max number of images that can be loaded at the same time
#ifndef SDL_GP_IMAGE_MAX
#define SDL_GP_IMAGE_MAX 64
#endif

// Max number of shaders that can be loaded at the same time
#ifndef SDL_GP_SHADER_MAX
#define SDL_GP_SHADER_MAX 8
#endif

// Max number of pipelines that can be created at the same time
#ifndef SDL_GP_PIPELINE_MAX
#define SDL_GP_PIPELINE_MAX 64
#endif

// ----------------------------------------------------------------------------
// Limits
// ----------------------------------------------------------------------------

// Max number of painters state that can be used simultaneously (per frame)
#ifndef SDL_GP_STATE_MAX
#define SDL_GP_STATE_MAX 16
#endif

// Max number of textures that can be bound at the same time (per draw call)
#ifndef SDL_GP_TEXTURE_SLOTS_MAX
#define SDL_GP_TEXTURE_SLOTS_MAX 4
#endif

// Max number of vertices that can be drawn in a single flush
#ifndef SDL_GP_VERTICES_MAX
#define SDL_GP_VERTICES_MAX 65536
#endif

// Max number of draw commands that can be issued in a single flush
#ifndef SDL_GP_COMMANDS_MAX
#define SDL_GP_COMMANDS_MAX 16384
#endif

// Max number of transforms that can be pushed at the same time (per frame)
#ifndef SDL_GP_TRANSFORMS_MAX
#define SDL_GP_TRANSFORMS_MAX 256
#endif

// Max number of float (4-bytes) uniforms that can be set in a shader
#ifndef SDL_GP_UNIFORM_FLOATS_MAX
#define SDL_GP_UNIFORM_FLOATS_MAX 8
#endif

// Max number of commands that are looked back and batched together for
// optimization
#ifndef SDL_GP_OPTIMIZER_DEPTH
#define SDL_GP_OPTIMIZER_DEPTH 8
#endif

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

#define SDL_GP_INVALID_ID 0
#define SDL_GP_IMPOSSIBLE_ID 0xFFFFFFFF

// Get a default value if the provided value is 0
#define SDL_GP_DEFAULT(val, def) (((val) == 0) ? (def) : (val))

// Get the offset of an element in a structure
#define SDL_GP_OFFSET_OF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))

// TODO DEfault macro

// Pool (Public for users who want to re-use it as-is for other resources).
// ----------------------------------------------------------------------------
// The pool is a simple resource management system.
//
// The pool work like this, there is a fixed number of slots (defined at pool
// creation) that can be acquired and released. Each slot has an incrementing
// generation counter, which is used to generate unique ids for each slot. When
// a slot is released, its generation counter is incremented, so that any ids
// generated from that slot will be invalid until the slot is acquired again.

#define SDL_GP_POOL_INVALID_SLOT 0
#define SDL_GP_POOL_SLOT_SHIFT 16
#define SDL_GP_POOL_SLOT_MASK ((1 << SDL_GP_POOL_SLOT_SHIFT) - 1)

typedef struct SDL_GPPool {
  size_t size; // total number of slots in the pool (counting the invalid slot)
  int queue_top;    // index of the top of the free queue
  Uint32 *counters; // incrementing generation counters for each slot
  int *free_queue;  // queue of free slots
} SDL_GPPool;

// Create a pool with the specified number of slots (not counting the invalid
// slot).
SDL_GPPool *SDL_GPCeatePool(size_t number_of_slots);

// Destroy a pool and free its resources.
void SDL_GPDestroyPool(SDL_GPPool *resource);

// Acquire a slot from the pool and return its index. Returns
// SDL_GP_POOL_INVALID_SLOT if no more slots are available.
int SDL_GPAcquirePoolSlot(SDL_GPPool *resource);

// Release a slot back to the pool, making it available for future acquisitions.
void SDL_GPReleasePoolSlot(SDL_GPPool *resource, int slot_index);

// Generate a unique id for a slot in the pool using its index and generation
// counter.
Uint32 SDL_GPGeneratePoolId(SDL_GPPool *resource, int slot_index);

// Extract the slot index from a generated id.
int SDL_GPPoolIdToSlot(Uint32 id);

// Image (Public)
// ----------------------------------------------------------------------------
//
// An image is a wrapper around a GPU texture, with some additional metadata
// (width and height). The image creation function will create a GPU texture
// from an SDL_Surface and upload the surface pixels to the GPU texture.
//
// NOTE: The surface will be converted to the swapchain texture format if
// needed.

typedef enum {
  SDL_GP_SAMPLER_POINT_CLAMP = 0,
  SDL_GP_SAMPLER_POINT_WRAP,
  SDL_GP_SAMPLER_LINEAR_CLAMP,
  SDL_GP_SAMPLER_LINEAR_WRAP,
  SDL_GP_SAMPLER_SIZE,
} SDL_GPSampler;

typedef struct SDL_GPImage {
  Uint32 id;
} SDL_GPImage;

// Create an image from an SDL_Surface. Returns an invalid image if creation
// failed.
SDL_GPImage SDL_GPCreateImage(SDL_Surface *surface);

// Destroy an image and free its resources.
void SDL_GPDestroyImage(SDL_GPImage image);

// Get the GPU texture associated with an image. Returns NULL if the image is
// invalid.
SDL_GPUTexture *SDL_GPGetImageGPUTexture(SDL_GPImage image);

// Get the width of an image in pixels. Returns 0 if the image is invalid.
int SDL_GPGetImageWidth(SDL_GPImage image);

// Get the height of an image in pixels. Returns 0 if the image is invalid.
int SDL_GPGetImageHeight(SDL_GPImage image);

// Sampler (Public)
// ----------------------------------------------------------------------------

// Shader (Public)
// ----------------------------------------------------------------------------

typedef struct SDL_GPShader {
  Uint32 id;
} SDL_GPShader;

typedef struct SDL_GPShaderDesc {
  // Vertex shader description
  size_t vert_code_size;
  const Uint8 *vert_code;
  const char *vert_entrypoint;
  SDL_GPUShaderFormat vert_format;
  Uint32 vert_num_samplers;
  Uint32 vert_num_storage_textures;
  Uint32 vert_num_storage_buffers;
  Uint32 vert_num_uniform_buffers;

  // Fragment shader description
  size_t frag_code_size;
  const Uint8 *frag_code;
  const char *frag_entrypoint;
  SDL_GPUShaderFormat frag_format;
  Uint32 frag_num_samplers;
  Uint32 frag_num_storage_textures;
  Uint32 frag_num_storage_buffers;
  Uint32 frag_num_uniform_buffers;
} SDL_GPShaderDesc;

// Create a shader from vertex and fragment shader descriptions. Returns an
// invalid shader if creation failed.
SDL_GPShader SDL_GPCreateShader(SDL_GPShaderDesc *desc);

SDL_GPUShader *SDL_GPGetGPUVertexShader(SDL_GPShader shader);

SDL_GPUShader *SDL_GPGetGPUFragmentShader(SDL_GPShader shader);

// Destroy a shader and free its resources.
void SDL_GPDestroyShader(SDL_GPShader shader);

// Pipeline (Public)
// ----------------------------------------------------------------------------

typedef enum {
  SDL_GP_BLENDMODE_NONE = SDL_BLENDMODE_NONE,
  SDL_GP_BLENDMODE_BLEND = SDL_BLENDMODE_BLEND,
  SDL_GP_BLENDMODE_BLEND_PREMULTIPLIED = SDL_BLENDMODE_BLEND_PREMULTIPLIED,
  SDL_GP_BLENDMODE_ADD = SDL_BLENDMODE_ADD,
  SDL_GP_BLENDMODE_ADD_PREMULTIPLIED = SDL_BLENDMODE_ADD_PREMULTIPLIED,
  SDL_GP_BLENDMODE_MOD = SDL_BLENDMODE_MOD,
  SDL_GP_BLENDMODE_MUL = SDL_BLENDMODE_MUL,
  SDL_GP_BLENDMODE_SIZE = 7,
} SDL_GPBlendMode;

typedef enum {
  SDL_GP_PRIMITIVE_TRIANGLES = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
  SDL_GP_PRIMITIVE_TRIANGLE_STRIP = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
  SDL_GP_PRIMITIVE_LINES = SDL_GPU_PRIMITIVETYPE_LINELIST,
  SDL_GP_PRIMITIVE_LINE_STRIP = SDL_GPU_PRIMITIVETYPE_LINESTRIP,
  SDL_GP_PRIMITIVE_POINTS = SDL_GPU_PRIMITIVETYPE_POINTLIST,
  SDL_GP_PRIMITIVE_SIZE = 5,
} SDL_GPPrimitiveType;

typedef struct SDL_GPPipeline {
  Uint32 id;
} SDL_GPPipeline;

// Create a graphics pipeline
SDL_GPPipeline SDL_GPCreatePipeline(SDL_GPShader shader,
                                    SDL_GPPrimitiveType primitive_type,
                                    SDL_GPBlendMode blend_mode);

// Destroy a graphics pipeline and free its resources.
void SDL_GPPipelineDestroy(SDL_GPPipeline pipeline);

// Get the GPU graphics pipeline associated with a pipeline. Returns NULL if the
// pipeline is invalid.
SDL_GPUGraphicsPipeline *SDL_GPGetGPUPipeline(SDL_GPPipeline pipeline);

// Painter (Public)
// ----------------------------------------------------------------------------

typedef enum {
  // TODO
} SDL_GPError;

typedef enum {
  SDL_GP_UNIFORM_SLOT_VS = 0,
  SDL_GP_UNIFORM_SLOT_FS = 1
} SDL_GPUniformSlot;

typedef struct SDL_GPColor {
  Uint8 r;
  Uint8 g;
  Uint8 b;
  Uint8 a;
} SDL_GPColor;

typedef struct SDL_GPVec2 {
  float x, y;
} SDL_GPVec2;

typedef SDL_GPVec2 SDL_GPPoint;

typedef struct SDL_GPLine {
  SDL_GPPoint a, b;
} SDL_GPLine;

typedef struct SDL_GPTriangle {
  SDL_GPPoint a, b, c;
} SDL_GPTriangle;

typedef struct SDL_GPRect {
  float x, y, w, h;
} SDL_GPRect;

typedef struct SDL_GPTexturedRect {
  SDL_GPRect dst;
  SDL_GPRect src;
} SDL_GPTexturedRect;

typedef struct SDL_GPMat2x3 {
  float m00, m01, m02;
  float m10, m11, m12;
} SDL_GPMat2x3;

// Create an immutable 2x3 matrix
#define SDL_GPCreateMat2x3(m00, m01, m02, m10, m11, m12)                       \
  ((const SDL_GPMat2x3){m00, m01, m02, m10, m11, m12})

// Create an immutable identity 2x3 matrix
#define SDL_GPCreateMat2x3Identity()                                           \
  ((const SDL_GPMat2x3){1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f})

typedef struct SDL_GPVertex {
  SDL_GPVec2 position;
  SDL_GPVec2 texcoord;
  SDL_GPColor color;
} SDL_GPVertex;

typedef union SDL_GPUniformData {
  float floats[SDL_GP_UNIFORM_FLOATS_MAX];
  uint8_t bytes[SDL_GP_UNIFORM_FLOATS_MAX * sizeof(float)];
} SDL_GPUniformData;

typedef struct SDL_GPUniform {
  Uint16 vs_size;
  Uint16 fs_size;
  SDL_GPUniformData data;
} SDL_GPUniform;

typedef struct SDL_GPTextureUniform {
  Uint32 count;
  SDL_GPImage images[SDL_GP_TEXTURE_SLOTS_MAX];
  SDL_GPUSampler *samplers[SDL_GP_TEXTURE_SLOTS_MAX];
} SDL_GPTextureUniform;

typedef struct SDL_GPState {
  SDL_GPMat2x3 projection;
  SDL_GPMat2x3 transform;
  SDL_GPMat2x3 mvp;
  SDL_GPTextureUniform texture;
  SDL_GPUniform uniform;
  SDL_GPPipeline pipeline;
  SDL_GPBlendMode blend_mode;
  SDL_GPVec2 frame_dimension;
  SDL_GPRect viewport;
  SDL_GPRect scissor;
  SDL_Color color;
  float thickness;
  Uint32 base_uniform;
  Uint32 base_vertex;
  Uint32 base_command;
} SDL_GPState;

typedef struct SDL_GPDesc {
  Uint32 max_vertices;
  Uint32 max_commands;
  SDL_Window *window;
  SDL_GPUDevice *gpu_device;
  SDL_GPUTexture *target_texture; // Swapchain
  SDL_GPUCommandBuffer *cmd_buffer;
} SDL_GPDesc;

void SDL_GPSetup(SDL_GPDesc *desc);
void SDL_GPShutdown();

void SDL_GPBegin(void);
void SDL_GPFlush(void);
void SDL_GPEnd(void);

SDL_GPVec2 SDL_GPGetFrameSize(void);

void SDL_GPSetProjection(float left, float right, float bottom, float top);
void SDL_GPResetProjection(void);
void SDL_GPPushTransform(void);
void SDL_GPPopTransform(void);
void SDL_GPResetTransform(void);
void SDL_GPTranslate(float x, float y);
void SDL_GPRotateAt(float angle, float ax, float ay);
void SDL_GPScale(float sx, float sy);
void SDL_GPScaleAt(float sx, float sy, float ax, float ay);

void SDL_GPSetPipeline(SDL_GPPipeline pipeline);
void SDL_GPResetPipeline(void);
void SDL_GPSetUniform(const void *vs_data, size_t vs_size, const void *fs_data,
                      size_t fs_size);
void SDL_GPResetUniform(void);
void SDL_GPPainterSetBlendMode(SDL_GPBlendMode blend_mode);
void SDL_GPPainterResetBlendMode(void);
void SDL_GPPainterSetColor(SDL_GPColor color);
SDL_GPColor SDL_GPGetColor(void);
void SDL_GPResetColor(void);
void SDL_GPSetImage(int channel, SDL_GPImage image);
SDL_GPImage SDL_GPGetImage(int channel);
void SDL_GPResetImage(int channel);
void SDL_GPUnsetImage(int channel);
void SDL_GPSetSampler(int channel, SDL_GPSampler sampler);
void SDL_GPResetSampler(int channel);

// Set the viewport for subsequent draw calls.
void SDL_GPViewport(float x, float y, float w, float h);
void SDL_GPResetViewport(void);

// Set the scissor for subsequent draw calls.
void SDL_GPScissor(float x, float y, float w, float h);
void SDL_GPResetScissor(void);

void SDL_GPClear(void);

void SDL_GPDraw(SDL_GPPrimitiveType primitive_type,
                const SDL_GPVertex *vertices, Uint32 vertices_count);

void SDL_GPDrawPoints(const SDL_GPVec2 *points, Uint32 count);
void SDL_GPDrawPoint(SDL_GPVec2 point);

void SDL_GPDrawLine(SDL_GPLine line);
void SDL_GPDrawLines(const SDL_GPLine *lines, Uint32 count);
void SDL_GPDrawLinesStrip(const SDL_GPVec2 *points, Uint32 count);

void SDL_GPDrawTriangleFilled(SDL_GPTriangle triangle);
void SDL_GPDrawTriangles(const SDL_GPTriangle *triangles, Uint32 count);
void SDL_GPDrawTrianglesStrip(const SDL_GPVec2 *points, Uint32 count);

void SDL_GPDrawRectFilled(SDL_GPRect rect);
void SDL_GPDrawRectsFilled(const SDL_GPRect *rects, Uint32 count);

void SDL_GPDrawRectTextured(int channel, SDL_GPTexturedRect rect);
void SDL_GPDrawRectsTextured(int channel, const SDL_GPTexturedRect *rects,
                             Uint32 count);

#ifdef __cplusplus
}
#endif

#endif // SDL_GP_H_

// ----------------------------------------------------------------------------
// Implementation and internal API
// ----------------------------------------------------------------------------

// TODO remove the next line once done.
#define SDL_GP_IMPLEMENTATION

#ifdef SDL_GP_IMPLEMENTATION

#define _SDL_GP_INIT_COOKIE 0xC0DED1ED

// Pool (Private)
// ----------------------------------------------------------------------------

SDL_GPPool *SDL_GPCeatePool(size_t number_of_slots) {
  SDL_GPPool *pool = (SDL_GPPool *)SDL_malloc(sizeof(SDL_GPPool));

  // the 0 slot is used for invalid ids, so we need to add 1 to the size to
  // account for it
  pool->size = number_of_slots + 1;
  pool->queue_top = 0;
  pool->counters = (Uint32 *)SDL_malloc(pool->size * sizeof(Uint32));
  pool->free_queue = (int *)SDL_malloc(pool->size * sizeof(int));

  // Initialize the free queue with all the slots (except 0) and set the
  // generation counters to 0
  for (int i = pool->size - 1; i >= 1; --i) {
    pool->free_queue[pool->queue_top++] = i;
    pool->counters[i] = 0;
  }

  return pool;
}

void SDL_GPDestroyPool(SDL_GPPool *pool) {
  SDL_free(pool->counters);
  SDL_free(pool->free_queue);
  SDL_free(pool);
}

int SDL_GPAcquirePoolSlot(SDL_GPPool *pool) {
  SDL_assert(pool);
  SDL_assert(pool->free_queue);

  if (pool->queue_top > 0) {
    return pool
        ->free_queue[--pool->queue_top]; // Get a slot from the free queue
  } else {
    return SDL_GP_POOL_INVALID_SLOT; // No more slots available
  }
}

void SDL_GPReleasePoolSlot(SDL_GPPool *pool, int slot) {
  SDL_assert(slot > SDL_GP_POOL_INVALID_SLOT && slot < (int)pool->size);
  SDL_assert(pool);
  SDL_assert(pool->free_queue);
  SDL_assert(pool->queue_top < (int)pool->size);

  pool->free_queue[pool->queue_top++] = slot;

  SDL_assert(pool->queue_top <= (int)pool->size);
}

Uint32 SDL_GPGeneratePoolId(SDL_GPPool *pool, int slot) {
  SDL_assert(slot > SDL_GP_POOL_INVALID_SLOT && slot < (int)pool->size);
  SDL_assert(pool);
  SDL_assert(pool->counters);

  uint32_t counter = ++pool->counters[slot]; // increment generation

  Uint32 id =
      (counter << SDL_GP_POOL_SLOT_SHIFT) | (slot & SDL_GP_POOL_SLOT_MASK);

  return id;
}

int SDL_GPPoolIdToSlot(Uint32 id) {
  int slot_index = (int)(id & SDL_GP_POOL_SLOT_MASK);
  return slot_index;
  return -1;
}

// Image (Private)
// ----------------------------------------------------------------------------

typedef struct _SDL_GPImage {
  SDL_GPUTexture *texture;
  int width;
  int height;
} _SDL_GPImage;

static Uint32 _image_initialized = 0;
static _SDL_GPImage *_images = NULL;
static SDL_GPPool *_image_pool = NULL;
static SDL_GPUTransferBuffer *_image_texture_transfer_buffer = NULL;
static SDL_GPUDevice *_image_gpu_device = NULL;
static SDL_GPUCommandBuffer *_image_cmd_buffer = NULL;
static SDL_PixelFormat *_image_pixel_format = NULL;

// Setup image resources management.
static void _SDL_GPImageSetup(SDL_GPUDevice *gpu_device,
                              SDL_GPUCommandBuffer *cmd_buffer,
                              SDL_PixelFormat pixel_format) {
  SDL_assert(_image_initialized == 0);
  SDL_assert(gpu_device);
  SDL_assert(cmd_buffer);

  _image_initialized = _SDL_GP_INIT_COOKIE;

  _image_gpu_device = gpu_device;
  _image_cmd_buffer = cmd_buffer;

  _image_pool = SDL_GPCeatePool(SDL_GP_IMAGE_MAX);

  _images = (_SDL_GPImage *)SDL_malloc(SDL_GP_IMAGE_MAX * sizeof(_SDL_GPImage));

  SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = 4 * SDL_GP_TEXTURE_DIMENSION_MAX * SDL_GP_TEXTURE_DIMENSION_MAX};

  _image_texture_transfer_buffer =
      SDL_CreateGPUTransferBuffer(gpu_device, &transfer_buffer_create_info);

  if (!_image_texture_transfer_buffer) {
    // TODO SDL_GPSetError(SDL_GP_ERROR_SETUP_IMAGE);
    return;
  }
}

// Shutdown image resources management and free resources.
static void _SDL_GPImageShutdown() {
  SDL_assert(_image_initialized == _SDL_GP_INIT_COOKIE);
  _image_initialized = 0;

  SDL_GPDestroyPool(_image_pool);
  SDL_free(_images);
  SDL_ReleaseGPUTransferBuffer(_image_gpu_device,
                               _image_texture_transfer_buffer);
}

SDL_GPImage SDL_GPCreateImage(SDL_Surface *surface) {
  SDL_assert(_image_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(surface);

  SDL_Surface *inner_surface = surface;

  // Convert the surface to the swapchain texture format if needed

  bool converted = false;
  if (surface->format != *_image_pixel_format) {
    inner_surface = SDL_ConvertSurface(surface, _image_pixel_format);

    if (inner_surface == NULL) {
      // TODO SDL_GPSetError(SDL_GP_ERROR_IMAGE_CREATE);
      return (SDL_GPImage){.id = SDL_GP_INVALID_ID};
    }

    converted = true;
  }

  // Transfer surface pixels to the GPU transfer buffer

  void *texture_transfer_ptr = SDL_MapGPUTransferBuffer(
      _image_gpu_device, _image_texture_transfer_buffer, true);

  SDL_assert(surface->w * surface->h * 4 <=
             SDL_GP_TEXTURE_DIMENSION_MAX * SDL_GP_TEXTURE_DIMENSION_MAX * 4);

  SDL_memcpy(texture_transfer_ptr, surface->pixels,
             surface->w * surface->h * 4);

  SDL_UnmapGPUTransferBuffer(_image_gpu_device, _image_texture_transfer_buffer);

  // Create GPU texture and copy the transfer buffer to it

  SDL_GPUTextureFormat _image_texture_format =
      SDL_GetGPUTextureFormatFromPixelFormat(*_image_pixel_format);

  SDL_GPUTextureCreateInfo texture_create_info = {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = _image_texture_format,
      .width = (Uint32)surface->w,
      .height = (Uint32)surface->h,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER};

  SDL_GPUTexture *texture =
      SDL_CreateGPUTexture(_image_gpu_device, &texture_create_info);

  if (texture == NULL) {
    // TODO SDL_GPSetError(SDL_GP_ERROR_IMAGE_CREATE);
    return (SDL_GPImage){.id = SDL_GP_INVALID_ID};
  }

  // Copy the texture data from the transfer buffer to the GPU texture using a
  // copy pass

  SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(_image_cmd_buffer);

  SDL_GPUTextureTransferInfo transfer_info = {
      .transfer_buffer = _image_texture_transfer_buffer,
      .offset = 0,
  };

  SDL_GPUTextureRegion region = {.texture = texture,
                                 .w = (Uint32)surface->w,
                                 .h = (Uint32)surface->h,
                                 .d = 1};

  SDL_UploadToGPUTexture(copy_pass, &transfer_info, &region, false);

  SDL_EndGPUCopyPass(copy_pass);

  // Allocate image from resource

  int slot = SDL_GPAcquirePoolSlot(_image_pool);
  if (slot == SDL_GP_POOL_INVALID_SLOT) {
    SDL_ReleaseGPUTexture(_image_gpu_device, texture);
    // TODO SDL_GPSetError(SDL_GP_ERROR_IMAGE_CREATE);
    return (SDL_GPImage){.id = SDL_GP_INVALID_ID};
  }

  _images[slot] = (_SDL_GPImage){
      .texture = texture,
      .width = surface->w,
      .height = surface->h,
  };

  // Destroy the converted surface if we created one
  if (converted) {
    SDL_DestroySurface(inner_surface);
  }

  return (SDL_GPImage){.id = SDL_GPGeneratePoolId(_image_pool, slot)};
}

void SDL_GPDestroyImage(SDL_GPImage image) {
  SDL_assert(_image_initialized == _SDL_GP_INIT_COOKIE);

  if (image.id == SDL_GP_INVALID_ID) {
    return;
  }

  int slot = SDL_GPPoolIdToSlot(image.id);

  SDL_GPReleasePoolSlot(_image_pool, slot);

  _SDL_GPImage inner_image = _images[slot];

  SDL_ReleaseGPUTexture(_image_gpu_device, inner_image.texture);

  _images[slot] = (_SDL_GPImage){
      .texture = NULL,
      .width = 0,
      .height = 0,
  };
}

SDL_GPUTexture *SDL_GPGetImageGPUTexture(SDL_GPImage image) {
  SDL_assert(_image_initialized == _SDL_GP_INIT_COOKIE);

  if (image.id == SDL_GP_INVALID_ID) {
    return NULL;
  }

  int slot = SDL_GPPoolIdToSlot(image.id);
  return _images[slot].texture;
}

int SDL_GPGetImageWidth(SDL_GPImage image) {
  SDL_assert(_image_initialized == _SDL_GP_INIT_COOKIE);

  if (image.id == SDL_GP_INVALID_ID) {
    return 0;
  }

  int slot = SDL_GPPoolIdToSlot(image.id);
  return _images[slot].width;
}

int SDL_GPGetImageHeight(SDL_GPImage image) {
  SDL_assert(_image_initialized == _SDL_GP_INIT_COOKIE);

  if (image.id == SDL_GP_INVALID_ID) {
    return 0;
  }

  int slot = SDL_GPPoolIdToSlot(image.id);
  return _images[slot].height;
};

// Shader (Private)
// ----------------------------------------------------------------------------

typedef struct _SDL_GPShader {
  SDL_GPUShader *vertex;
  SDL_GPUShader *fragment;
} _SDL_GPShader;

static Uint32 _shader_initialized = 0;
static _SDL_GPShader *_shaders = NULL;
static SDL_GPPool *_shader_pool = NULL;
static SDL_GPUDevice *_shader_gpu_device = NULL;

// Setup shader resources management.
static void _SDL_GPShaderSetup(SDL_GPUDevice *gpu_device) {
  SDL_assert(_shader_initialized == 0);
  SDL_assert(gpu_device);

  _shader_initialized = _SDL_GP_INIT_COOKIE;
  _shader_gpu_device = gpu_device;

  _shader_pool = SDL_GPCeatePool(SDL_GP_SHADER_MAX);
  _shaders =
      (_SDL_GPShader *)SDL_malloc(SDL_GP_SHADER_MAX * sizeof(_SDL_GPShader));
}

// Shutdown shader resources management and free resources.
static void _SDL_GPShaderShutdown() {
  SDL_assert(_shader_initialized == _SDL_GP_INIT_COOKIE);
  _shader_initialized = 0;

  SDL_GPDestroyPool(_shader_pool);
  SDL_free(_shaders);
}

SDL_GPShader SDL_GPCreateShader(SDL_GPShaderDesc *desc) {
  SDL_assert(_shader_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(desc);

  SDL_GPUShaderCreateInfo vert_shader_create_info = {
      .code_size = desc->vert_code_size,
      .code = desc->vert_code,
      .entrypoint = desc->vert_entrypoint,
      .format = desc->vert_format,
      .stage = SDL_GPU_SHADERSTAGE_VERTEX,
      .num_samplers = desc->vert_num_samplers,
      .num_storage_textures = desc->vert_num_storage_textures,
      .num_storage_buffers = desc->vert_num_storage_buffers,
      .num_uniform_buffers = desc->vert_num_uniform_buffers,
  };

  // Create the shader from the bytecode
  SDL_GPUShader *vert_shader =
      SDL_CreateGPUShader(_shader_gpu_device, &vert_shader_create_info);

  if (!vert_shader) {
    // TODO SDL_GPSetError(SDL_GP_ERROR_SHADER_CREATE);
    return (SDL_GPShader){.id = SDL_GP_INVALID_ID};
  }

  SDL_GPUShaderCreateInfo frag_shader_create_info = {
      .code_size = desc->frag_code_size,
      .code = desc->frag_code,
      .entrypoint = desc->frag_entrypoint,
      .format = desc->frag_format,
      .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
      .num_samplers = desc->frag_num_samplers,
      .num_storage_textures = desc->frag_num_storage_textures,
      .num_storage_buffers = desc->frag_num_storage_buffers,
      .num_uniform_buffers = desc->frag_num_uniform_buffers,
  };

  // Create the shader from the bytecode
  SDL_GPUShader *frag_shader =
      SDL_CreateGPUShader(_shader_gpu_device, &frag_shader_create_info);

  if (!frag_shader) {
    SDL_ReleaseGPUShader(_shader_gpu_device, vert_shader);
    // TODO SDL_GPSetError(SDL_GP_ERROR_SHADER_CREATE);
    return (SDL_GPShader){.id = SDL_GP_INVALID_ID};
  }

  int slot = SDL_GPAcquirePoolSlot(_shader_pool);
  if (slot == SDL_GP_POOL_INVALID_SLOT) {
    SDL_ReleaseGPUShader(_shader_gpu_device, vert_shader);
    SDL_ReleaseGPUShader(_shader_gpu_device, frag_shader);
    // TODO SDL_GPSetError(SDL_GP_ERROR_SHADER_CREATE);
    return (SDL_GPShader){.id = SDL_GP_INVALID_ID};
  }

  _shaders[slot] = (_SDL_GPShader){
      .vertex = vert_shader,
      .fragment = frag_shader,
  };

  return (SDL_GPShader){.id = SDL_GPGeneratePoolId(_shader_pool, slot)};
}

void SDL_GPDestroyShader(SDL_GPShader shader) {
  SDL_assert(_shader_initialized == _SDL_GP_INIT_COOKIE);

  if (shader.id == SDL_GP_INVALID_ID) {
    return;
  }

  int slot = SDL_GPPoolIdToSlot(shader.id);

  _SDL_GPShader inner_shader = _shaders[slot];

  SDL_ReleaseGPUShader(_shader_gpu_device, inner_shader.vertex);
  SDL_ReleaseGPUShader(_shader_gpu_device, inner_shader.fragment);

  SDL_GPReleasePoolSlot(_shader_pool, slot);

  _shaders[slot] = (_SDL_GPShader){
      .vertex = NULL,
      .fragment = NULL,
  };
}

SDL_GPUShader *SDL_GPGetGPUVertexShader(SDL_GPShader shader) {
  SDL_assert(_shader_initialized == _SDL_GP_INIT_COOKIE);

  if (shader.id == SDL_GP_INVALID_ID) {
    return NULL;
  }

  int slot = SDL_GPPoolIdToSlot(shader.id);
  return _shaders[slot].vertex;
}

SDL_GPUShader *SDL_GPGetGPUFragmentShader(SDL_GPShader shader) {
  SDL_assert(_shader_initialized == _SDL_GP_INIT_COOKIE);

  if (shader.id == SDL_GP_INVALID_ID) {
    return NULL;
  }

  int slot = SDL_GPPoolIdToSlot(shader.id);
  return _shaders[slot].fragment;
}

// Pipeline (Private)
// ----------------------------------------------------------------------------

typedef struct _SDL_GPPipeline {
  SDL_GPUGraphicsPipeline *pipeline;
} _SDL_GPPipeline;

static Uint32 _pipeline_initialized = 0;
static _SDL_GPPipeline *_pipelines = NULL;
static SDL_GPPool *_pipeline_pool = NULL;
static SDL_GPUDevice *_pipeline_gpu_device = NULL;
static SDL_Window *_pipeline_window = NULL;

// Setup pipeline resources management.
static void _SDL_GPPipelineSetup(SDL_GPUDevice *gpu_device,
                                 SDL_Window *window) {
  SDL_assert(_pipeline_initialized == 0);

  _pipeline_initialized = _SDL_GP_INIT_COOKIE;

  _pipeline_pool = SDL_GPCeatePool(SDL_GP_PIPELINE_MAX);
  _pipelines = (_SDL_GPPipeline *)SDL_malloc(SDL_GP_PIPELINE_MAX *
                                             sizeof(_SDL_GPPipeline));
}

// Shutdown pipeline resources management and free resources.
static void _SDL_GPPipelineShutdown() {
  SDL_assert(_pipeline_initialized == _SDL_GP_INIT_COOKIE);
  _pipeline_initialized = 0;

  SDL_GPDestroyPool(_pipeline_pool);
  SDL_free(_pipelines);
}

static SDL_GPUColorTargetBlendState
_SDL_GPPipelineBlendState(SDL_GPBlendMode blend_mode) {
  SDL_GPUColorTargetBlendState blend;

  SDL_memset(&blend, 0, sizeof(SDL_GPUColorTargetBlendState));

  switch (blend_mode) {
  case SDL_GP_BLENDMODE_BLEND:
    blend.enable_blend = true;
    blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.color_blend_op = SDL_GPU_BLENDOP_ADD;
    blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    break;
  case SDL_GP_BLENDMODE_BLEND_PREMULTIPLIED:
    blend.enable_blend = true;
    blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.color_blend_op = SDL_GPU_BLENDOP_ADD;
    blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    break;
  case SDL_GP_BLENDMODE_ADD:
    blend.enable_blend = true;
    blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.color_blend_op = SDL_GPU_BLENDOP_ADD;
    blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    break;
  case SDL_GP_BLENDMODE_ADD_PREMULTIPLIED:
    blend.enable_blend = true;
    blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.color_blend_op = SDL_GPU_BLENDOP_ADD;
    blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    break;
  case SDL_GP_BLENDMODE_MOD:
    blend.enable_blend = true;
    blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_DST_COLOR;
    blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    blend.color_blend_op = SDL_GPU_BLENDOP_ADD;
    blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    break;
  case SDL_GP_BLENDMODE_MUL:
    blend.enable_blend = true;
    blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_DST_COLOR;
    blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.color_blend_op = SDL_GPU_BLENDOP_ADD;
    blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_DST_ALPHA;
    blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    break;
  default: // SDL_GP_BLENDMODE_NONE
    blend.enable_blend = false;
    blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    blend.color_blend_op = SDL_GPU_BLENDOP_ADD;
    blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    blend.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    break;
  }

  return blend;
}

SDL_GPPipeline SDL_GPCreatePipeline(SDL_GPShader shader,
                                    SDL_GPPrimitiveType primitive_type,
                                    SDL_GPBlendMode blend_mode) {
  SDL_assert(_pipeline_initialized == _SDL_GP_INIT_COOKIE);

  SDL_GPUVertexBufferDescription vertex_buffer_description[1] = {{
      .slot = 0,
      .pitch = sizeof(SDL_GPVertex),
      .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
      .instance_step_rate = 0,
  }};

  SDL_GPUVertexAttribute vertex_attributes[2] = {
      {.location = 0,
       .buffer_slot = 0,
       .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
       .offset = (Uint32)SDL_GP_OFFSET_OF(SDL_GPVertex, position)},
      {.location = 1,
       .buffer_slot = 0,
       .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
       .offset = (Uint32)SDL_GP_OFFSET_OF(SDL_GPVertex, color)}};

  SDL_GPUVertexInputState vertex_input_state = {
      .vertex_buffer_descriptions = vertex_buffer_description,
      .num_vertex_buffers = 1,
      .vertex_attributes = vertex_attributes,
      .num_vertex_attributes = 2,
  };

  SDL_GPUColorTargetBlendState blend_state =
      _SDL_GPPipelineBlendState(blend_mode);

  SDL_GPUColorTargetDescription color_target_description[1] = {
      {.format = SDL_GetGPUSwapchainTextureFormat(_pipeline_gpu_device,
                                                  _pipeline_window),
       .blend_state = blend_state}};

  SDL_GPUGraphicsPipelineTargetInfo target_info = {
      .color_target_descriptions = color_target_description,
      .num_color_targets = 1,
  };

  SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info = {
      .vertex_shader = SDL_GPGetGPUVertexShader(shader),
      .fragment_shader = SDL_GPGetGPUFragmentShader(shader),
      .vertex_input_state = vertex_input_state,
      .primitive_type = (SDL_GPUPrimitiveType)primitive_type,
      .target_info = target_info,
  };

  SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(
      _pipeline_gpu_device, &pipeline_create_info);

  if (pipeline == NULL) {
    // TODO SDL_GPSetError(SDL_GP_ERROR_PIPELINE_CREATE);
    return (SDL_GPPipeline){.id = SDL_GP_INVALID_ID};
  }

  int pipeline_slot = SDL_GPAcquirePoolSlot(_pipeline_pool);
  if (pipeline_slot == SDL_GP_POOL_INVALID_SLOT) {
    // TODO SDL_GPSetError(SDL_GP_ERROR_PIPELINE_CREATE);
    return (SDL_GPPipeline){.id = SDL_GP_INVALID_ID};
  }

  _pipelines[pipeline_slot] = (_SDL_GPPipeline){
      .pipeline = pipeline,
  };

  return (SDL_GPPipeline){
      .id = SDL_GPGeneratePoolId(_pipeline_pool, pipeline_slot)};
}

void SDL_GPPipelineDestroy(SDL_GPPipeline pipeline) {
  SDL_assert(_pipeline_initialized == _SDL_GP_INIT_COOKIE);

  if (pipeline.id == SDL_GP_INVALID_ID) {
    return;
  }

  int slot = SDL_GPPoolIdToSlot(pipeline.id);

  SDL_GPReleasePoolSlot(_pipeline_pool, slot);

  _pipelines[slot] = (_SDL_GPPipeline){
      .pipeline = NULL,
  };
}

SDL_GPUGraphicsPipeline *SDL_GPGetGPUPipeline(SDL_GPPipeline pipeline) {
  SDL_assert(_pipeline_initialized == _SDL_GP_INIT_COOKIE);

  if (pipeline.id == SDL_GP_INVALID_ID) {
    return NULL;
  }

  int slot = SDL_GPPoolIdToSlot(pipeline.id);
  return _pipelines[slot].pipeline;
};

// Painter (Private)
// ----------------------------------------------------------------------------

// TODO add the code of the glsl shader

size_t _shader_vert_spv_len = 1234; // Exact byte length
Uint8 _shader_vert_spv[] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x07, 0x00,
                            0x72, 0x65, 0x6e, 0x64, 0x00, 0x00};

size_t _shader_farg_spv_len = 1234; // Exact byte length
Uint8 _shader_frag_spv[] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x07, 0x00,
                            0x72, 0x65, 0x6e, 0x64, 0x00, 0x00};

// TODO add others shader bytformats (msl, dxil, dxbc) for testing and fallback

typedef enum {
  _SDL_GP_COMMAND_NONE = 0,
  _SDL_GP_COMMAND_DRAW,
  _SDL_GP_COMMAND_VIEWPORT,
  _SDL_GPCOMMAND_SCISSOR
} _SDL_GPCommandType;

typedef struct _SDL_GPRegion {
  float x1, y1, x2, y2;
} _SDL_GPRegion;

typedef struct _SDL_GPDrawArgs {
  _SDL_GPRegion region;
  SDL_GPPipeline pipeline;
  SDL_GPTextureUniform texture;
  Uint32 uniform_index;
  Uint32 vertex_index;
  Uint32 vertices_count;
} _SDL_GPDrawArgs;

typedef union _SDL_GPCommandArgs {
  _SDL_GPDrawArgs draw;
  SDL_GPRect viewport;
  SDL_GPRect scissor;
} _SDL_GPCommandArgs;

typedef struct _SDL_GPCommand {
  _SDL_GPCommandType cmd;
  _SDL_GPCommandArgs args;
} _SDL_GPCommand;

typedef struct _SDL_GP {
  SDL_GPDesc desc;
  SDL_GPUTransferBuffer *vertex_transfer_buffer;
  SDL_GPUBuffer *vertex_data_buffer;
  SDL_GPShader shader;
  SDL_GPPipeline pipelines[SDL_GP_PRIMITIVE_SIZE * SDL_GP_BLENDMODE_SIZE];
  SDL_GPUSampler *nearest_samplers;
  SDL_PixelFormat pixel_format;
  SDL_GPImage white_image;

  // States stack
  Uint32 current_state;
  SDL_GPState states[SDL_GP_STATE_MAX];
  SDL_GPState state;

  // Transforms stack
  Uint32 current_transform;
  SDL_GPMat2x3 transforms[SDL_GP_TRANSFORMS_MAX];

  // Vertecies stack
  Uint32 current_vertex;
  SDL_GPVertex vertices[SDL_GP_VERTICES_MAX];

  // Uniforms stack
  Uint32 current_uniform;
  SDL_GPUniform uniforms[SDL_GP_COMMANDS_MAX];

  // Commands management
  Uint32 current_command;
  _SDL_GPCommand commands[SDL_GP_COMMANDS_MAX];
} _SDL_GP;

static _SDL_GP _gp = {0};
static Uint32 _gp_initialized = 0;

// Create painter common shader.
static SDL_GPShader _SDL_GPCreateCommonShader(SDL_GPUDevice *gpu_device) {
  SDL_GPUShaderFormat supported_formats = SDL_GetGPUShaderFormats(gpu_device);
  SDL_GPUShaderFormat format;

  Uint8 *code_vert = NULL;
  size_t code_vert_size = 0;

  Uint8 *code_frag = NULL;
  size_t code_frag_size = 0;

  if (supported_formats & SDL_GPU_SHADERFORMAT_SPIRV) {
    format = SDL_GPU_SHADERFORMAT_SPIRV;

    code_vert = _shader_vert_spv;
    code_vert_size = _shader_vert_spv_len;

    code_frag = _shader_frag_spv;
    code_frag_size = _shader_farg_spv_len;
  } else if (supported_formats & SDL_GPU_SHADERFORMAT_MSL) {
    format = SDL_GPU_SHADERFORMAT_MSL;
    // TODO
  } else if (supported_formats & SDL_GPU_SHADERFORMAT_DXIL) {
    format = SDL_GPU_SHADERFORMAT_DXIL;
    // TODO
  } else if (supported_formats & SDL_GPU_SHADERFORMAT_DXBC) {
    format = SDL_GPU_SHADERFORMAT_DXBC;
    // TODO
  } else {
    // TODO SDL_GPSetError(SDL_GP_ERROR_SHADER_FORMAT_UNSUPPORTED);
    return (SDL_GPShader){.id = SDL_GP_INVALID_ID};
  }

  SDL_GPShaderDesc shader_desc = {
      // Vertex shader description
      .vert_code_size = code_vert_size,
      .vert_code = code_vert,
      .vert_entrypoint = "main",
      .vert_format = format,
      .vert_num_samplers = 0,
      .vert_num_storage_textures = 0,
      .vert_num_storage_buffers = 0,
      .vert_num_uniform_buffers = 0,

      // Fragment shader description
      .frag_code_size = code_frag_size,
      .frag_code = code_frag,
      .frag_entrypoint = "main",
      .frag_format = format,
      .frag_num_samplers = SDL_GP_TEXTURE_SLOTS_MAX,
      .frag_num_storage_textures = 0,
      .frag_num_storage_buffers = 0,
      .frag_num_uniform_buffers = 0,
  };

  return SDL_GPCreateShader(&shader_desc);
}

static SDL_GPPipeline
_SDL_GP_FindOrCreatePipeline(SDL_GPPrimitiveType primitive_type,
                             SDL_GPBlendMode blend_mode) {
  SDL_GPPipeline pipeline =
      _gp.pipelines[primitive_type * SDL_GP_BLENDMODE_SIZE + blend_mode];

  if (pipeline.id == SDL_GP_INVALID_ID) {
    pipeline = SDL_GPCreatePipeline(_gp.shader, primitive_type, blend_mode);
    _gp.pipelines[primitive_type * SDL_GP_BLENDMODE_SIZE + blend_mode] =
        pipeline;
  }

  return pipeline;
}

// TODO static functions

void SDL_GPSetup(SDL_GPDesc *desc) {
  SDL_assert(_gp_initialized == 0);
  SDL_assert(desc);

  _gp_initialized = _SDL_GP_INIT_COOKIE;
  // TODO reset error

  _gp.desc = *desc;
  _gp.desc.max_vertices =
      SDL_GP_DEFAULT(desc->max_vertices, SDL_GP_VERTICES_MAX);
  _gp.desc.max_commands =
      SDL_GP_DEFAULT(desc->max_commands, SDL_GP_COMMANDS_MAX);

  // Get Swapchain pixel format
  SDL_GPUTextureFormat texture_format =
      SDL_GetGPUSwapchainTextureFormat(_gp.desc.gpu_device, _gp.desc.window);

  SDL_PixelFormat pixel_format =
      SDL_GetPixelFormatFromGPUTextureFormat(texture_format);

  _gp.pixel_format = pixel_format;

  // Setup resources management for shaders, pipelines and images

  _SDL_GPShaderSetup(_gp.desc.gpu_device);
  _SDL_GPPipelineSetup(_gp.desc.gpu_device, _gp.desc.window);
  _SDL_GPImageSetup(_gp.desc.gpu_device, _gp.desc.cmd_buffer, _gp.pixel_format);

  // Create a white texture

  SDL_Surface *white_surface = SDL_CreateSurfaceFrom(
      2, 2, SDL_PIXELFORMAT_RGBA8888,
      (Uint32[]){0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF},
      4 * sizeof(Uint32));

  _gp.white_image = SDL_GPCreateImage(white_surface);

  SDL_DestroySurface(white_surface);

  // Create a GPU transfer buffer for vertex data

  SDL_GPUTransferBufferCreateInfo vertex_transfer_buffer_create_info = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = (Uint32)(_gp.desc.max_vertices * sizeof(SDL_GPVertex)),
  };

  _gp.vertex_transfer_buffer = SDL_CreateGPUTransferBuffer(
      desc->gpu_device, &vertex_transfer_buffer_create_info);

  if (_gp.vertex_transfer_buffer == NULL) {
    // TODO SDL_GPSetError(SDL_GP_ERROR_SETUP);
    return;
  }

  // Create a GPU buffer for vertex data

  SDL_GPUBufferCreateInfo vertex_data_buffer_create_info = {
      .size = (Uint32)(_gp.desc.max_vertices * sizeof(SDL_GPVertex)),
      .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
  };

  _gp.vertex_data_buffer =
      SDL_CreateGPUBuffer(desc->gpu_device, &vertex_data_buffer_create_info);

  if (_gp.vertex_data_buffer == NULL) {
    SDL_ReleaseGPUTransferBuffer(desc->gpu_device, _gp.vertex_transfer_buffer);
    // TODO SDL_GPSetError(SDL_GP_ERROR_SETUP);
    return;
  }

  // Create nearest sampler

  SDL_GPUSamplerCreateInfo nearest_sampler_info = {
      .min_filter = SDL_GPU_FILTER_NEAREST,
      .mag_filter = SDL_GPU_FILTER_NEAREST,
      .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
      .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
      .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
      .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
  };

  _gp.nearest_samplers =
      SDL_CreateGPUSampler(desc->gpu_device, &nearest_sampler_info);

  // Create common shader

  _gp.shader = _SDL_GPCreateCommonShader(desc->gpu_device);

  // Create common pipelines

  bool is_ok = true;
  is_ok &= _SDL_GP_FindOrCreatePipeline(SDL_GP_PRIMITIVE_POINTS,
                                        SDL_GP_BLENDMODE_NONE)
               .id != SDL_GP_INVALID_ID;
  is_ok &= _SDL_GP_FindOrCreatePipeline(SDL_GP_PRIMITIVE_POINTS,
                                        SDL_GP_BLENDMODE_BLEND)
               .id != SDL_GP_INVALID_ID;
  is_ok &= _SDL_GP_FindOrCreatePipeline(SDL_GP_PRIMITIVE_LINES,
                                        SDL_GP_BLENDMODE_NONE)
               .id != SDL_GP_INVALID_ID;
  is_ok &= _SDL_GP_FindOrCreatePipeline(SDL_GP_PRIMITIVE_LINES,
                                        SDL_GP_BLENDMODE_BLEND)
               .id != SDL_GP_INVALID_ID;
  is_ok &= _SDL_GP_FindOrCreatePipeline(SDL_GP_PRIMITIVE_LINE_STRIP,
                                        SDL_GP_BLENDMODE_NONE)
               .id != SDL_GP_INVALID_ID;
  is_ok &= _SDL_GP_FindOrCreatePipeline(SDL_GP_PRIMITIVE_LINE_STRIP,
                                        SDL_GP_BLENDMODE_BLEND)
               .id != SDL_GP_INVALID_ID;
  is_ok &= _SDL_GP_FindOrCreatePipeline(SDL_GP_PRIMITIVE_TRIANGLES,
                                        SDL_GP_BLENDMODE_NONE)
               .id != SDL_GP_INVALID_ID;
  is_ok &= _SDL_GP_FindOrCreatePipeline(SDL_GP_PRIMITIVE_TRIANGLES,
                                        SDL_GP_BLENDMODE_BLEND)
               .id != SDL_GP_INVALID_ID;

  if (!is_ok) {
    SDL_GPShutdown();
    // TODO SDL_GPSetError(SDL_GP_ERROR_SETUP);
    return;
  }
}

void SDL_GPShutdown() {
  if (_gp_initialized != _SDL_GP_INIT_COOKIE) {
    return;
  }

  // Destroy common pipelines

  for (int i = 0; i < SDL_GP_PRIMITIVE_SIZE * SDL_GP_BLENDMODE_SIZE; ++i) {
    if (_gp.pipelines[i].id != SDL_GP_INVALID_ID) {
      SDL_GPPipelineDestroy(_gp.pipelines[i]);
      _gp.pipelines[i] = (SDL_GPPipeline){.id = SDL_GP_INVALID_ID};
    }
  }

  // Destroy common shader

  if (_gp.shader.id != SDL_GP_INVALID_ID) {
    SDL_GPDestroyShader(_gp.shader);
    _gp.shader = (SDL_GPShader){.id = SDL_GP_INVALID_ID};
  }

  // Destroy nearest sampler

  if (_gp.nearest_samplers) {
    SDL_ReleaseGPUSampler(_gp.desc.gpu_device, _gp.nearest_samplers);
    _gp.nearest_samplers = NULL;
  }

  // Destroy vertex data buffer

  if (_gp.vertex_data_buffer) {
    SDL_ReleaseGPUBuffer(_gp.desc.gpu_device, _gp.vertex_data_buffer);
    _gp.vertex_data_buffer = NULL;
  }

  // Destroy vertex transfer buffer

  if (_gp.vertex_transfer_buffer) {
    SDL_ReleaseGPUTransferBuffer(_gp.desc.gpu_device,
                                 _gp.vertex_transfer_buffer);
    _gp.vertex_transfer_buffer = NULL;
  }

  // Destroy white texture

  if (_gp.white_image.id != SDL_GP_INVALID_ID) {
    SDL_GPDestroyImage(_gp.white_image);
    _gp.white_image = (SDL_GPImage){.id = SDL_GP_INVALID_ID};
  }

  // Shutdown resources management for shaders, pipelines and images
  _SDL_GPImageShutdown();
  _SDL_GPPipelineShutdown();
  _SDL_GPShaderShutdown();

  SDL_memset(&_gp, 0, sizeof(_SDL_GP));
}

void SDL_GPBegin() {
  // TODO
}

void SDL_GPFlush() {
  // TODO
}

void SDL_GPEnd() {
  // TODO
}

SDL_GPVec2 SDL_GPGetFrameSize() {
  // TODO
  return (SDL_GPVec2){0, 0};
}

void SDL_GPSetProjection(float left, float right, float bottom, float top) {
  // TODO
}

void SDL_GPResetProjection() {
  // TODO
}

void SDL_GPPushTransform() {
  // TODO
}

void SDL_GPPopTransform() {
  // TODO
}

void SDL_GPResetTransform() {
  // TODO
}

void SDL_GPTranslate(float x, float y) {
  // TODO
}

void SDL_GPRotateAt(float angle, float ax, float ay) {
  // TODO
}

void SDL_GPScale(float sx, float sy) {
  // TODO
}

void SDL_GPScaleAt(float sx, float sy, float ax, float ay) {
  // TODO
}

void SDL_GPSetPipeline(SDL_GPPipeline pipeline) {
  // TODO
}

void SDL_GPResetPipeline() {
  // TODO
}

void SDL_GPSetUniform(const void *vs_data, size_t vs_size, const void *fs_data,
                      size_t fs_size) {
  // TODO
}

void SDL_GPResetUniform() {
  // TODO
}

void SDL_GPPainterSetBlendMode(SDL_GPBlendMode blend_mode) {
  // TODO
}

void SDL_GPPainterResetBlendMode() {
  // TODO
}

void SDL_GPPainterSetColor(SDL_GPColor color) {
  // TODO
}

SDL_GPColor SDL_GPGetColor() {
  // TODO
  return (SDL_GPColor){0, 0, 0, 0};
}

void SDL_GPResetColor() {
  // TODO
}

void SDL_GPSetImage(int channel, SDL_GPImage image) {
  // TODO
}

SDL_GPImage SDL_GPGetImage(int channel) {
  // TODO
  return (SDL_GPImage){0};
}

void SDL_GPResetImage(int channel) {
  // TODO
}

void SDL_GPUnsetImage(int channel) {
  // TODO
}

void SDL_GPSetSampler(int channel, SDL_GPSampler sampler) {
  // TODO
}

void SDL_GPResetSampler(int channel) {
  // TODO
}

// Set the viewport for subsequent draw calls.
void SDL_GPViewport(float x, float y, float w, float h) {
  // TODO
}

void SDL_GPResetViewport(void) {
  // TODO
}

// Set the scissor for subsequent draw calls.
void SDL_GPScissor(float x, float y, float w, float h) {
  // TODO
}

void SDL_GPResetScissor() {
  // TODO
}

void SDL_GPClear() {
  // TODO
}

void SDL_GPDraw(SDL_GPPrimitiveType primitive_type,
                const SDL_GPVertex *vertices, Uint32 vertices_count) {
  // TODO
}

void SDL_GPDrawPoints(const SDL_GPVec2 *points, Uint32 count) {
  // TODO
}

void SDL_GPDrawPoint(SDL_GPVec2 point) {
  // TODO
}

void SDL_GPDrawLine(SDL_GPLine line) {
  // TODO
}

void SDL_GPDrawLines(const SDL_GPLine *lines, Uint32 count) {
  // TODO
}

void SDL_GPDrawLinesStrip(const SDL_GPVec2 *points, Uint32 count) {
  // TODO
}

void SDL_GPDrawTriangleFilled(SDL_GPTriangle triangle) {
  // TODO
}

void SDL_GPDrawTriangles(const SDL_GPTriangle *triangles, Uint32 count) {
  // TODO
}

void SDL_GPDrawTrianglesStrip(const SDL_GPVec2 *points, Uint32 count) {
  // TODO
}

void SDL_GPDrawRectFilled(SDL_GPRect rect) {
  // TODO
}

void SDL_GPDrawRectsFilled(const SDL_GPRect *rects, Uint32 count) {
  // TODO
}

void SDL_GPDrawRectTextured(int channel, SDL_GPTexturedRect rect) {
  // TODO
}

void SDL_GPDrawRectsTextured(int channel, const SDL_GPTexturedRect *rects,
                             Uint32 count) {
  // TODO
}

#endif // SDL_GP_IMPLEMENTATION

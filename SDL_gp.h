/**
 * TODO re-organize structures to take less memory.
 *
 * https://github.com/n67094/SDL_gp
 *
 * # SDL_gp
 *
 * > SDL_gp: A minimal and efficient 2D (g)raphics (p)ainter for SDL3.
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

#include <float.h>

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

#if defined(_MSC_VER)
#define INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define SDL_GP_INLINE static inline __attribute((always_inline))
#else
#define INLINE
#endif

#define SDL_GP_INVALID_ID 0
#define SDL_GP_IMPOSSIBLE_ID 0xFFFFFFFF

// Get a default value if the provided value is 0
#define SDL_GP_DEFAULT(val, def) (((val) == 0) ? (def) : (val))

// Get the offset of an element in a structure
#define SDL_GP_OFFSET_OF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))

#ifdef __cplusplus
extern "C"
{
#endif

  // ----------------------------------------------------------------------------
  // Public API
  // ----------------------------------------------------------------------------

  // Error handling (Public)
  // ----------------------------------------------------------------------------

  typedef enum SDL_GP_Error
  {
    SDL_GP_ERROR_NONE = 0,
    SDL_GP_ERROR_SETUP_IMAGE_FAILED,
    SDL_GP_ERROR_CREATE_IMAGE_FAILED,
    SDL_GP_ERROR_CREATE_SHADER_FAILED,
    SDL_GP_ERROR_CREATE_PIPELINE_FAILED,
    SDL_GP_ERROR_CREATE_COMMON_SHADER_FAILED,
    SDL_GP_ERROR_CREATE_WHITE_TEXTURE_FAILED,
    SDL_GP_ERROR_CREATE_TRANSFER_BUFFER_FAILED,
    SDL_GP_ERROR_CREATE_VERTEX_BUFFER_FAILED,
    SDL_GP_ERROR_CREATE_COMMON_PIPELINE_FAILED,
    SDL_GP_ERROR_PAINTER_UNIFORMS_FULL,
    SDL_GP_ERROR_PAINTER_VERTICES_FULL,
    SDL_GP_ERROR_PAINTER_COMMANDS_FULL,
    SDL_GP_ERROR_FLUSH_FAILED
  } SDL_GP_Error;

  // Get the last error that occurred in SDL_gp. Returns SDL_GP_ERROR_NONE if no
  // error has occurred.
  SDL_GP_Error SDL_GPGetLastError(void);

  // Get a human-readable string describing an SDL_GP_Error value. Returns
  // "Unknown error" if the error value is not recognized.
  const char *SDL_GPGetErrorMessage(SDL_GP_Error error);

  // Pool (Public for users who want to re-use it as-is for other resources).
  // ----------------------------------------------------------------------------
  // The pool is a simple resource management system.
  //
  // The pool work like this, there is a fixed number of slots (defined at pool
  // creation) that can be acquired and released. Each slot has an incrementing
  // generation counter, which is used to generate unique ids for each slot.
  // When a slot is released, its generation counter is incremented, so that any
  // ids generated from that slot will be invalid until the slot is acquired
  // again.

#define SDL_GP_POOL_INVALID_SLOT 0
#define SDL_GP_POOL_SLOT_SHIFT 16
#define SDL_GP_POOL_SLOT_MASK ((1 << SDL_GP_POOL_SLOT_SHIFT) - 1)

  typedef struct SDL_GPPool
  {
    size_t
        size; // total number of slots in the pool (counting the invalid slot)
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

  // Release a slot back to the pool, making it available for future
  // acquisitions.
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

  typedef enum
  {
    SDL_GP_SAMPLER_POINT_CLAMP = 0,
    SDL_GP_SAMPLER_POINT_WRAP,
    SDL_GP_SAMPLER_LINEAR_CLAMP,
    SDL_GP_SAMPLER_LINEAR_WRAP,
    SDL_GP_SAMPLER_SIZE,
  } SDL_GPSampler;

  typedef struct SDL_GPImage
  {
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

  // Shader (Public)
  // ----------------------------------------------------------------------------

  typedef struct SDL_GPShader
  {
    Uint32 id;
  } SDL_GPShader;

  typedef struct SDL_GPShaderDesc
  {
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

  // Get the GPU vertex shader associated with a shader. Returns NULL if the
  // shader is invalid.
  SDL_GPUShader *SDL_GPGetGPUVertexShader(SDL_GPShader shader);

  // Get the GPU fragment shader associated with a shader. Returns NULL if the
  // shader is invalid.
  SDL_GPUShader *SDL_GPGetGPUFragmentShader(SDL_GPShader shader);

  // Destroy a shader and free its resources.
  void SDL_GPDestroyShader(SDL_GPShader shader);

  // Pipeline (Public)
  // ----------------------------------------------------------------------------

  typedef enum
  {
    SDL_GP_BLENDMODE_NONE                = SDL_BLENDMODE_NONE,
    SDL_GP_BLENDMODE_BLEND               = SDL_BLENDMODE_BLEND,
    SDL_GP_BLENDMODE_BLEND_PREMULTIPLIED = SDL_BLENDMODE_BLEND_PREMULTIPLIED,
    SDL_GP_BLENDMODE_ADD                 = SDL_BLENDMODE_ADD,
    SDL_GP_BLENDMODE_ADD_PREMULTIPLIED   = SDL_BLENDMODE_ADD_PREMULTIPLIED,
    SDL_GP_BLENDMODE_MOD                 = SDL_BLENDMODE_MOD,
    SDL_GP_BLENDMODE_MUL                 = SDL_BLENDMODE_MUL,
    SDL_GP_BLENDMODE_SIZE                = 7,
  } SDL_GPBlendMode;

  typedef enum
  {
    SDL_GP_PRIMITIVE_TRIANGLES      = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
    SDL_GP_PRIMITIVE_TRIANGLE_STRIP = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
    SDL_GP_PRIMITIVE_LINES          = SDL_GPU_PRIMITIVETYPE_LINELIST,
    SDL_GP_PRIMITIVE_LINE_STRIP     = SDL_GPU_PRIMITIVETYPE_LINESTRIP,
    SDL_GP_PRIMITIVE_POINTS         = SDL_GPU_PRIMITIVETYPE_POINTLIST,
    SDL_GP_PRIMITIVE_SIZE           = 5,
  } SDL_GPPrimitiveType;

  typedef struct SDL_GPPipeline
  {
    Uint32 id;
  } SDL_GPPipeline;

  // Create a graphics pipeline
  SDL_GPPipeline SDL_GPCreatePipeline(SDL_GPShader shader,
                                      SDL_GPPrimitiveType primitive_type,
                                      SDL_GPBlendMode blend_mode);

  // Destroy a graphics pipeline and free its resources.
  void SDL_GPPipelineDestroy(SDL_GPPipeline pipeline);

  // Get the GPU graphics pipeline associated with a SDL_gp pipeline. Returns
  // NULL if the pipeline is invalid.
  SDL_GPUGraphicsPipeline *SDL_GPGetGPUPipeline(SDL_GPPipeline pipeline);

  // Painter (Public)
  // ----------------------------------------------------------------------------

  typedef enum
  {
    SDL_GP_UNIFORM_SLOT_VS = 0,
    SDL_GP_UNIFORM_SLOT_FS = 1
  } SDL_GPUniformSlot;

  typedef struct SDL_GPISize
  {
    int w, h;
  } SDL_GPISize;

  typedef struct SDL_GPVec2
  {
    float x, y;
  } SDL_GPVec2;

  typedef SDL_GPVec2 SDL_GPPoint;

  typedef struct SDL_GPLine
  {
    SDL_GPPoint a, b;
  } SDL_GPLine;

  typedef struct SDL_GPTriangle
  {
    SDL_GPPoint a, b, c;
  } SDL_GPTriangle;

  typedef struct SDL_GPIRect
  {
    int x, y, w, h;
  } SDL_GPIRect;

  typedef struct SDL_GPRect
  {
    float x, y, w, h;
  } SDL_GPRect;

  typedef struct SDL_GPTexturedRect
  {
    SDL_GPRect dst;
    SDL_GPRect src;
  } SDL_GPTexturedRect;

  typedef struct SDL_GPMat2x3
  {
    float m00, m01, m02;
    float m10, m11, m12;
  } SDL_GPMat2x3;

// Create an immutable 2x3 matrix
#define SDL_GPCreateMat2x3(m00, m01, m02, m10, m11, m12)                       \
  ((const SDL_GPMat2x3){ m00, m01, m02, m10, m11, m12 })

// Create an immutable identity 2x3 matrix
#define SDL_GPCreateMat2x3Identity()                                           \
  ((const SDL_GPMat2x3){ 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f })

  typedef struct SDL_GPVertex
  {
    SDL_GPVec2 position;
    SDL_GPVec2 texcoord;
    SDL_Color color;
  } SDL_GPVertex;

  typedef union SDL_GPUniformData
  {
    float floats[SDL_GP_UNIFORM_FLOATS_MAX];
    uint8_t bytes[SDL_GP_UNIFORM_FLOATS_MAX * sizeof(float)];
  } SDL_GPUniformData;

  typedef struct SDL_GPUniform
  {
    Uint16 vs_size;
    Uint16 fs_size;
    SDL_GPUniformData data;
  } SDL_GPUniform;

  typedef struct SDL_GPTextureUniform
  {
    Uint32 count;
    SDL_GPImage images[SDL_GP_TEXTURE_SLOTS_MAX];
    SDL_GPUSampler *samplers[SDL_GP_TEXTURE_SLOTS_MAX];
  } SDL_GPTextureUniform;

  typedef struct SDL_GPState
  {
    SDL_GPMat2x3 projection;
    SDL_GPMat2x3 transform;
    SDL_GPMat2x3 mvp;
    SDL_GPTextureUniform texture;
    SDL_GPUniform uniform;
    SDL_GPPipeline pipeline;
    SDL_GPBlendMode blend_mode;
    SDL_GPISize frame_size;
    SDL_GPIRect viewport;
    SDL_GPIRect scissor;
    SDL_Color color;
    float thickness;
    Uint32 base_uniform;
    Uint32 base_vertex;
    Uint32 base_command;
  } SDL_GPState;

  typedef struct SDL_GPDesc
  {
    Uint32 max_vertices;
    Uint32 max_commands;
    SDL_Window *window;
    SDL_GPUDevice *gpu_device;
  } SDL_GPDesc;

  // Update the swapchain texture for the current frame. This should be called
  // after acquiring a new swapchain texture and before issuing any draw calls
  // that use it.
  void SDL_GPUpdateSwapchainTexture(SDL_GPUTexture *swapchain_texture);

  // Update the command buffer for the current frame. This should be called
  // after acquiring a new command buffer and before issuing any draw calls.
  void SDL_GPUpdateCommandBuffer(SDL_GPUCommandBuffer *cmd_buffer);

  // Setup SDL_GP context.
  void SDL_GPSetup(SDL_GPDesc *desc);

  // Shutdown SDL_GP context.
  void SDL_GPShutdown();

  // Begin recoarding draw calls for the current frame. This should be called
  // after setting up SDL_gp and acquiring a swapchain texture and command
  // buffer for the current frame.
  void SDL_GPBegin(int width, int height);

  // Flush the recorded draw calls to the GPU.
  void SDL_GPFlush(SDL_GPUTexture *swapchain_texture);

  // End recording draw calls for the current frame.
  void SDL_GPEnd(void);

  // Set the coordinate space boundaries in the current viewport.
  void SDL_GPSetProjection(float left, float right, float bottom, float top);

  // Reset the projection to the default coordinate space, which is the
  // coordinate of the current viewport.
  void SDL_GPResetProjection(void);

  // Save the current transform matrix on the transform stack. To be pop later
  // with SDL_GPPopTransform.
  void SDL_GPPushTransform(void);

  // Restore the transform matrix from the top of the transform stack.
  void SDL_GPPopTransform(void);

  // Set the current transform matrix to identity (no transformation).
  void SDL_GPResetTransform(void);

  // Translates the 2D coordinates space.
  void SDL_GPTranslate(float x, float y);

  // Rotates the 2D coordinate space around the origin.
  void SDL_GPRotate(float angle);

  // Rotates the 2D coordinate space around a point.
  void SDL_GPRotateAt(float angle, float ax, float ay);

  // Scales the 2D coordinate space around the origin.
  void SDL_GPScale(float sx, float sy);

  // Scales the 2D coordinate space around a point.
  void SDL_GPScaleAt(float sx, float sy, float ax, float ay);

  // Set the current graphics pipeline.
  void SDL_GPSetPipeline(SDL_GPPipeline pipeline);

  // Reset the graphics pipeline to the default pipeline builtin pipeline.
  void SDL_GPResetPipeline(void);

  // Set uniform data for the current pipeline.
  void SDL_GPSetUniform(const void *vs_data,
                        size_t vs_size,
                        const void *fs_data,
                        size_t fs_size);

  // Reset uniform data to the default state (current state color).
  void SDL_GPResetUniform(void);

  // Set the current blend mode.
  void SDL_GPSetBlendMode(SDL_GPBlendMode blend_mode);

  // Reset the current blend mode to the default blend mode (no blending).
  void SDL_GPResetBlendMode(void);

  // Sets current color.
  void SDL_GPSetColor(SDL_Color color);

  // Gets current color.
  SDL_Color SDL_GPGetColor(void);

  // Reset current color to the default color (white).
  void SDL_GPResetColor(void);

  // Sets current bound image in a texture channel.
  void SDL_GPSetImage(int channel, SDL_GPImage image);

  // Remove current bound image from a texture channel (no texture).
  void SDL_GPUnsetImage(int channel);

  // Reset current bound image in a texture channel to the default (white
  // texture).
  void SDL_GPResetImage(int channel);

  // Set current bound sampler in a texture channel.
  void SDL_GPSetSampler(int channel, SDL_GPUSampler *sampler);

  // Remove current bound sampler from a texture channel (no sampler).
  void SDL_GPUnsetSampler(int channel);

  // Reset current bound sampler in a texture channel to default (nearest
  // sampler).
  void SDL_GPResetSampler(int channel);

  // Set the screen are to draw to.
  void SDL_GPViewport(int x, int y, int w, int h);

  // Reset the viewport to default (0, 0, width, height).
  void SDL_GPResetViewport(void);

  // Set the clipping rectangle in the viewport.
  void SDL_GPScissor(int x, int y, int w, int h);

  // Reset the clipping rectangle to default (viewport bounds).
  void SDL_GPResetScissor(void);

  // Reset all state to default.
  void SDL_GPResetState(void);

  // Clear the current viewport with the current color.
  void SDL_GPClear(void);

  // Draw any primitive.
  void SDL_GPDraw(SDL_GPPrimitiveType primitive_type,
                  const SDL_GPVertex *vertices,
                  Uint32 vertices_count);

  // Draw points in batch.
  void SDL_GPDrawPoints(const SDL_GPVec2 *points, Uint32 count);

  // Draw a single point.
  void SDL_GPDrawPoint(SDL_GPVec2 point);

  // Draw lines in batch.
  void SDL_GPDrawLines(const SDL_GPLine *lines, Uint32 count);

  // Draw a single line.
  void SDL_GPDrawLine(SDL_GPLine line);

  // Draw a stip of lines.
  void SDL_GPDrawLinesStrip(const SDL_GPVec2 *points, Uint32 count);

  // Draw triangles in batch.
  void SDL_GPDrawFilledTriangles(const SDL_GPTriangle *triangles, Uint32 count);

  // Draw a single triangle.
  void SDL_GPDrawFilledTriangle(SDL_GPTriangle triangle);

  // Draw a strip of triangles.
  void SDL_GPDrawFilledTrianglesStrip(const SDL_GPVec2 *points, Uint32 count);

  // Draw rectangles in batch.
  void SDL_GPDrawFilledRects(const SDL_GPRect *rects, Uint32 count);

  // Draw a single rectangle.
  void SDL_GPDrawFilledRect(SDL_GPRect rect);

  // Draw textured rectangles in batch.
  void SDL_GPDrawTexturedRects(int channel,
                               const SDL_GPTexturedRect *rects,
                               Uint32 count);

  // Draw a single textured rectangle.
  void SDL_GPDrawTexturedRect(int channel, SDL_GPTexturedRect rect);

#ifdef __cplusplus
}
#endif

#endif // SDL_GP_H_

// ----------------------------------------------------------------------------
// Implementation and internal API
// ----------------------------------------------------------------------------

#ifdef SDL_GP_IMPLEMENTATION

#define _SDL_GP_INIT_COOKIE 0xC0DED1ED

static SDL_GPUTexture *_swapchain_texture;
static SDL_GPUCommandBuffer *_cmd_buffer;

// Error handling (Private)
// ----------------------------------------------------------------------------

static SDL_GP_Error _last_error = SDL_GP_ERROR_NONE;

static void
_SDL_GPSetError(SDL_GP_Error error)
{
  SDL_LogError(
      SDL_LOG_CATEGORY_VIDEO, "SDL_gp error: %s", SDL_GPGetErrorMessage(error));
  _last_error = error;
}

SDL_GP_Error
SDL_GPGetLastError(void)
{
  return _last_error;
}

const char *
SDL_GPGetErrorMessage(SDL_GP_Error error)
{
  switch (error) {
  case SDL_GP_ERROR_NONE:
    return "No error";
  case SDL_GP_ERROR_SETUP_IMAGE_FAILED:
    return "Failed to setup image resources";
  case SDL_GP_ERROR_CREATE_IMAGE_FAILED:
    return "Failed to create image";
  case SDL_GP_ERROR_CREATE_SHADER_FAILED:
    return "Failed to create shader";
  case SDL_GP_ERROR_CREATE_PIPELINE_FAILED:
    return "Failed to create pipeline";
  case SDL_GP_ERROR_CREATE_COMMON_SHADER_FAILED:
    return "Failed to create common shader";
  case SDL_GP_ERROR_CREATE_WHITE_TEXTURE_FAILED:
    return "Failed to create white texture";
  case SDL_GP_ERROR_CREATE_TRANSFER_BUFFER_FAILED:
    return "Failed to create transfer buffer";
  case SDL_GP_ERROR_CREATE_VERTEX_BUFFER_FAILED:
    return "Failed to create vertex buffer";
  case SDL_GP_ERROR_CREATE_COMMON_PIPELINE_FAILED:
    return "Failed to create common pipeline";
  case SDL_GP_ERROR_PAINTER_UNIFORMS_FULL:
    return "Painter uniforms are full";
  case SDL_GP_ERROR_PAINTER_VERTICES_FULL:
    return "Painter vertices are full";
  case SDL_GP_ERROR_PAINTER_COMMANDS_FULL:
    return "Painter commands are full";
  case SDL_GP_ERROR_FLUSH_FAILED:
    return "Failed to flush painter";
  default:
    return "Unknown error";
  }
}

// Pool (Private)
// ----------------------------------------------------------------------------

SDL_GPPool *
SDL_GPCeatePool(size_t number_of_slots)
{
  SDL_GPPool *pool = (SDL_GPPool *)SDL_malloc(sizeof(SDL_GPPool));

  // the 0 slot is used for invalid ids, so we need to add 1 to the size to
  // account for it
  pool->size       = number_of_slots + 1;
  pool->queue_top  = 0;
  pool->counters   = (Uint32 *)SDL_malloc(pool->size * sizeof(Uint32));
  pool->free_queue = (int *)SDL_malloc(pool->size * sizeof(int));

  // Initialize the free queue with all the slots (except 0) and set the
  // generation counters to 0
  for (int i = pool->size - 1; i >= 1; --i) {
    pool->free_queue[pool->queue_top++] = i;
    pool->counters[i]                   = 0;
  }

  return pool;
}

void
SDL_GPDestroyPool(SDL_GPPool *pool)
{
  SDL_free(pool->counters);
  SDL_free(pool->free_queue);
  SDL_free(pool);
}

int
SDL_GPAcquirePoolSlot(SDL_GPPool *pool)
{
  SDL_assert(pool);
  SDL_assert(pool->free_queue);

  if (pool->queue_top > 0) {
    return pool
        ->free_queue[--pool->queue_top]; // Get a slot from the free queue
  } else {
    return SDL_GP_POOL_INVALID_SLOT; // No more slots available
  }
}

void
SDL_GPReleasePoolSlot(SDL_GPPool *pool, int slot)
{
  SDL_assert(slot > SDL_GP_POOL_INVALID_SLOT && slot < (int)pool->size);
  SDL_assert(pool);
  SDL_assert(pool->free_queue);
  SDL_assert(pool->queue_top < (int)pool->size);

  pool->free_queue[pool->queue_top++] = slot;

  SDL_assert(pool->queue_top <= (int)pool->size);
}

Uint32
SDL_GPGeneratePoolId(SDL_GPPool *pool, int slot)
{
  SDL_assert(slot > SDL_GP_POOL_INVALID_SLOT && slot < (int)pool->size);
  SDL_assert(pool);
  SDL_assert(pool->counters);

  uint32_t counter = ++pool->counters[slot]; // increment generation

  Uint32 id
      = (counter << SDL_GP_POOL_SLOT_SHIFT) | (slot & SDL_GP_POOL_SLOT_MASK);

  return id;
}

int
SDL_GPPoolIdToSlot(Uint32 id)
{
  int slot_index = (int)(id & SDL_GP_POOL_SLOT_MASK);
  return slot_index;
  return -1;
}

// Image (Private)
// ----------------------------------------------------------------------------

typedef struct _SDL_GPImage
{
  SDL_GPUTexture *texture;
  int width;
  int height;
} _SDL_GPImage;

static Uint32 _image_initialized                             = 0;
static _SDL_GPImage *_images                                 = NULL;
static SDL_GPPool *_image_pool                               = NULL;
static SDL_GPUTransferBuffer *_image_texture_transfer_buffer = NULL;
static SDL_GPUDevice *_image_gpu_device                      = NULL;
static SDL_Window *_image_window                             = NULL;

// Setup image resources management.
static void
_SDL_GPImageSetup(SDL_GPUDevice *gpu_device, SDL_Window *window)
{
  SDL_assert(_image_initialized == 0);
  SDL_assert(gpu_device);

  _image_initialized = _SDL_GP_INIT_COOKIE;

  _image_gpu_device = gpu_device;
  _image_window     = window;

  _image_pool = SDL_GPCeatePool(SDL_GP_IMAGE_MAX);

  _images = (_SDL_GPImage *)SDL_malloc(SDL_GP_IMAGE_MAX * sizeof(_SDL_GPImage));

  SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info
      = { .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
          .size
          = 4 * SDL_GP_TEXTURE_DIMENSION_MAX * SDL_GP_TEXTURE_DIMENSION_MAX };

  _image_texture_transfer_buffer
      = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_buffer_create_info);

  if (!_image_texture_transfer_buffer) {
    _SDL_GPSetError(SDL_GP_ERROR_SETUP_IMAGE_FAILED);
    return;
  }
}

// Shutdown image resources management and free resources.
static void
_SDL_GPImageShutdown()
{
  SDL_assert(_image_initialized == _SDL_GP_INIT_COOKIE);
  _image_initialized = 0;

  SDL_GPDestroyPool(_image_pool);
  SDL_free(_images);
  SDL_ReleaseGPUTransferBuffer(_image_gpu_device,
                               _image_texture_transfer_buffer);
}

SDL_GPImage
SDL_GPCreateImage(SDL_Surface *surface)
{
  SDL_assert(_image_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_cmd_buffer);
  SDL_assert(surface);

  SDL_Surface *inner_surface = surface;

  SDL_GPUTextureFormat texture_format
      = SDL_GetGPUSwapchainTextureFormat(_image_gpu_device, _image_window);
  SDL_PixelFormat pixel_format
      = SDL_GetPixelFormatFromGPUTextureFormat(texture_format);

  // Convert the surface to the swapchain texture format if needed

  bool converted = false;
  if (surface->format != pixel_format) {

    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "Converting image pixel format from %s to %s",
                SDL_GetPixelFormatName(surface->format),
                SDL_GetPixelFormatName(pixel_format));

    inner_surface = SDL_ConvertSurface(surface, pixel_format);

    if (inner_surface == NULL) {
      _SDL_GPSetError(SDL_GP_ERROR_CREATE_IMAGE_FAILED);
      return (SDL_GPImage){ .id = SDL_GP_INVALID_ID };
    }

    converted = true;
  }

  // Transfer surface pixels to the GPU transfer buffer

  void *texture_transfer_ptr = SDL_MapGPUTransferBuffer(
      _image_gpu_device, _image_texture_transfer_buffer, true);

  const SDL_PixelFormatDetails *format_details
      = SDL_GetPixelFormatDetails(inner_surface->format);

  SDL_assert(inner_surface->w * inner_surface->h
                 * format_details->bytes_per_pixel
             <= SDL_GP_TEXTURE_DIMENSION_MAX * SDL_GP_TEXTURE_DIMENSION_MAX
                    * format_details->bytes_per_pixel);

  SDL_memcpy(texture_transfer_ptr,
             inner_surface->pixels,
             inner_surface->w * inner_surface->h
                 * format_details->bytes_per_pixel);

  SDL_UnmapGPUTransferBuffer(_image_gpu_device, _image_texture_transfer_buffer);

  // Create GPU texture and copy the transfer buffer to it

  SDL_GPUTextureCreateInfo texture_create_info
      = { .type                 = SDL_GPU_TEXTURETYPE_2D,
          .format               = texture_format,
          .width                = (Uint32)inner_surface->w,
          .height               = (Uint32)inner_surface->h,
          .layer_count_or_depth = 1,
          .num_levels           = 1,
          .usage                = SDL_GPU_TEXTUREUSAGE_SAMPLER };

  SDL_GPUTexture *texture
      = SDL_CreateGPUTexture(_image_gpu_device, &texture_create_info);

  if (texture == NULL) {
    _SDL_GPSetError(SDL_GP_ERROR_CREATE_IMAGE_FAILED);
    return (SDL_GPImage){ .id = SDL_GP_INVALID_ID };
  }

  // Copy the texture data from the transfer buffer to the GPU texture using a
  // copy pass

  SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(_cmd_buffer);

  SDL_GPUTextureTransferInfo transfer_info = {
    .transfer_buffer = _image_texture_transfer_buffer,
    .offset          = 0,
  };

  SDL_GPUTextureRegion region = { .texture = texture,
                                  .w       = (Uint32)inner_surface->w,
                                  .h       = (Uint32)inner_surface->h,
                                  .d       = 1 };

  SDL_UploadToGPUTexture(copy_pass, &transfer_info, &region, false);

  SDL_EndGPUCopyPass(copy_pass);

  // Allocate image from resource

  int slot = SDL_GPAcquirePoolSlot(_image_pool);
  if (slot == SDL_GP_POOL_INVALID_SLOT) {
    SDL_ReleaseGPUTexture(_image_gpu_device, texture);
    _SDL_GPSetError(SDL_GP_ERROR_CREATE_IMAGE_FAILED);
    return (SDL_GPImage){ .id = SDL_GP_INVALID_ID };
  }

  _images[slot] = (_SDL_GPImage){
    .texture = texture,
    .width   = inner_surface->w,
    .height  = inner_surface->h,
  };

  // Destroy the converted surface if we created one

  if (converted) {
    SDL_DestroySurface(inner_surface);
  }

  return (SDL_GPImage){ .id = SDL_GPGeneratePoolId(_image_pool, slot) };
}

void
SDL_GPDestroyImage(SDL_GPImage image)
{
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
    .width   = 0,
    .height  = 0,
  };
}

SDL_GPUTexture *
SDL_GPGetImageGPUTexture(SDL_GPImage image)
{
  SDL_assert(_image_initialized == _SDL_GP_INIT_COOKIE);

  if (image.id == SDL_GP_INVALID_ID) {
    return NULL;
  }

  int slot = SDL_GPPoolIdToSlot(image.id);
  return _images[slot].texture;
}

int
SDL_GPGetImageWidth(SDL_GPImage image)
{
  SDL_assert(_image_initialized == _SDL_GP_INIT_COOKIE);

  if (image.id == SDL_GP_INVALID_ID) {
    return 0;
  }

  int slot = SDL_GPPoolIdToSlot(image.id);
  return _images[slot].width;
}

int
SDL_GPGetImageHeight(SDL_GPImage image)
{
  SDL_assert(_image_initialized == _SDL_GP_INIT_COOKIE);

  if (image.id == SDL_GP_INVALID_ID) {
    return 0;
  }

  int slot = SDL_GPPoolIdToSlot(image.id);
  return _images[slot].height;
};

// Shader (Private)
// ----------------------------------------------------------------------------

typedef struct _SDL_GPShader
{
  SDL_GPUShader *vertex;
  SDL_GPUShader *fragment;
} _SDL_GPShader;

static Uint32 _shader_initialized        = 0;
static _SDL_GPShader *_shaders           = NULL;
static SDL_GPPool *_shader_pool          = NULL;
static SDL_GPUDevice *_shader_gpu_device = NULL;

// Setup shader resources management.
static void
_SDL_GPShaderSetup(SDL_GPUDevice *gpu_device)
{
  SDL_assert(_shader_initialized == 0);
  SDL_assert(gpu_device);

  _shader_initialized = _SDL_GP_INIT_COOKIE;
  _shader_gpu_device  = gpu_device;

  _shader_pool = SDL_GPCeatePool(SDL_GP_SHADER_MAX);
  _shaders
      = (_SDL_GPShader *)SDL_malloc(SDL_GP_SHADER_MAX * sizeof(_SDL_GPShader));
}

// Shutdown shader resources management and free resources.
static void
_SDL_GPShaderShutdown()
{
  SDL_assert(_shader_initialized == _SDL_GP_INIT_COOKIE);
  _shader_initialized = 0;

  SDL_GPDestroyPool(_shader_pool);
  SDL_free(_shaders);
}

SDL_GPShader
SDL_GPCreateShader(SDL_GPShaderDesc *desc)
{
  SDL_assert(_shader_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(desc);

  SDL_GPUShaderCreateInfo vert_shader_create_info = {
    .code_size            = desc->vert_code_size,
    .code                 = desc->vert_code,
    .entrypoint           = desc->vert_entrypoint,
    .format               = desc->vert_format,
    .stage                = SDL_GPU_SHADERSTAGE_VERTEX,
    .num_samplers         = desc->vert_num_samplers,
    .num_storage_textures = desc->vert_num_storage_textures,
    .num_storage_buffers  = desc->vert_num_storage_buffers,
    .num_uniform_buffers  = desc->vert_num_uniform_buffers,
  };

  // Create the shader from the bytecode
  SDL_GPUShader *vert_shader
      = SDL_CreateGPUShader(_shader_gpu_device, &vert_shader_create_info);

  if (!vert_shader) {
    _SDL_GPSetError(SDL_GP_ERROR_CREATE_SHADER_FAILED);
    return (SDL_GPShader){ .id = SDL_GP_INVALID_ID };
  }

  SDL_GPUShaderCreateInfo frag_shader_create_info = {
    .code_size            = desc->frag_code_size,
    .code                 = desc->frag_code,
    .entrypoint           = desc->frag_entrypoint,
    .format               = desc->frag_format,
    .stage                = SDL_GPU_SHADERSTAGE_FRAGMENT,
    .num_samplers         = desc->frag_num_samplers,
    .num_storage_textures = desc->frag_num_storage_textures,
    .num_storage_buffers  = desc->frag_num_storage_buffers,
    .num_uniform_buffers  = desc->frag_num_uniform_buffers,
  };

  // Create the shader from the bytecode
  SDL_GPUShader *frag_shader
      = SDL_CreateGPUShader(_shader_gpu_device, &frag_shader_create_info);

  if (!frag_shader) {
    SDL_ReleaseGPUShader(_shader_gpu_device, vert_shader);
    _SDL_GPSetError(SDL_GP_ERROR_CREATE_SHADER_FAILED);
    return (SDL_GPShader){ .id = SDL_GP_INVALID_ID };
  }

  int slot = SDL_GPAcquirePoolSlot(_shader_pool);
  if (slot == SDL_GP_POOL_INVALID_SLOT) {
    SDL_ReleaseGPUShader(_shader_gpu_device, vert_shader);
    SDL_ReleaseGPUShader(_shader_gpu_device, frag_shader);
    _SDL_GPSetError(SDL_GP_ERROR_CREATE_SHADER_FAILED);
    return (SDL_GPShader){ .id = SDL_GP_INVALID_ID };
  }

  _shaders[slot] = (_SDL_GPShader){
    .vertex   = vert_shader,
    .fragment = frag_shader,
  };

  return (SDL_GPShader){ .id = SDL_GPGeneratePoolId(_shader_pool, slot) };
}

void
SDL_GPDestroyShader(SDL_GPShader shader)
{
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
    .vertex   = NULL,
    .fragment = NULL,
  };
}

SDL_GPUShader *
SDL_GPGetGPUVertexShader(SDL_GPShader shader)
{
  SDL_assert(_shader_initialized == _SDL_GP_INIT_COOKIE);

  if (shader.id == SDL_GP_INVALID_ID) {
    return NULL;
  }

  int slot = SDL_GPPoolIdToSlot(shader.id);
  return _shaders[slot].vertex;
}

SDL_GPUShader *
SDL_GPGetGPUFragmentShader(SDL_GPShader shader)
{
  SDL_assert(_shader_initialized == _SDL_GP_INIT_COOKIE);

  if (shader.id == SDL_GP_INVALID_ID) {
    return NULL;
  }

  int slot = SDL_GPPoolIdToSlot(shader.id);
  return _shaders[slot].fragment;
}

// Pipeline (Private)
// ----------------------------------------------------------------------------

typedef struct _SDL_GPPipeline
{
  SDL_GPUGraphicsPipeline *pipeline;
} _SDL_GPPipeline;

static Uint32 _pipeline_initialized        = 0;
static _SDL_GPPipeline *_pipelines         = NULL;
static SDL_GPPool *_pipeline_pool          = NULL;
static SDL_GPUDevice *_pipeline_gpu_device = NULL;
static SDL_Window *_pipeline_window        = NULL;

// Setup pipeline resources management.
static void
_SDL_GPPipelineSetup(SDL_GPUDevice *gpu_device, SDL_Window *window)
{
  SDL_assert(_pipeline_initialized == 0);
  SDL_assert(gpu_device);
  SDL_assert(window);

  _pipeline_initialized = _SDL_GP_INIT_COOKIE;
  _pipeline_gpu_device  = gpu_device;
  _pipeline_window      = window;

  _pipeline_pool = SDL_GPCeatePool(SDL_GP_PIPELINE_MAX);
  _pipelines     = (_SDL_GPPipeline *)SDL_malloc(SDL_GP_PIPELINE_MAX
                                                 * sizeof(_SDL_GPPipeline));
}

// Shutdown pipeline resources management and free resources.
static void
_SDL_GPPipelineShutdown()
{
  SDL_assert(_pipeline_initialized == _SDL_GP_INIT_COOKIE);
  _pipeline_initialized = 0;

  SDL_GPDestroyPool(_pipeline_pool);
  SDL_free(_pipelines);
}

static SDL_GPUColorTargetBlendState
_SDL_GPPipelineBlendState(SDL_GPBlendMode blend_mode)
{
  SDL_GPUColorTargetBlendState blend;

  SDL_memset(&blend, 0, sizeof(SDL_GPUColorTargetBlendState));

  switch (blend_mode) {
  case SDL_GP_BLENDMODE_BLEND:
    blend.enable_blend          = true;
    blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;
    break;
  case SDL_GP_BLENDMODE_BLEND_PREMULTIPLIED:
    blend.enable_blend          = true;
    blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;
    break;
  case SDL_GP_BLENDMODE_ADD:
    blend.enable_blend          = true;
    blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;
    break;
  case SDL_GP_BLENDMODE_ADD_PREMULTIPLIED:
    blend.enable_blend          = true;
    blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;
    break;
  case SDL_GP_BLENDMODE_MOD:
    blend.enable_blend          = true;
    blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_DST_COLOR;
    blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    blend.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;
    break;
  case SDL_GP_BLENDMODE_MUL:
    blend.enable_blend          = true;
    blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_DST_COLOR;
    blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_DST_ALPHA;
    blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;
    break;
  default: // SDL_GP_BLENDMODE_NONE
    blend.enable_blend          = false;
    blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    blend.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    blend.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;
    break;
  }

  return blend;
}

SDL_GPPipeline
SDL_GPCreatePipeline(SDL_GPShader shader,
                     SDL_GPPrimitiveType primitive_type,
                     SDL_GPBlendMode blend_mode)
{
  SDL_assert(_pipeline_initialized == _SDL_GP_INIT_COOKIE);

  SDL_GPUVertexBufferDescription vertex_buffer_description[1] = { {
      .slot               = 0,
      .pitch              = sizeof(SDL_GPVertex),
      .input_rate         = SDL_GPU_VERTEXINPUTRATE_VERTEX,
      .instance_step_rate = 0,
  } };

  SDL_GPUVertexAttribute vertex_attributes[2]
      = { { .location    = 0,
            .buffer_slot = 0,
            .format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset      = (Uint32)SDL_GP_OFFSET_OF(SDL_GPVertex, position) },
          { .location    = 1,
            .buffer_slot = 0,
            .format      = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
            .offset      = (Uint32)SDL_GP_OFFSET_OF(SDL_GPVertex, color) } };

  SDL_GPUVertexInputState vertex_input_state = {
    .vertex_buffer_descriptions = vertex_buffer_description,
    .num_vertex_buffers         = 1,
    .vertex_attributes          = vertex_attributes,
    .num_vertex_attributes      = 2,
  };

  SDL_GPUColorTargetBlendState blend_state
      = _SDL_GPPipelineBlendState(blend_mode);

  SDL_GPUColorTargetDescription color_target_description[1]
      = { { .format = SDL_GetGPUSwapchainTextureFormat(_pipeline_gpu_device,
                                                       _pipeline_window),
            .blend_state = blend_state } };

  SDL_GPUGraphicsPipelineTargetInfo target_info = {
    .color_target_descriptions = color_target_description,
    .num_color_targets         = 1,
  };

  SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info = {
    .vertex_shader      = SDL_GPGetGPUVertexShader(shader),
    .fragment_shader    = SDL_GPGetGPUFragmentShader(shader),
    .vertex_input_state = vertex_input_state,
    .primitive_type     = (SDL_GPUPrimitiveType)primitive_type,
    .target_info        = target_info,
  };

  SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(
      _pipeline_gpu_device, &pipeline_create_info);

  if (pipeline == NULL) {
    _SDL_GPSetError(SDL_GP_ERROR_CREATE_PIPELINE_FAILED);
    return (SDL_GPPipeline){ .id = SDL_GP_INVALID_ID };
  }

  int pipeline_slot = SDL_GPAcquirePoolSlot(_pipeline_pool);
  if (pipeline_slot == SDL_GP_POOL_INVALID_SLOT) {
    _SDL_GPSetError(SDL_GP_ERROR_CREATE_PIPELINE_FAILED);
    return (SDL_GPPipeline){ .id = SDL_GP_INVALID_ID };
  }

  _pipelines[pipeline_slot] = (_SDL_GPPipeline){
    .pipeline = pipeline,
  };

  return (SDL_GPPipeline){ .id = SDL_GPGeneratePoolId(_pipeline_pool,
                                                      pipeline_slot) };
}

void
SDL_GPPipelineDestroy(SDL_GPPipeline pipeline)
{
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

SDL_GPUGraphicsPipeline *
SDL_GPGetGPUPipeline(SDL_GPPipeline pipeline)
{
  SDL_assert(_pipeline_initialized == _SDL_GP_INIT_COOKIE);

  if (pipeline.id == SDL_GP_INVALID_ID) {
    return NULL;
  }

  int slot = SDL_GPPoolIdToSlot(pipeline.id);
  return _pipelines[slot].pipeline;
};

// Painter (Private)
// ----------------------------------------------------------------------------

/*
#version 450 core

layout(location=0) in vec4 coord;
layout(location=1) in vec4 color;

layout(location=0) out vec2 tex_uv;
layout(location=1) out vec4 i_color;

void main() {
  gl_Position = vec4(coord.xy, 0.0, 1.0);
  gl_PointSize = 1.0;

  tex_uv = coord.zw;
  i_color = color;
}
*/

size_t _shader_vert_spv_len = 1168;
Uint8 _shader_vert_spv[]    = {
  0X03, 0X02, 0X23, 0X07, 0X00, 0X00, 0X01, 0X00, 0X0B, 0X00, 0X08, 0X00, 0X26,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X11, 0X00, 0X02, 0X00, 0X01, 0X00,
  0X00, 0X00, 0X0B, 0X00, 0X06, 0X00, 0X01, 0X00, 0X00, 0X00, 0X47, 0X4C, 0X53,
  0X4C, 0X2E, 0X73, 0X74, 0X64, 0X2E, 0X34, 0X35, 0X30, 0X00, 0X00, 0X00, 0X00,
  0X0E, 0X00, 0X03, 0X00, 0X00, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X0F,
  0X00, 0X0A, 0X00, 0X00, 0X00, 0X00, 0X00, 0X04, 0X00, 0X00, 0X00, 0X6D, 0X61,
  0X69, 0X6E, 0X00, 0X00, 0X00, 0X00, 0X0D, 0X00, 0X00, 0X00, 0X11, 0X00, 0X00,
  0X00, 0X20, 0X00, 0X00, 0X00, 0X23, 0X00, 0X00, 0X00, 0X24, 0X00, 0X00, 0X00,
  0X03, 0X00, 0X03, 0X00, 0X02, 0X00, 0X00, 0X00, 0XC2, 0X01, 0X00, 0X00, 0X05,
  0X00, 0X04, 0X00, 0X04, 0X00, 0X00, 0X00, 0X6D, 0X61, 0X69, 0X6E, 0X00, 0X00,
  0X00, 0X00, 0X05, 0X00, 0X06, 0X00, 0X0B, 0X00, 0X00, 0X00, 0X67, 0X6C, 0X5F,
  0X50, 0X65, 0X72, 0X56, 0X65, 0X72, 0X74, 0X65, 0X78, 0X00, 0X00, 0X00, 0X00,
  0X06, 0X00, 0X06, 0X00, 0X0B, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X67,
  0X6C, 0X5F, 0X50, 0X6F, 0X73, 0X69, 0X74, 0X69, 0X6F, 0X6E, 0X00, 0X06, 0X00,
  0X07, 0X00, 0X0B, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X67, 0X6C, 0X5F,
  0X50, 0X6F, 0X69, 0X6E, 0X74, 0X53, 0X69, 0X7A, 0X65, 0X00, 0X00, 0X00, 0X00,
  0X06, 0X00, 0X07, 0X00, 0X0B, 0X00, 0X00, 0X00, 0X02, 0X00, 0X00, 0X00, 0X67,
  0X6C, 0X5F, 0X43, 0X6C, 0X69, 0X70, 0X44, 0X69, 0X73, 0X74, 0X61, 0X6E, 0X63,
  0X65, 0X00, 0X06, 0X00, 0X07, 0X00, 0X0B, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00,
  0X00, 0X67, 0X6C, 0X5F, 0X43, 0X75, 0X6C, 0X6C, 0X44, 0X69, 0X73, 0X74, 0X61,
  0X6E, 0X63, 0X65, 0X00, 0X05, 0X00, 0X03, 0X00, 0X0D, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X05, 0X00, 0X04, 0X00, 0X11, 0X00, 0X00, 0X00, 0X63, 0X6F,
  0X6F, 0X72, 0X64, 0X00, 0X00, 0X00, 0X05, 0X00, 0X04, 0X00, 0X20, 0X00, 0X00,
  0X00, 0X74, 0X65, 0X78, 0X5F, 0X75, 0X76, 0X00, 0X00, 0X05, 0X00, 0X04, 0X00,
  0X23, 0X00, 0X00, 0X00, 0X69, 0X5F, 0X63, 0X6F, 0X6C, 0X6F, 0X72, 0X00, 0X05,
  0X00, 0X04, 0X00, 0X24, 0X00, 0X00, 0X00, 0X63, 0X6F, 0X6C, 0X6F, 0X72, 0X00,
  0X00, 0X00, 0X47, 0X00, 0X03, 0X00, 0X0B, 0X00, 0X00, 0X00, 0X02, 0X00, 0X00,
  0X00, 0X48, 0X00, 0X05, 0X00, 0X0B, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X0B, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X48, 0X00, 0X05, 0X00, 0X0B,
  0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X0B, 0X00, 0X00, 0X00, 0X01, 0X00,
  0X00, 0X00, 0X48, 0X00, 0X05, 0X00, 0X0B, 0X00, 0X00, 0X00, 0X02, 0X00, 0X00,
  0X00, 0X0B, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00, 0X00, 0X48, 0X00, 0X05, 0X00,
  0X0B, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00, 0X00, 0X0B, 0X00, 0X00, 0X00, 0X04,
  0X00, 0X00, 0X00, 0X47, 0X00, 0X04, 0X00, 0X11, 0X00, 0X00, 0X00, 0X1E, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X47, 0X00, 0X04, 0X00, 0X20, 0X00, 0X00,
  0X00, 0X1E, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X47, 0X00, 0X04, 0X00,
  0X23, 0X00, 0X00, 0X00, 0X1E, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X47,
  0X00, 0X04, 0X00, 0X24, 0X00, 0X00, 0X00, 0X1E, 0X00, 0X00, 0X00, 0X01, 0X00,
  0X00, 0X00, 0X13, 0X00, 0X02, 0X00, 0X02, 0X00, 0X00, 0X00, 0X21, 0X00, 0X03,
  0X00, 0X03, 0X00, 0X00, 0X00, 0X02, 0X00, 0X00, 0X00, 0X16, 0X00, 0X03, 0X00,
  0X06, 0X00, 0X00, 0X00, 0X20, 0X00, 0X00, 0X00, 0X17, 0X00, 0X04, 0X00, 0X07,
  0X00, 0X00, 0X00, 0X06, 0X00, 0X00, 0X00, 0X04, 0X00, 0X00, 0X00, 0X15, 0X00,
  0X04, 0X00, 0X08, 0X00, 0X00, 0X00, 0X20, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X2B, 0X00, 0X04, 0X00, 0X08, 0X00, 0X00, 0X00, 0X09, 0X00, 0X00, 0X00,
  0X01, 0X00, 0X00, 0X00, 0X1C, 0X00, 0X04, 0X00, 0X0A, 0X00, 0X00, 0X00, 0X06,
  0X00, 0X00, 0X00, 0X09, 0X00, 0X00, 0X00, 0X1E, 0X00, 0X06, 0X00, 0X0B, 0X00,
  0X00, 0X00, 0X07, 0X00, 0X00, 0X00, 0X06, 0X00, 0X00, 0X00, 0X0A, 0X00, 0X00,
  0X00, 0X0A, 0X00, 0X00, 0X00, 0X20, 0X00, 0X04, 0X00, 0X0C, 0X00, 0X00, 0X00,
  0X03, 0X00, 0X00, 0X00, 0X0B, 0X00, 0X00, 0X00, 0X3B, 0X00, 0X04, 0X00, 0X0C,
  0X00, 0X00, 0X00, 0X0D, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00, 0X00, 0X15, 0X00,
  0X04, 0X00, 0X0E, 0X00, 0X00, 0X00, 0X20, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00,
  0X00, 0X2B, 0X00, 0X04, 0X00, 0X0E, 0X00, 0X00, 0X00, 0X0F, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X20, 0X00, 0X04, 0X00, 0X10, 0X00, 0X00, 0X00, 0X01,
  0X00, 0X00, 0X00, 0X07, 0X00, 0X00, 0X00, 0X3B, 0X00, 0X04, 0X00, 0X10, 0X00,
  0X00, 0X00, 0X11, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X17, 0X00, 0X04,
  0X00, 0X12, 0X00, 0X00, 0X00, 0X06, 0X00, 0X00, 0X00, 0X02, 0X00, 0X00, 0X00,
  0X2B, 0X00, 0X04, 0X00, 0X06, 0X00, 0X00, 0X00, 0X15, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X2B, 0X00, 0X04, 0X00, 0X06, 0X00, 0X00, 0X00, 0X16, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X80, 0X3F, 0X20, 0X00, 0X04, 0X00, 0X1A, 0X00, 0X00,
  0X00, 0X03, 0X00, 0X00, 0X00, 0X07, 0X00, 0X00, 0X00, 0X2B, 0X00, 0X04, 0X00,
  0X0E, 0X00, 0X00, 0X00, 0X1C, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X20,
  0X00, 0X04, 0X00, 0X1D, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00, 0X00, 0X06, 0X00,
  0X00, 0X00, 0X20, 0X00, 0X04, 0X00, 0X1F, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00,
  0X00, 0X12, 0X00, 0X00, 0X00, 0X3B, 0X00, 0X04, 0X00, 0X1F, 0X00, 0X00, 0X00,
  0X20, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00, 0X00, 0X3B, 0X00, 0X04, 0X00, 0X1A,
  0X00, 0X00, 0X00, 0X23, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00, 0X00, 0X3B, 0X00,
  0X04, 0X00, 0X10, 0X00, 0X00, 0X00, 0X24, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00,
  0X00, 0X36, 0X00, 0X05, 0X00, 0X02, 0X00, 0X00, 0X00, 0X04, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00, 0X00, 0XF8, 0X00, 0X02, 0X00, 0X05,
  0X00, 0X00, 0X00, 0X3D, 0X00, 0X04, 0X00, 0X07, 0X00, 0X00, 0X00, 0X13, 0X00,
  0X00, 0X00, 0X11, 0X00, 0X00, 0X00, 0X4F, 0X00, 0X07, 0X00, 0X12, 0X00, 0X00,
  0X00, 0X14, 0X00, 0X00, 0X00, 0X13, 0X00, 0X00, 0X00, 0X13, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X51, 0X00, 0X05, 0X00, 0X06,
  0X00, 0X00, 0X00, 0X17, 0X00, 0X00, 0X00, 0X14, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X51, 0X00, 0X05, 0X00, 0X06, 0X00, 0X00, 0X00, 0X18, 0X00, 0X00,
  0X00, 0X14, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X50, 0X00, 0X07, 0X00,
  0X07, 0X00, 0X00, 0X00, 0X19, 0X00, 0X00, 0X00, 0X17, 0X00, 0X00, 0X00, 0X18,
  0X00, 0X00, 0X00, 0X15, 0X00, 0X00, 0X00, 0X16, 0X00, 0X00, 0X00, 0X41, 0X00,
  0X05, 0X00, 0X1A, 0X00, 0X00, 0X00, 0X1B, 0X00, 0X00, 0X00, 0X0D, 0X00, 0X00,
  0X00, 0X0F, 0X00, 0X00, 0X00, 0X3E, 0X00, 0X03, 0X00, 0X1B, 0X00, 0X00, 0X00,
  0X19, 0X00, 0X00, 0X00, 0X41, 0X00, 0X05, 0X00, 0X1D, 0X00, 0X00, 0X00, 0X1E,
  0X00, 0X00, 0X00, 0X0D, 0X00, 0X00, 0X00, 0X1C, 0X00, 0X00, 0X00, 0X3E, 0X00,
  0X03, 0X00, 0X1E, 0X00, 0X00, 0X00, 0X16, 0X00, 0X00, 0X00, 0X3D, 0X00, 0X04,
  0X00, 0X07, 0X00, 0X00, 0X00, 0X21, 0X00, 0X00, 0X00, 0X11, 0X00, 0X00, 0X00,
  0X4F, 0X00, 0X07, 0X00, 0X12, 0X00, 0X00, 0X00, 0X22, 0X00, 0X00, 0X00, 0X21,
  0X00, 0X00, 0X00, 0X21, 0X00, 0X00, 0X00, 0X02, 0X00, 0X00, 0X00, 0X03, 0X00,
  0X00, 0X00, 0X3E, 0X00, 0X03, 0X00, 0X20, 0X00, 0X00, 0X00, 0X22, 0X00, 0X00,
  0X00, 0X3D, 0X00, 0X04, 0X00, 0X07, 0X00, 0X00, 0X00, 0X25, 0X00, 0X00, 0X00,
  0X24, 0X00, 0X00, 0X00, 0X3E, 0X00, 0X03, 0X00, 0X23, 0X00, 0X00, 0X00, 0X25,
  0X00, 0X00, 0X00, 0XFD, 0X00, 0X01, 0X00, 0X38, 0X00, 0X01, 0X00
};

unsigned int _shader_vert_msl_len = 567;
unsigned char _shader_vert_msl[]  = {
  0X23, 0X69, 0X6E, 0X63, 0X6C, 0X75, 0X64, 0X65, 0X20, 0X3C, 0X6D, 0X65, 0X74,
  0X61, 0X6C, 0X5F, 0X73, 0X74, 0X64, 0X6C, 0X69, 0X62, 0X3E, 0X0A, 0X23, 0X69,
  0X6E, 0X63, 0X6C, 0X75, 0X64, 0X65, 0X20, 0X3C, 0X73, 0X69, 0X6D, 0X64, 0X2F,
  0X73, 0X69, 0X6D, 0X64, 0X2E, 0X68, 0X3E, 0X0A, 0X0A, 0X75, 0X73, 0X69, 0X6E,
  0X67, 0X20, 0X6E, 0X61, 0X6D, 0X65, 0X73, 0X70, 0X61, 0X63, 0X65, 0X20, 0X6D,
  0X65, 0X74, 0X61, 0X6C, 0X3B, 0X0A, 0X0A, 0X73, 0X74, 0X72, 0X75, 0X63, 0X74,
  0X20, 0X6D, 0X61, 0X69, 0X6E, 0X30, 0X5F, 0X6F, 0X75, 0X74, 0X0A, 0X7B, 0X0A,
  0X20, 0X20, 0X20, 0X20, 0X66, 0X6C, 0X6F, 0X61, 0X74, 0X32, 0X20, 0X74, 0X65,
  0X78, 0X5F, 0X75, 0X76, 0X20, 0X5B, 0X5B, 0X75, 0X73, 0X65, 0X72, 0X28, 0X6C,
  0X6F, 0X63, 0X6E, 0X30, 0X29, 0X5D, 0X5D, 0X3B, 0X0A, 0X20, 0X20, 0X20, 0X20,
  0X66, 0X6C, 0X6F, 0X61, 0X74, 0X34, 0X20, 0X69, 0X5F, 0X63, 0X6F, 0X6C, 0X6F,
  0X72, 0X20, 0X5B, 0X5B, 0X75, 0X73, 0X65, 0X72, 0X28, 0X6C, 0X6F, 0X63, 0X6E,
  0X31, 0X29, 0X5D, 0X5D, 0X3B, 0X0A, 0X20, 0X20, 0X20, 0X20, 0X66, 0X6C, 0X6F,
  0X61, 0X74, 0X34, 0X20, 0X67, 0X6C, 0X5F, 0X50, 0X6F, 0X73, 0X69, 0X74, 0X69,
  0X6F, 0X6E, 0X20, 0X5B, 0X5B, 0X70, 0X6F, 0X73, 0X69, 0X74, 0X69, 0X6F, 0X6E,
  0X5D, 0X5D, 0X3B, 0X0A, 0X20, 0X20, 0X20, 0X20, 0X66, 0X6C, 0X6F, 0X61, 0X74,
  0X20, 0X67, 0X6C, 0X5F, 0X50, 0X6F, 0X69, 0X6E, 0X74, 0X53, 0X69, 0X7A, 0X65,
  0X20, 0X5B, 0X5B, 0X70, 0X6F, 0X69, 0X6E, 0X74, 0X5F, 0X73, 0X69, 0X7A, 0X65,
  0X5D, 0X5D, 0X3B, 0X0A, 0X7D, 0X3B, 0X0A, 0X0A, 0X73, 0X74, 0X72, 0X75, 0X63,
  0X74, 0X20, 0X6D, 0X61, 0X69, 0X6E, 0X30, 0X5F, 0X69, 0X6E, 0X0A, 0X7B, 0X0A,
  0X20, 0X20, 0X20, 0X20, 0X66, 0X6C, 0X6F, 0X61, 0X74, 0X34, 0X20, 0X63, 0X6F,
  0X6F, 0X72, 0X64, 0X20, 0X5B, 0X5B, 0X61, 0X74, 0X74, 0X72, 0X69, 0X62, 0X75,
  0X74, 0X65, 0X28, 0X30, 0X29, 0X5D, 0X5D, 0X3B, 0X0A, 0X20, 0X20, 0X20, 0X20,
  0X66, 0X6C, 0X6F, 0X61, 0X74, 0X34, 0X20, 0X63, 0X6F, 0X6C, 0X6F, 0X72, 0X20,
  0X5B, 0X5B, 0X61, 0X74, 0X74, 0X72, 0X69, 0X62, 0X75, 0X74, 0X65, 0X28, 0X31,
  0X29, 0X5D, 0X5D, 0X3B, 0X0A, 0X7D, 0X3B, 0X0A, 0X0A, 0X76, 0X65, 0X72, 0X74,
  0X65, 0X78, 0X20, 0X6D, 0X61, 0X69, 0X6E, 0X30, 0X5F, 0X6F, 0X75, 0X74, 0X20,
  0X6D, 0X61, 0X69, 0X6E, 0X30, 0X28, 0X6D, 0X61, 0X69, 0X6E, 0X30, 0X5F, 0X69,
  0X6E, 0X20, 0X69, 0X6E, 0X20, 0X5B, 0X5B, 0X73, 0X74, 0X61, 0X67, 0X65, 0X5F,
  0X69, 0X6E, 0X5D, 0X5D, 0X29, 0X0A, 0X7B, 0X0A, 0X20, 0X20, 0X20, 0X20, 0X6D,
  0X61, 0X69, 0X6E, 0X30, 0X5F, 0X6F, 0X75, 0X74, 0X20, 0X6F, 0X75, 0X74, 0X20,
  0X3D, 0X20, 0X7B, 0X7D, 0X3B, 0X0A, 0X20, 0X20, 0X20, 0X20, 0X6F, 0X75, 0X74,
  0X2E, 0X67, 0X6C, 0X5F, 0X50, 0X6F, 0X73, 0X69, 0X74, 0X69, 0X6F, 0X6E, 0X20,
  0X3D, 0X20, 0X66, 0X6C, 0X6F, 0X61, 0X74, 0X34, 0X28, 0X69, 0X6E, 0X2E, 0X63,
  0X6F, 0X6F, 0X72, 0X64, 0X2E, 0X78, 0X79, 0X2C, 0X20, 0X30, 0X2E, 0X30, 0X2C,
  0X20, 0X31, 0X2E, 0X30, 0X29, 0X3B, 0X0A, 0X20, 0X20, 0X20, 0X20, 0X6F, 0X75,
  0X74, 0X2E, 0X67, 0X6C, 0X5F, 0X50, 0X6F, 0X69, 0X6E, 0X74, 0X53, 0X69, 0X7A,
  0X65, 0X20, 0X3D, 0X20, 0X31, 0X2E, 0X30, 0X3B, 0X0A, 0X20, 0X20, 0X20, 0X20,
  0X6F, 0X75, 0X74, 0X2E, 0X74, 0X65, 0X78, 0X5F, 0X75, 0X76, 0X20, 0X3D, 0X20,
  0X69, 0X6E, 0X2E, 0X63, 0X6F, 0X6F, 0X72, 0X64, 0X2E, 0X7A, 0X77, 0X3B, 0X0A,
  0X20, 0X20, 0X20, 0X20, 0X6F, 0X75, 0X74, 0X2E, 0X69, 0X5F, 0X63, 0X6F, 0X6C,
  0X6F, 0X72, 0X20, 0X3D, 0X20, 0X69, 0X6E, 0X2E, 0X63, 0X6F, 0X6C, 0X6F, 0X72,
  0X3B, 0X0A, 0X20, 0X20, 0X20, 0X20, 0X72, 0X65, 0X74, 0X75, 0X72, 0X6E, 0X20,
  0X6F, 0X75, 0X74, 0X3B, 0X0A, 0X7D, 0X0A, 0X0A
};

unsigned int _shader_vert_dxil_len = 3280;
unsigned char _shader_vert_dxil[]  = {
  0X44, 0X58, 0X42, 0X43, 0X24, 0X6B, 0X07, 0X1C, 0XEF, 0X19, 0X41, 0XFB, 0XF1,
  0X66, 0XD1, 0XEA, 0X57, 0XB4, 0X9E, 0XED, 0X01, 0X00, 0X00, 0X00, 0XD0, 0X0C,
  0X00, 0X00, 0X07, 0X00, 0X00, 0X00, 0X3C, 0X00, 0X00, 0X00, 0X4C, 0X00, 0X00,
  0X00, 0XA8, 0X00, 0X00, 0X00, 0X30, 0X01, 0X00, 0X00, 0X24, 0X02, 0X00, 0X00,
  0X1C, 0X07, 0X00, 0X00, 0X38, 0X07, 0X00, 0X00, 0X53, 0X46, 0X49, 0X30, 0X08,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X49, 0X53,
  0X47, 0X31, 0X54, 0X00, 0X00, 0X00, 0X02, 0X00, 0X00, 0X00, 0X08, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X48, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X0F,
  0X0F, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X48, 0X00,
  0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00,
  0X00, 0X01, 0X00, 0X00, 0X00, 0X0F, 0X0F, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X54, 0X45, 0X58, 0X43, 0X4F, 0X4F, 0X52, 0X44, 0X00, 0X00, 0X00, 0X00, 0X4F,
  0X53, 0X47, 0X31, 0X80, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00, 0X00, 0X08, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X68, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X03, 0X0C, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X68,
  0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X03, 0X00,
  0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X0F, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X71, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X01, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00, 0X00, 0X02, 0X00, 0X00, 0X00, 0X0F,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X54, 0X45, 0X58, 0X43, 0X4F, 0X4F,
  0X52, 0X44, 0X00, 0X53, 0X56, 0X5F, 0X50, 0X6F, 0X73, 0X69, 0X74, 0X69, 0X6F,
  0X6E, 0X00, 0X00, 0X00, 0X00, 0X50, 0X53, 0X56, 0X30, 0XEC, 0X00, 0X00, 0X00,
  0X34, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0XFF, 0XFF,
  0XFF, 0XFF, 0X01, 0X00, 0X00, 0X00, 0X02, 0X03, 0X00, 0X02, 0X03, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X25, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X2C, 0X00, 0X00, 0X00, 0X00,
  0X54, 0X45, 0X58, 0X43, 0X4F, 0X4F, 0X52, 0X44, 0X00, 0X54, 0X45, 0X58, 0X43,
  0X4F, 0X4F, 0X52, 0X44, 0X00, 0X54, 0X45, 0X58, 0X43, 0X4F, 0X4F, 0X52, 0X44,
  0X00, 0X54, 0X45, 0X58, 0X43, 0X4F, 0X4F, 0X52, 0X44, 0X00, 0X6D, 0X61, 0X69,
  0X6E, 0X00, 0X00, 0X00, 0X02, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X01,
  0X00, 0X00, 0X00, 0X10, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X01, 0X00, 0X44, 0X00, 0X03, 0X00, 0X00, 0X00, 0X0A, 0X00, 0X00,
  0X00, 0X01, 0X00, 0X00, 0X00, 0X01, 0X01, 0X44, 0X00, 0X03, 0X00, 0X00, 0X00,
  0X13, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X01, 0X00, 0X42, 0X00, 0X03,
  0X02, 0X00, 0X00, 0X1C, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X01, 0X01,
  0X44, 0X00, 0X03, 0X02, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X01, 0X02, 0X44, 0X03, 0X03, 0X04, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00,
  0X00, 0X02, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X02, 0X00, 0X00, 0X00, 0X10,
  0X00, 0X00, 0X00, 0X20, 0X00, 0X00, 0X00, 0X40, 0X00, 0X00, 0X00, 0X80, 0X00,
  0X00, 0X00, 0X53, 0X54, 0X41, 0X54, 0XF0, 0X04, 0X00, 0X00, 0X60, 0X00, 0X01,
  0X00, 0X3C, 0X01, 0X00, 0X00, 0X44, 0X58, 0X49, 0X4C, 0X00, 0X01, 0X00, 0X00,
  0X10, 0X00, 0X00, 0X00, 0XD8, 0X04, 0X00, 0X00, 0X42, 0X43, 0XC0, 0XDE, 0X21,
  0X0C, 0X00, 0X00, 0X33, 0X01, 0X00, 0X00, 0X0B, 0X82, 0X20, 0X00, 0X02, 0X00,
  0X00, 0X00, 0X13, 0X00, 0X00, 0X00, 0X07, 0X81, 0X23, 0X91, 0X41, 0XC8, 0X04,
  0X49, 0X06, 0X10, 0X32, 0X39, 0X92, 0X01, 0X84, 0X0C, 0X25, 0X05, 0X08, 0X19,
  0X1E, 0X04, 0X8B, 0X62, 0X80, 0X10, 0X45, 0X02, 0X42, 0X92, 0X0B, 0X42, 0X84,
  0X10, 0X32, 0X14, 0X38, 0X08, 0X18, 0X4B, 0X0A, 0X32, 0X42, 0X88, 0X48, 0X90,
  0X14, 0X20, 0X43, 0X46, 0X88, 0XA5, 0X00, 0X19, 0X32, 0X42, 0XE4, 0X48, 0X0E,
  0X90, 0X11, 0X22, 0XC4, 0X50, 0X41, 0X51, 0X81, 0X8C, 0XE1, 0X83, 0XE5, 0X8A,
  0X04, 0X21, 0X46, 0X06, 0X51, 0X18, 0X00, 0X00, 0X06, 0X00, 0X00, 0X00, 0X1B,
  0X8C, 0XE0, 0XFF, 0XFF, 0XFF, 0XFF, 0X07, 0X40, 0X02, 0XA8, 0X0D, 0X84, 0XF0,
  0XFF, 0XFF, 0XFF, 0XFF, 0X03, 0X20, 0X01, 0X00, 0X00, 0X00, 0X49, 0X18, 0X00,
  0X00, 0X02, 0X00, 0X00, 0X00, 0X13, 0X82, 0X60, 0X42, 0X20, 0X00, 0X00, 0X00,
  0X89, 0X20, 0X00, 0X00, 0X0F, 0X00, 0X00, 0X00, 0X32, 0X22, 0X08, 0X09, 0X20,
  0X64, 0X85, 0X04, 0X13, 0X22, 0XA4, 0X84, 0X04, 0X13, 0X22, 0XE3, 0X84, 0XA1,
  0X90, 0X14, 0X12, 0X4C, 0X88, 0X8C, 0X0B, 0X84, 0X84, 0X4C, 0X10, 0X30, 0X23,
  0X00, 0X25, 0X00, 0X8A, 0X19, 0X80, 0X39, 0X02, 0X30, 0X98, 0X23, 0X40, 0X8A,
  0X31, 0X44, 0X54, 0X44, 0X56, 0X0C, 0X20, 0XA2, 0X1A, 0XC2, 0X81, 0X80, 0X54,
  0X20, 0X00, 0X00, 0X13, 0X14, 0X72, 0XC0, 0X87, 0X74, 0X60, 0X87, 0X36, 0X68,
  0X87, 0X79, 0X68, 0X03, 0X72, 0XC0, 0X87, 0X0D, 0XAF, 0X50, 0X0E, 0X6D, 0XD0,
  0X0E, 0X7A, 0X50, 0X0E, 0X6D, 0X00, 0X0F, 0X7A, 0X30, 0X07, 0X72, 0XA0, 0X07,
  0X73, 0X20, 0X07, 0X6D, 0X90, 0X0E, 0X71, 0XA0, 0X07, 0X73, 0X20, 0X07, 0X6D,
  0X90, 0X0E, 0X78, 0XA0, 0X07, 0X73, 0X20, 0X07, 0X6D, 0X90, 0X0E, 0X71, 0X60,
  0X07, 0X7A, 0X30, 0X07, 0X72, 0XD0, 0X06, 0XE9, 0X30, 0X07, 0X72, 0XA0, 0X07,
  0X73, 0X20, 0X07, 0X6D, 0X90, 0X0E, 0X76, 0X40, 0X07, 0X7A, 0X60, 0X07, 0X74,
  0XD0, 0X06, 0XE6, 0X10, 0X07, 0X76, 0XA0, 0X07, 0X73, 0X20, 0X07, 0X6D, 0X60,
  0X0E, 0X73, 0X20, 0X07, 0X7A, 0X30, 0X07, 0X72, 0XD0, 0X06, 0XE6, 0X60, 0X07,
  0X74, 0XA0, 0X07, 0X76, 0X40, 0X07, 0X6D, 0XE0, 0X0E, 0X78, 0XA0, 0X07, 0X71,
  0X60, 0X07, 0X7A, 0X30, 0X07, 0X72, 0XA0, 0X07, 0X76, 0X40, 0X07, 0X43, 0X9E,
  0X00, 0X08, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X86, 0X3C,
  0X06, 0X10, 0X00, 0X01, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X0C, 0X79,
  0X10, 0X20, 0X00, 0X04, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0XC8, 0X02,
  0X01, 0X0E, 0X00, 0X00, 0X00, 0X32, 0X1E, 0X98, 0X14, 0X19, 0X11, 0X4C, 0X90,
  0X8C, 0X09, 0X26, 0X47, 0XC6, 0X04, 0X43, 0XA2, 0X12, 0X18, 0X01, 0X28, 0X86,
  0X32, 0X28, 0X8F, 0X22, 0X28, 0X93, 0X82, 0X28, 0X0A, 0XAA, 0X92, 0X18, 0X01,
  0X28, 0X84, 0X22, 0X28, 0X03, 0XDA, 0XB1, 0X14, 0X84, 0X01, 0X11, 0X10, 0X0A,
  0X04, 0X08, 0X0C, 0X40, 0X01, 0X24, 0X00, 0X00, 0X00, 0X79, 0X18, 0X00, 0X00,
  0X66, 0X00, 0X00, 0X00, 0X1A, 0X03, 0X4C, 0X90, 0X46, 0X02, 0X13, 0XC4, 0X31,
  0X20, 0XC3, 0X1B, 0X43, 0X81, 0X93, 0X4B, 0XB3, 0X0B, 0XA3, 0X2B, 0X4B, 0X01,
  0X89, 0X71, 0XC9, 0X71, 0X81, 0X71, 0XA9, 0X89, 0XC1, 0XA1, 0X01, 0X41, 0X91,
  0X89, 0X21, 0X93, 0XC1, 0X31, 0XBB, 0X91, 0XB9, 0X49, 0XD9, 0X10, 0X04, 0X13,
  0X04, 0X62, 0X98, 0X20, 0X10, 0XC4, 0X06, 0X61, 0X20, 0X36, 0X08, 0X04, 0X41,
  0XC1, 0X6E, 0X6E, 0X82, 0X40, 0X14, 0X1B, 0X86, 0X03, 0X21, 0X26, 0X08, 0X02,
  0XB0, 0X01, 0XD8, 0X30, 0X10, 0XCB, 0XB2, 0X21, 0X60, 0X36, 0X0C, 0X83, 0XD2,
  0X4C, 0X10, 0X96, 0X68, 0X43, 0XF0, 0X90, 0X68, 0X0B, 0X4B, 0X73, 0X23, 0X42,
  0X55, 0X84, 0X35, 0XF4, 0XF4, 0X24, 0X45, 0X34, 0X41, 0X28, 0X98, 0X09, 0X42,
  0XD1, 0X6C, 0X08, 0X88, 0X09, 0X42, 0XE1, 0X4C, 0X10, 0X08, 0X63, 0X82, 0X40,
  0X1C, 0X1B, 0X84, 0X0B, 0XDB, 0XB0, 0X10, 0XD2, 0X44, 0X55, 0XD4, 0X60, 0X11,
  0X54, 0XB6, 0X21, 0X18, 0X36, 0X2C, 0X83, 0X34, 0X51, 0X1B, 0X35, 0X58, 0X03,
  0X95, 0X6D, 0X10, 0X34, 0X6E, 0X82, 0X50, 0X3C, 0X1B, 0X84, 0XEB, 0XDA, 0XB0,
  0X10, 0XD2, 0X44, 0X55, 0XDE, 0XE0, 0X11, 0XD4, 0XB7, 0X61, 0X19, 0XA4, 0X89,
  0XDA, 0XBC, 0XC1, 0X1A, 0XA8, 0X6C, 0X82, 0X40, 0X20, 0X5C, 0XA6, 0XAC, 0XBE,
  0XA0, 0XDE, 0XE6, 0XD2, 0XE8, 0XD2, 0XDE, 0XDC, 0X26, 0X08, 0X05, 0XB4, 0X61,
  0X11, 0X83, 0X31, 0X98, 0XC8, 0XA0, 0XB2, 0X06, 0X4B, 0X0C, 0XA8, 0X6C, 0XC3,
  0X00, 0X06, 0X61, 0X50, 0X06, 0X1B, 0X86, 0XCE, 0X0C, 0X80, 0X0D, 0X85, 0X12,
  0X9D, 0X01, 0X00, 0XB0, 0X48, 0X73, 0X9B, 0XA3, 0X9B, 0X9B, 0X20, 0X10, 0X09,
  0X8D, 0XB9, 0XB4, 0XB3, 0X2F, 0X36, 0XB2, 0X09, 0X02, 0XA1, 0XD0, 0X98, 0X4B,
  0X3B, 0XFB, 0X9A, 0XA3, 0X9B, 0X20, 0X10, 0XCB, 0X06, 0X23, 0X0D, 0XD4, 0X60,
  0X0D, 0XD8, 0XA0, 0X0D, 0XDC, 0XA0, 0X0A, 0X1B, 0X9B, 0X5D, 0X9B, 0X4B, 0X1A,
  0X59, 0X99, 0X1B, 0XDD, 0X94, 0X20, 0XA8, 0X42, 0X86, 0XE7, 0X62, 0X57, 0X26,
  0X37, 0X97, 0XF6, 0XE6, 0X36, 0X25, 0X20, 0X9A, 0X90, 0XE1, 0XB9, 0XD8, 0X85,
  0XB1, 0XD9, 0X95, 0XC9, 0X4D, 0X09, 0X8A, 0X3A, 0X64, 0X78, 0X2E, 0X73, 0X68,
  0X61, 0X64, 0X65, 0X72, 0X4D, 0X6F, 0X64, 0X65, 0X6C, 0X53, 0X02, 0XA4, 0X12,
  0X19, 0X9E, 0X0B, 0X5D, 0X1E, 0X5C, 0X59, 0X90, 0X9B, 0XDB, 0X1B, 0X5D, 0X18,
  0X5D, 0XDA, 0X9B, 0XDB, 0XDC, 0X94, 0XA0, 0XA9, 0X43, 0X86, 0XE7, 0X62, 0X97,
  0X56, 0X76, 0X97, 0X44, 0X36, 0X45, 0X17, 0X46, 0X57, 0X36, 0X25, 0X78, 0XEA,
  0X90, 0XE1, 0XB9, 0X94, 0XB9, 0XD1, 0XC9, 0XE5, 0X41, 0XBD, 0XA5, 0XB9, 0XD1,
  0XCD, 0X4D, 0X09, 0XCE, 0XA0, 0X0B, 0X19, 0X9E, 0XCB, 0XD8, 0X5B, 0X9D, 0X1B,
  0X5D, 0X99, 0XDC, 0XDC, 0X94, 0XC0, 0X0D, 0X00, 0X00, 0X79, 0X18, 0X00, 0X00,
  0X4C, 0X00, 0X00, 0X00, 0X33, 0X08, 0X80, 0X1C, 0XC4, 0XE1, 0X1C, 0X66, 0X14,
  0X01, 0X3D, 0X88, 0X43, 0X38, 0X84, 0XC3, 0X8C, 0X42, 0X80, 0X07, 0X79, 0X78,
  0X07, 0X73, 0X98, 0X71, 0X0C, 0XE6, 0X00, 0X0F, 0XED, 0X10, 0X0E, 0XF4, 0X80,
  0X0E, 0X33, 0X0C, 0X42, 0X1E, 0XC2, 0XC1, 0X1D, 0XCE, 0XA1, 0X1C, 0X66, 0X30,
  0X05, 0X3D, 0X88, 0X43, 0X38, 0X84, 0X83, 0X1B, 0XCC, 0X03, 0X3D, 0XC8, 0X43,
  0X3D, 0X8C, 0X03, 0X3D, 0XCC, 0X78, 0X8C, 0X74, 0X70, 0X07, 0X7B, 0X08, 0X07,
  0X79, 0X48, 0X87, 0X70, 0X70, 0X07, 0X7A, 0X70, 0X03, 0X76, 0X78, 0X87, 0X70,
  0X20, 0X87, 0X19, 0XCC, 0X11, 0X0E, 0XEC, 0X90, 0X0E, 0XE1, 0X30, 0X0F, 0X6E,
  0X30, 0X0F, 0XE3, 0XF0, 0X0E, 0XF0, 0X50, 0X0E, 0X33, 0X10, 0XC4, 0X1D, 0XDE,
  0X21, 0X1C, 0XD8, 0X21, 0X1D, 0XC2, 0X61, 0X1E, 0X66, 0X30, 0X89, 0X3B, 0XBC,
  0X83, 0X3B, 0XD0, 0X43, 0X39, 0XB4, 0X03, 0X3C, 0XBC, 0X83, 0X3C, 0X84, 0X03,
  0X3B, 0XCC, 0XF0, 0X14, 0X76, 0X60, 0X07, 0X7B, 0X68, 0X07, 0X37, 0X68, 0X87,
  0X72, 0X68, 0X07, 0X37, 0X80, 0X87, 0X70, 0X90, 0X87, 0X70, 0X60, 0X07, 0X76,
  0X28, 0X07, 0X76, 0XF8, 0X05, 0X76, 0X78, 0X87, 0X77, 0X80, 0X87, 0X5F, 0X08,
  0X87, 0X71, 0X18, 0X87, 0X72, 0X98, 0X87, 0X79, 0X98, 0X81, 0X2C, 0XEE, 0XF0,
  0X0E, 0XEE, 0XE0, 0X0E, 0XF5, 0XC0, 0X0E, 0XEC, 0X30, 0X03, 0X62, 0XC8, 0XA1,
  0X1C, 0XE4, 0XA1, 0X1C, 0XCC, 0XA1, 0X1C, 0XE4, 0XA1, 0X1C, 0XDC, 0X61, 0X1C,
  0XCA, 0X21, 0X1C, 0XC4, 0X81, 0X1D, 0XCA, 0X61, 0X06, 0XD6, 0X90, 0X43, 0X39,
  0XC8, 0X43, 0X39, 0X98, 0X43, 0X39, 0XC8, 0X43, 0X39, 0XB8, 0XC3, 0X38, 0X94,
  0X43, 0X38, 0X88, 0X03, 0X3B, 0X94, 0XC3, 0X2F, 0XBC, 0X83, 0X3C, 0XFC, 0X82,
  0X3B, 0XD4, 0X03, 0X3B, 0XB0, 0XC3, 0X8C, 0XC8, 0X21, 0X07, 0X7C, 0X70, 0X03,
  0X72, 0X10, 0X87, 0X73, 0X70, 0X03, 0X7B, 0X08, 0X07, 0X79, 0X60, 0X87, 0X70,
  0XC8, 0X87, 0X77, 0XA8, 0X07, 0X7A, 0X98, 0X81, 0X3C, 0XE4, 0X80, 0X0F, 0X6E,
  0X40, 0X0F, 0XE5, 0XD0, 0X0E, 0XF0, 0X00, 0X00, 0X00, 0X71, 0X20, 0X00, 0X00,
  0X0B, 0X00, 0X00, 0X00, 0X16, 0X30, 0X0D, 0X97, 0XEF, 0X3C, 0XFE, 0XE2, 0X00,
  0X83, 0XD8, 0X3C, 0XD4, 0XE4, 0X17, 0XB7, 0X6D, 0X02, 0XD5, 0X70, 0XF9, 0XCE,
  0XE3, 0X4B, 0X93, 0X13, 0X11, 0X28, 0X35, 0X3D, 0XD4, 0XE4, 0X17, 0XB7, 0X6D,
  0X00, 0X04, 0X03, 0X20, 0X0D, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X48, 0X41, 0X53, 0X48, 0X14, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X49,
  0X76, 0XB8, 0XA7, 0XCF, 0XA3, 0XA4, 0X03, 0XA2, 0XEC, 0X09, 0X1A, 0XCD, 0XE2,
  0XB4, 0XEA, 0X44, 0X58, 0X49, 0X4C, 0X90, 0X05, 0X00, 0X00, 0X60, 0X00, 0X01,
  0X00, 0X64, 0X01, 0X00, 0X00, 0X44, 0X58, 0X49, 0X4C, 0X00, 0X01, 0X00, 0X00,
  0X10, 0X00, 0X00, 0X00, 0X78, 0X05, 0X00, 0X00, 0X42, 0X43, 0XC0, 0XDE, 0X21,
  0X0C, 0X00, 0X00, 0X5B, 0X01, 0X00, 0X00, 0X0B, 0X82, 0X20, 0X00, 0X02, 0X00,
  0X00, 0X00, 0X13, 0X00, 0X00, 0X00, 0X07, 0X81, 0X23, 0X91, 0X41, 0XC8, 0X04,
  0X49, 0X06, 0X10, 0X32, 0X39, 0X92, 0X01, 0X84, 0X0C, 0X25, 0X05, 0X08, 0X19,
  0X1E, 0X04, 0X8B, 0X62, 0X80, 0X10, 0X45, 0X02, 0X42, 0X92, 0X0B, 0X42, 0X84,
  0X10, 0X32, 0X14, 0X38, 0X08, 0X18, 0X4B, 0X0A, 0X32, 0X42, 0X88, 0X48, 0X90,
  0X14, 0X20, 0X43, 0X46, 0X88, 0XA5, 0X00, 0X19, 0X32, 0X42, 0XE4, 0X48, 0X0E,
  0X90, 0X11, 0X22, 0XC4, 0X50, 0X41, 0X51, 0X81, 0X8C, 0XE1, 0X83, 0XE5, 0X8A,
  0X04, 0X21, 0X46, 0X06, 0X51, 0X18, 0X00, 0X00, 0X06, 0X00, 0X00, 0X00, 0X1B,
  0X8C, 0XE0, 0XFF, 0XFF, 0XFF, 0XFF, 0X07, 0X40, 0X02, 0XA8, 0X0D, 0X84, 0XF0,
  0XFF, 0XFF, 0XFF, 0XFF, 0X03, 0X20, 0X01, 0X00, 0X00, 0X00, 0X49, 0X18, 0X00,
  0X00, 0X02, 0X00, 0X00, 0X00, 0X13, 0X82, 0X60, 0X42, 0X20, 0X00, 0X00, 0X00,
  0X89, 0X20, 0X00, 0X00, 0X0F, 0X00, 0X00, 0X00, 0X32, 0X22, 0X08, 0X09, 0X20,
  0X64, 0X85, 0X04, 0X13, 0X22, 0XA4, 0X84, 0X04, 0X13, 0X22, 0XE3, 0X84, 0XA1,
  0X90, 0X14, 0X12, 0X4C, 0X88, 0X8C, 0X0B, 0X84, 0X84, 0X4C, 0X10, 0X30, 0X23,
  0X00, 0X25, 0X00, 0X8A, 0X19, 0X80, 0X39, 0X02, 0X30, 0X98, 0X23, 0X40, 0X8A,
  0X31, 0X44, 0X54, 0X44, 0X56, 0X0C, 0X20, 0XA2, 0X1A, 0XC2, 0X81, 0X80, 0X54,
  0X20, 0X00, 0X00, 0X13, 0X14, 0X72, 0XC0, 0X87, 0X74, 0X60, 0X87, 0X36, 0X68,
  0X87, 0X79, 0X68, 0X03, 0X72, 0XC0, 0X87, 0X0D, 0XAF, 0X50, 0X0E, 0X6D, 0XD0,
  0X0E, 0X7A, 0X50, 0X0E, 0X6D, 0X00, 0X0F, 0X7A, 0X30, 0X07, 0X72, 0XA0, 0X07,
  0X73, 0X20, 0X07, 0X6D, 0X90, 0X0E, 0X71, 0XA0, 0X07, 0X73, 0X20, 0X07, 0X6D,
  0X90, 0X0E, 0X78, 0XA0, 0X07, 0X73, 0X20, 0X07, 0X6D, 0X90, 0X0E, 0X71, 0X60,
  0X07, 0X7A, 0X30, 0X07, 0X72, 0XD0, 0X06, 0XE9, 0X30, 0X07, 0X72, 0XA0, 0X07,
  0X73, 0X20, 0X07, 0X6D, 0X90, 0X0E, 0X76, 0X40, 0X07, 0X7A, 0X60, 0X07, 0X74,
  0XD0, 0X06, 0XE6, 0X10, 0X07, 0X76, 0XA0, 0X07, 0X73, 0X20, 0X07, 0X6D, 0X60,
  0X0E, 0X73, 0X20, 0X07, 0X7A, 0X30, 0X07, 0X72, 0XD0, 0X06, 0XE6, 0X60, 0X07,
  0X74, 0XA0, 0X07, 0X76, 0X40, 0X07, 0X6D, 0XE0, 0X0E, 0X78, 0XA0, 0X07, 0X71,
  0X60, 0X07, 0X7A, 0X30, 0X07, 0X72, 0XA0, 0X07, 0X76, 0X40, 0X07, 0X43, 0X9E,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X86, 0X3C,
  0X06, 0X10, 0X00, 0X01, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X0C, 0X79,
  0X10, 0X20, 0X00, 0X04, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0XC8, 0X02,
  0X01, 0X0D, 0X00, 0X00, 0X00, 0X32, 0X1E, 0X98, 0X14, 0X19, 0X11, 0X4C, 0X90,
  0X8C, 0X09, 0X26, 0X47, 0XC6, 0X04, 0X43, 0XA2, 0X12, 0X18, 0X01, 0X28, 0X89,
  0X62, 0X28, 0X83, 0XF2, 0X28, 0X02, 0XAA, 0X92, 0X18, 0X01, 0X28, 0X84, 0X22,
  0X28, 0X03, 0XDA, 0XB1, 0X14, 0X84, 0X01, 0X11, 0X10, 0X0A, 0X04, 0X08, 0X0C,
  0X40, 0X01, 0X24, 0X00, 0X00, 0X79, 0X18, 0X00, 0X00, 0X4F, 0X00, 0X00, 0X00,
  0X1A, 0X03, 0X4C, 0X90, 0X46, 0X02, 0X13, 0XC4, 0X31, 0X20, 0XC3, 0X1B, 0X43,
  0X81, 0X93, 0X4B, 0XB3, 0X0B, 0XA3, 0X2B, 0X4B, 0X01, 0X89, 0X71, 0XC9, 0X71,
  0X81, 0X71, 0XA9, 0X89, 0XC1, 0XA1, 0X01, 0X41, 0X91, 0X89, 0X21, 0X93, 0XC1,
  0X31, 0XBB, 0X91, 0XB9, 0X49, 0XD9, 0X10, 0X04, 0X13, 0X04, 0X62, 0X98, 0X20,
  0X10, 0XC4, 0X06, 0X61, 0X20, 0X26, 0X08, 0X44, 0XB1, 0X41, 0X18, 0X0C, 0X0A,
  0X76, 0X73, 0X13, 0X04, 0XC2, 0XD8, 0X30, 0X20, 0X09, 0X31, 0X41, 0X58, 0X9E,
  0X0D, 0XC1, 0X32, 0X41, 0X10, 0X00, 0X12, 0X6D, 0X61, 0X69, 0X6E, 0X44, 0XA8,
  0X8A, 0XB0, 0X86, 0X9E, 0X9E, 0XA4, 0X88, 0X26, 0X08, 0X85, 0X32, 0X41, 0X28,
  0X96, 0X0D, 0X01, 0X31, 0X41, 0X28, 0X98, 0X09, 0X02, 0X71, 0X4C, 0X10, 0X08,
  0X64, 0X83, 0X40, 0X55, 0X1B, 0X16, 0XE2, 0X81, 0X22, 0X29, 0X1A, 0X26, 0X22,
  0XB2, 0X36, 0X04, 0XC3, 0X86, 0X65, 0X78, 0XA0, 0X08, 0X8B, 0X86, 0X69, 0X88,
  0XAC, 0X0D, 0XC2, 0X95, 0X4D, 0X10, 0X8A, 0X66, 0X83, 0X40, 0X51, 0X1B, 0X16,
  0XE2, 0X81, 0X22, 0X69, 0X1B, 0X36, 0X22, 0XE2, 0X36, 0X2C, 0XC3, 0X03, 0X45,
  0XD8, 0X36, 0X4C, 0X43, 0X64, 0X4D, 0X10, 0X88, 0X84, 0XCB, 0X94, 0XD5, 0X17,
  0XD4, 0XDB, 0X5C, 0X1A, 0X5D, 0XDA, 0X9B, 0XDB, 0X04, 0XA1, 0X70, 0X36, 0X2C,
  0X1F, 0X18, 0X40, 0X61, 0X20, 0X4D, 0XC3, 0XF4, 0X45, 0XD6, 0X86, 0XA1, 0XF3,
  0XC4, 0X60, 0XC3, 0XA0, 0X8D, 0X01, 0XB0, 0XA1, 0X68, 0X1C, 0X32, 0X00, 0X80,
  0X2A, 0X6C, 0X6C, 0X76, 0X6D, 0X2E, 0X69, 0X64, 0X65, 0X6E, 0X74, 0X53, 0X82,
  0XA0, 0X0A, 0X19, 0X9E, 0X8B, 0X5D, 0X99, 0XDC, 0X5C, 0XDA, 0X9B, 0XDB, 0X94,
  0X80, 0X68, 0X42, 0X86, 0XE7, 0X62, 0X17, 0XC6, 0X66, 0X57, 0X26, 0X37, 0X25,
  0X30, 0XEA, 0X90, 0XE1, 0XB9, 0XCC, 0XA1, 0X85, 0X91, 0X95, 0XC9, 0X35, 0XBD,
  0X91, 0X95, 0XB1, 0X4D, 0X09, 0X92, 0X3A, 0X64, 0X78, 0X2E, 0X76, 0X69, 0X65,
  0X77, 0X49, 0X64, 0X53, 0X74, 0X61, 0X74, 0X65, 0X53, 0X82, 0XA5, 0X0E, 0X19,
  0X9E, 0X4B, 0X99, 0X1B, 0X9D, 0X5C, 0X1E, 0XD4, 0X5B, 0X9A, 0X1B, 0XDD, 0XDC,
  0X94, 0X80, 0X0C, 0X00, 0X79, 0X18, 0X00, 0X00, 0X4C, 0X00, 0X00, 0X00, 0X33,
  0X08, 0X80, 0X1C, 0XC4, 0XE1, 0X1C, 0X66, 0X14, 0X01, 0X3D, 0X88, 0X43, 0X38,
  0X84, 0XC3, 0X8C, 0X42, 0X80, 0X07, 0X79, 0X78, 0X07, 0X73, 0X98, 0X71, 0X0C,
  0XE6, 0X00, 0X0F, 0XED, 0X10, 0X0E, 0XF4, 0X80, 0X0E, 0X33, 0X0C, 0X42, 0X1E,
  0XC2, 0XC1, 0X1D, 0XCE, 0XA1, 0X1C, 0X66, 0X30, 0X05, 0X3D, 0X88, 0X43, 0X38,
  0X84, 0X83, 0X1B, 0XCC, 0X03, 0X3D, 0XC8, 0X43, 0X3D, 0X8C, 0X03, 0X3D, 0XCC,
  0X78, 0X8C, 0X74, 0X70, 0X07, 0X7B, 0X08, 0X07, 0X79, 0X48, 0X87, 0X70, 0X70,
  0X07, 0X7A, 0X70, 0X03, 0X76, 0X78, 0X87, 0X70, 0X20, 0X87, 0X19, 0XCC, 0X11,
  0X0E, 0XEC, 0X90, 0X0E, 0XE1, 0X30, 0X0F, 0X6E, 0X30, 0X0F, 0XE3, 0XF0, 0X0E,
  0XF0, 0X50, 0X0E, 0X33, 0X10, 0XC4, 0X1D, 0XDE, 0X21, 0X1C, 0XD8, 0X21, 0X1D,
  0XC2, 0X61, 0X1E, 0X66, 0X30, 0X89, 0X3B, 0XBC, 0X83, 0X3B, 0XD0, 0X43, 0X39,
  0XB4, 0X03, 0X3C, 0XBC, 0X83, 0X3C, 0X84, 0X03, 0X3B, 0XCC, 0XF0, 0X14, 0X76,
  0X60, 0X07, 0X7B, 0X68, 0X07, 0X37, 0X68, 0X87, 0X72, 0X68, 0X07, 0X37, 0X80,
  0X87, 0X70, 0X90, 0X87, 0X70, 0X60, 0X07, 0X76, 0X28, 0X07, 0X76, 0XF8, 0X05,
  0X76, 0X78, 0X87, 0X77, 0X80, 0X87, 0X5F, 0X08, 0X87, 0X71, 0X18, 0X87, 0X72,
  0X98, 0X87, 0X79, 0X98, 0X81, 0X2C, 0XEE, 0XF0, 0X0E, 0XEE, 0XE0, 0X0E, 0XF5,
  0XC0, 0X0E, 0XEC, 0X30, 0X03, 0X62, 0XC8, 0XA1, 0X1C, 0XE4, 0XA1, 0X1C, 0XCC,
  0XA1, 0X1C, 0XE4, 0XA1, 0X1C, 0XDC, 0X61, 0X1C, 0XCA, 0X21, 0X1C, 0XC4, 0X81,
  0X1D, 0XCA, 0X61, 0X06, 0XD6, 0X90, 0X43, 0X39, 0XC8, 0X43, 0X39, 0X98, 0X43,
  0X39, 0XC8, 0X43, 0X39, 0XB8, 0XC3, 0X38, 0X94, 0X43, 0X38, 0X88, 0X03, 0X3B,
  0X94, 0XC3, 0X2F, 0XBC, 0X83, 0X3C, 0XFC, 0X82, 0X3B, 0XD4, 0X03, 0X3B, 0XB0,
  0XC3, 0X8C, 0XC8, 0X21, 0X07, 0X7C, 0X70, 0X03, 0X72, 0X10, 0X87, 0X73, 0X70,
  0X03, 0X7B, 0X08, 0X07, 0X79, 0X60, 0X87, 0X70, 0XC8, 0X87, 0X77, 0XA8, 0X07,
  0X7A, 0X98, 0X81, 0X3C, 0XE4, 0X80, 0X0F, 0X6E, 0X40, 0X0F, 0XE5, 0XD0, 0X0E,
  0XF0, 0X00, 0X00, 0X00, 0X71, 0X20, 0X00, 0X00, 0X0B, 0X00, 0X00, 0X00, 0X16,
  0X30, 0X0D, 0X97, 0XEF, 0X3C, 0XFE, 0XE2, 0X00, 0X83, 0XD8, 0X3C, 0XD4, 0XE4,
  0X17, 0XB7, 0X6D, 0X02, 0XD5, 0X70, 0XF9, 0XCE, 0XE3, 0X4B, 0X93, 0X13, 0X11,
  0X28, 0X35, 0X3D, 0XD4, 0XE4, 0X17, 0XB7, 0X6D, 0X00, 0X04, 0X03, 0X20, 0X0D,
  0X00, 0X00, 0X00, 0X00, 0X61, 0X20, 0X00, 0X00, 0X3E, 0X00, 0X00, 0X00, 0X13,
  0X04, 0X41, 0X2C, 0X10, 0X00, 0X00, 0X00, 0X05, 0X00, 0X00, 0X00, 0X54, 0X25,
  0X40, 0X34, 0X03, 0X50, 0X0A, 0X85, 0X40, 0X33, 0X46, 0X00, 0X82, 0X20, 0X88,
  0X7F, 0X23, 0X00, 0X00, 0X00, 0X23, 0X06, 0X09, 0X00, 0X82, 0X60, 0X60, 0X54,
  0XC3, 0X24, 0X2D, 0XC5, 0X88, 0X41, 0X02, 0X80, 0X20, 0X18, 0X18, 0X16, 0X41,
  0X4D, 0X87, 0X31, 0X62, 0X90, 0X00, 0X20, 0X08, 0X06, 0XC6, 0X55, 0X54, 0XD4,
  0X72, 0X8C, 0X18, 0X24, 0X00, 0X08, 0X82, 0X81, 0X81, 0X19, 0X56, 0XB5, 0X20,
  0X23, 0X06, 0X09, 0X00, 0X82, 0X60, 0X60, 0X64, 0X87, 0X65, 0X3D, 0XC9, 0X88,
  0X41, 0X02, 0X80, 0X20, 0X18, 0X18, 0X1A, 0X72, 0X5D, 0X8B, 0X32, 0X62, 0X90,
  0X00, 0X20, 0X08, 0X06, 0XC6, 0X96, 0X60, 0XD8, 0XB3, 0X8C, 0X18, 0X24, 0X00,
  0X08, 0X82, 0X81, 0XC1, 0X29, 0X59, 0XF6, 0X30, 0X23, 0X06, 0X09, 0X00, 0X82,
  0X60, 0X80, 0X70, 0X8C, 0XA6, 0X4D, 0XC2, 0X88, 0X41, 0X02, 0X80, 0X20, 0X18,
  0X20, 0X1C, 0XA3, 0X69, 0X4E, 0X30, 0X62, 0X90, 0X00, 0X20, 0X08, 0X06, 0X08,
  0XC7, 0X6C, 0XDA, 0X84, 0X8C, 0X18, 0X24, 0X00, 0X08, 0X82, 0X01, 0XC2, 0X31,
  0X9B, 0XE6, 0X1C, 0X23, 0X06, 0X09, 0X00, 0X82, 0X60, 0X80, 0X70, 0XCC, 0XA6,
  0X45, 0XC6, 0X88, 0X41, 0X02, 0X80, 0X20, 0X18, 0X20, 0X1C, 0XB3, 0X69, 0X50,
  0X31, 0X62, 0X90, 0X00, 0X20, 0X08, 0X06, 0X08, 0XC7, 0X54, 0XDA, 0X44, 0X8C,
  0X18, 0X24, 0X00, 0X08, 0X82, 0X01, 0XC2, 0X31, 0X95, 0XE6, 0X0C, 0X23, 0X06,
  0X09, 0X00, 0X82, 0X60, 0X80, 0X70, 0X4C, 0XA5, 0X45, 0XC9, 0X88, 0X41, 0X02,
  0X80, 0X20, 0X18, 0X20, 0X1C, 0X53, 0X69, 0X90, 0X82, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00
};

/*
 * #version 450 core
 *
 * layout(location=0) in vec2 tex_uv;
 * layout(location=1) in vec4 i_color;
 *
 * layout(location=0) out vec4 frag_color;
 *
 * layout(set = 2, binding=0) uniform sampler2D uni_tex_sampler;
 *
 * void main() {
 *   frag_color = texture(uni_tex_sampler, tex_uv) * i_color;
 * }
 */

size_t _shader_farg_spv_len = 668;
Uint8 _shader_frag_spv[]    = {
  0X03, 0X02, 0X23, 0X07, 0X00, 0X00, 0X01, 0X00, 0X0B, 0X00, 0X08, 0X00, 0X18,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X11, 0X00, 0X02, 0X00, 0X01, 0X00,
  0X00, 0X00, 0X0B, 0X00, 0X06, 0X00, 0X01, 0X00, 0X00, 0X00, 0X47, 0X4C, 0X53,
  0X4C, 0X2E, 0X73, 0X74, 0X64, 0X2E, 0X34, 0X35, 0X30, 0X00, 0X00, 0X00, 0X00,
  0X0E, 0X00, 0X03, 0X00, 0X00, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X0F,
  0X00, 0X08, 0X00, 0X04, 0X00, 0X00, 0X00, 0X04, 0X00, 0X00, 0X00, 0X6D, 0X61,
  0X69, 0X6E, 0X00, 0X00, 0X00, 0X00, 0X09, 0X00, 0X00, 0X00, 0X11, 0X00, 0X00,
  0X00, 0X15, 0X00, 0X00, 0X00, 0X10, 0X00, 0X03, 0X00, 0X04, 0X00, 0X00, 0X00,
  0X07, 0X00, 0X00, 0X00, 0X03, 0X00, 0X03, 0X00, 0X02, 0X00, 0X00, 0X00, 0XC2,
  0X01, 0X00, 0X00, 0X05, 0X00, 0X04, 0X00, 0X04, 0X00, 0X00, 0X00, 0X6D, 0X61,
  0X69, 0X6E, 0X00, 0X00, 0X00, 0X00, 0X05, 0X00, 0X05, 0X00, 0X09, 0X00, 0X00,
  0X00, 0X66, 0X72, 0X61, 0X67, 0X5F, 0X63, 0X6F, 0X6C, 0X6F, 0X72, 0X00, 0X00,
  0X05, 0X00, 0X06, 0X00, 0X0D, 0X00, 0X00, 0X00, 0X75, 0X6E, 0X69, 0X5F, 0X74,
  0X65, 0X78, 0X5F, 0X73, 0X61, 0X6D, 0X70, 0X6C, 0X65, 0X72, 0X00, 0X05, 0X00,
  0X04, 0X00, 0X11, 0X00, 0X00, 0X00, 0X74, 0X65, 0X78, 0X5F, 0X75, 0X76, 0X00,
  0X00, 0X05, 0X00, 0X04, 0X00, 0X15, 0X00, 0X00, 0X00, 0X69, 0X5F, 0X63, 0X6F,
  0X6C, 0X6F, 0X72, 0X00, 0X47, 0X00, 0X04, 0X00, 0X09, 0X00, 0X00, 0X00, 0X1E,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X47, 0X00, 0X04, 0X00, 0X0D, 0X00,
  0X00, 0X00, 0X21, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X47, 0X00, 0X04,
  0X00, 0X0D, 0X00, 0X00, 0X00, 0X22, 0X00, 0X00, 0X00, 0X02, 0X00, 0X00, 0X00,
  0X47, 0X00, 0X04, 0X00, 0X11, 0X00, 0X00, 0X00, 0X1E, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X47, 0X00, 0X04, 0X00, 0X15, 0X00, 0X00, 0X00, 0X1E, 0X00,
  0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X13, 0X00, 0X02, 0X00, 0X02, 0X00, 0X00,
  0X00, 0X21, 0X00, 0X03, 0X00, 0X03, 0X00, 0X00, 0X00, 0X02, 0X00, 0X00, 0X00,
  0X16, 0X00, 0X03, 0X00, 0X06, 0X00, 0X00, 0X00, 0X20, 0X00, 0X00, 0X00, 0X17,
  0X00, 0X04, 0X00, 0X07, 0X00, 0X00, 0X00, 0X06, 0X00, 0X00, 0X00, 0X04, 0X00,
  0X00, 0X00, 0X20, 0X00, 0X04, 0X00, 0X08, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00,
  0X00, 0X07, 0X00, 0X00, 0X00, 0X3B, 0X00, 0X04, 0X00, 0X08, 0X00, 0X00, 0X00,
  0X09, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00, 0X00, 0X19, 0X00, 0X09, 0X00, 0X0A,
  0X00, 0X00, 0X00, 0X06, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X1B, 0X00, 0X03, 0X00, 0X0B, 0X00, 0X00, 0X00,
  0X0A, 0X00, 0X00, 0X00, 0X20, 0X00, 0X04, 0X00, 0X0C, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X0B, 0X00, 0X00, 0X00, 0X3B, 0X00, 0X04, 0X00, 0X0C, 0X00,
  0X00, 0X00, 0X0D, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X17, 0X00, 0X04,
  0X00, 0X0F, 0X00, 0X00, 0X00, 0X06, 0X00, 0X00, 0X00, 0X02, 0X00, 0X00, 0X00,
  0X20, 0X00, 0X04, 0X00, 0X10, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X0F,
  0X00, 0X00, 0X00, 0X3B, 0X00, 0X04, 0X00, 0X10, 0X00, 0X00, 0X00, 0X11, 0X00,
  0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X20, 0X00, 0X04, 0X00, 0X14, 0X00, 0X00,
  0X00, 0X01, 0X00, 0X00, 0X00, 0X07, 0X00, 0X00, 0X00, 0X3B, 0X00, 0X04, 0X00,
  0X14, 0X00, 0X00, 0X00, 0X15, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X36,
  0X00, 0X05, 0X00, 0X02, 0X00, 0X00, 0X00, 0X04, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X03, 0X00, 0X00, 0X00, 0XF8, 0X00, 0X02, 0X00, 0X05, 0X00, 0X00,
  0X00, 0X3D, 0X00, 0X04, 0X00, 0X0B, 0X00, 0X00, 0X00, 0X0E, 0X00, 0X00, 0X00,
  0X0D, 0X00, 0X00, 0X00, 0X3D, 0X00, 0X04, 0X00, 0X0F, 0X00, 0X00, 0X00, 0X12,
  0X00, 0X00, 0X00, 0X11, 0X00, 0X00, 0X00, 0X57, 0X00, 0X05, 0X00, 0X07, 0X00,
  0X00, 0X00, 0X13, 0X00, 0X00, 0X00, 0X0E, 0X00, 0X00, 0X00, 0X12, 0X00, 0X00,
  0X00, 0X3D, 0X00, 0X04, 0X00, 0X07, 0X00, 0X00, 0X00, 0X16, 0X00, 0X00, 0X00,
  0X15, 0X00, 0X00, 0X00, 0X85, 0X00, 0X05, 0X00, 0X07, 0X00, 0X00, 0X00, 0X17,
  0X00, 0X00, 0X00, 0X13, 0X00, 0X00, 0X00, 0X16, 0X00, 0X00, 0X00, 0X3E, 0X00,
  0X03, 0X00, 0X09, 0X00, 0X00, 0X00, 0X17, 0X00, 0X00, 0X00, 0XFD, 0X00, 0X01,
  0X00, 0X38, 0X00, 0X01, 0X00
};

unsigned int _shader_frag_msl_len = 505;
unsigned char _shader_frag_msl[]  = {
  0X23, 0X69, 0X6E, 0X63, 0X6C, 0X75, 0X64, 0X65, 0X20, 0X3C, 0X6D, 0X65, 0X74,
  0X61, 0X6C, 0X5F, 0X73, 0X74, 0X64, 0X6C, 0X69, 0X62, 0X3E, 0X0A, 0X23, 0X69,
  0X6E, 0X63, 0X6C, 0X75, 0X64, 0X65, 0X20, 0X3C, 0X73, 0X69, 0X6D, 0X64, 0X2F,
  0X73, 0X69, 0X6D, 0X64, 0X2E, 0X68, 0X3E, 0X0A, 0X0A, 0X75, 0X73, 0X69, 0X6E,
  0X67, 0X20, 0X6E, 0X61, 0X6D, 0X65, 0X73, 0X70, 0X61, 0X63, 0X65, 0X20, 0X6D,
  0X65, 0X74, 0X61, 0X6C, 0X3B, 0X0A, 0X0A, 0X73, 0X74, 0X72, 0X75, 0X63, 0X74,
  0X20, 0X6D, 0X61, 0X69, 0X6E, 0X30, 0X5F, 0X6F, 0X75, 0X74, 0X0A, 0X7B, 0X0A,
  0X20, 0X20, 0X20, 0X20, 0X66, 0X6C, 0X6F, 0X61, 0X74, 0X34, 0X20, 0X66, 0X72,
  0X61, 0X67, 0X5F, 0X63, 0X6F, 0X6C, 0X6F, 0X72, 0X20, 0X5B, 0X5B, 0X63, 0X6F,
  0X6C, 0X6F, 0X72, 0X28, 0X30, 0X29, 0X5D, 0X5D, 0X3B, 0X0A, 0X7D, 0X3B, 0X0A,
  0X0A, 0X73, 0X74, 0X72, 0X75, 0X63, 0X74, 0X20, 0X6D, 0X61, 0X69, 0X6E, 0X30,
  0X5F, 0X69, 0X6E, 0X0A, 0X7B, 0X0A, 0X20, 0X20, 0X20, 0X20, 0X66, 0X6C, 0X6F,
  0X61, 0X74, 0X32, 0X20, 0X74, 0X65, 0X78, 0X5F, 0X75, 0X76, 0X20, 0X5B, 0X5B,
  0X75, 0X73, 0X65, 0X72, 0X28, 0X6C, 0X6F, 0X63, 0X6E, 0X30, 0X29, 0X5D, 0X5D,
  0X3B, 0X0A, 0X20, 0X20, 0X20, 0X20, 0X66, 0X6C, 0X6F, 0X61, 0X74, 0X34, 0X20,
  0X69, 0X5F, 0X63, 0X6F, 0X6C, 0X6F, 0X72, 0X20, 0X5B, 0X5B, 0X75, 0X73, 0X65,
  0X72, 0X28, 0X6C, 0X6F, 0X63, 0X6E, 0X31, 0X29, 0X5D, 0X5D, 0X3B, 0X0A, 0X7D,
  0X3B, 0X0A, 0X0A, 0X66, 0X72, 0X61, 0X67, 0X6D, 0X65, 0X6E, 0X74, 0X20, 0X6D,
  0X61, 0X69, 0X6E, 0X30, 0X5F, 0X6F, 0X75, 0X74, 0X20, 0X6D, 0X61, 0X69, 0X6E,
  0X30, 0X28, 0X6D, 0X61, 0X69, 0X6E, 0X30, 0X5F, 0X69, 0X6E, 0X20, 0X69, 0X6E,
  0X20, 0X5B, 0X5B, 0X73, 0X74, 0X61, 0X67, 0X65, 0X5F, 0X69, 0X6E, 0X5D, 0X5D,
  0X2C, 0X20, 0X74, 0X65, 0X78, 0X74, 0X75, 0X72, 0X65, 0X32, 0X64, 0X3C, 0X66,
  0X6C, 0X6F, 0X61, 0X74, 0X3E, 0X20, 0X75, 0X6E, 0X69, 0X5F, 0X74, 0X65, 0X78,
  0X5F, 0X73, 0X61, 0X6D, 0X70, 0X6C, 0X65, 0X72, 0X20, 0X5B, 0X5B, 0X74, 0X65,
  0X78, 0X74, 0X75, 0X72, 0X65, 0X28, 0X30, 0X29, 0X5D, 0X5D, 0X2C, 0X20, 0X73,
  0X61, 0X6D, 0X70, 0X6C, 0X65, 0X72, 0X20, 0X75, 0X6E, 0X69, 0X5F, 0X74, 0X65,
  0X78, 0X5F, 0X73, 0X61, 0X6D, 0X70, 0X6C, 0X65, 0X72, 0X53, 0X6D, 0X70, 0X6C,
  0X72, 0X20, 0X5B, 0X5B, 0X73, 0X61, 0X6D, 0X70, 0X6C, 0X65, 0X72, 0X28, 0X30,
  0X29, 0X5D, 0X5D, 0X29, 0X0A, 0X7B, 0X0A, 0X20, 0X20, 0X20, 0X20, 0X6D, 0X61,
  0X69, 0X6E, 0X30, 0X5F, 0X6F, 0X75, 0X74, 0X20, 0X6F, 0X75, 0X74, 0X20, 0X3D,
  0X20, 0X7B, 0X7D, 0X3B, 0X0A, 0X20, 0X20, 0X20, 0X20, 0X6F, 0X75, 0X74, 0X2E,
  0X66, 0X72, 0X61, 0X67, 0X5F, 0X63, 0X6F, 0X6C, 0X6F, 0X72, 0X20, 0X3D, 0X20,
  0X75, 0X6E, 0X69, 0X5F, 0X74, 0X65, 0X78, 0X5F, 0X73, 0X61, 0X6D, 0X70, 0X6C,
  0X65, 0X72, 0X2E, 0X73, 0X61, 0X6D, 0X70, 0X6C, 0X65, 0X28, 0X75, 0X6E, 0X69,
  0X5F, 0X74, 0X65, 0X78, 0X5F, 0X73, 0X61, 0X6D, 0X70, 0X6C, 0X65, 0X72, 0X53,
  0X6D, 0X70, 0X6C, 0X72, 0X2C, 0X20, 0X69, 0X6E, 0X2E, 0X74, 0X65, 0X78, 0X5F,
  0X75, 0X76, 0X29, 0X20, 0X2A, 0X20, 0X69, 0X6E, 0X2E, 0X69, 0X5F, 0X63, 0X6F,
  0X6C, 0X6F, 0X72, 0X3B, 0X0A, 0X20, 0X20, 0X20, 0X20, 0X72, 0X65, 0X74, 0X75,
  0X72, 0X6E, 0X20, 0X6F, 0X75, 0X74, 0X3B, 0X0A, 0X7D, 0X0A, 0X0A
};

unsigned int _shader_frag_dxil_len = 3896;
unsigned char _shader_frag_dxil[]  = {
  0X44, 0X58, 0X42, 0X43, 0X3D, 0XE0, 0X10, 0X51, 0XB5, 0XE1, 0X54, 0X94, 0XA7,
  0XBB, 0XA5, 0X02, 0X68, 0XFC, 0X36, 0X09, 0X01, 0X00, 0X00, 0X00, 0X38, 0X0F,
  0X00, 0X00, 0X07, 0X00, 0X00, 0X00, 0X3C, 0X00, 0X00, 0X00, 0X4C, 0X00, 0X00,
  0X00, 0XA8, 0X00, 0X00, 0X00, 0XE4, 0X00, 0X00, 0X00, 0XD8, 0X01, 0X00, 0X00,
  0X54, 0X08, 0X00, 0X00, 0X70, 0X08, 0X00, 0X00, 0X53, 0X46, 0X49, 0X30, 0X08,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X49, 0X53,
  0X47, 0X31, 0X54, 0X00, 0X00, 0X00, 0X02, 0X00, 0X00, 0X00, 0X08, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X48, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X03,
  0X03, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X48, 0X00,
  0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00,
  0X00, 0X01, 0X00, 0X00, 0X00, 0X0F, 0X0F, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X54, 0X45, 0X58, 0X43, 0X4F, 0X4F, 0X52, 0X44, 0X00, 0X00, 0X00, 0X00, 0X4F,
  0X53, 0X47, 0X31, 0X34, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X08, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X28, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X40, 0X00, 0X00, 0X00, 0X03, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X0F, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X53, 0X56, 0X5F, 0X54, 0X61,
  0X72, 0X67, 0X65, 0X74, 0X00, 0X00, 0X00, 0X50, 0X53, 0X56, 0X30, 0XEC, 0X00,
  0X00, 0X00, 0X34, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0XFF, 0XFF, 0XFF, 0XFF, 0X00, 0X00, 0X00, 0X00, 0X02, 0X01, 0X00, 0X02, 0X01,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X13, 0X00, 0X00, 0X00, 0X02, 0X00, 0X00, 0X00, 0X18, 0X00, 0X00,
  0X00, 0X01, 0X00, 0X00, 0X00, 0X02, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X0E, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X03,
  0X00, 0X00, 0X00, 0X02, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X02, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X18, 0X00, 0X00,
  0X00, 0X00, 0X54, 0X45, 0X58, 0X43, 0X4F, 0X4F, 0X52, 0X44, 0X00, 0X54, 0X45,
  0X58, 0X43, 0X4F, 0X4F, 0X52, 0X44, 0X00, 0X6D, 0X61, 0X69, 0X6E, 0X00, 0X02,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X10, 0X00,
  0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X01, 0X00, 0X42,
  0X00, 0X03, 0X02, 0X00, 0X00, 0X0A, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00,
  0X01, 0X01, 0X44, 0X00, 0X03, 0X02, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X00, 0X00, 0X01, 0X00, 0X44, 0X10, 0X03, 0X00, 0X00, 0X00, 0X0F, 0X00,
  0X00, 0X00, 0X0F, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X00, 0X01, 0X00, 0X00, 0X00, 0X02, 0X00, 0X00, 0X00, 0X04, 0X00, 0X00, 0X00,
  0X08, 0X00, 0X00, 0X00, 0X53, 0X54, 0X41, 0X54, 0X74, 0X06, 0X00, 0X00, 0X60,
  0X00, 0X00, 0X00, 0X9D, 0X01, 0X00, 0X00, 0X44, 0X58, 0X49, 0X4C, 0X00, 0X01,
  0X00, 0X00, 0X10, 0X00, 0X00, 0X00, 0X5C, 0X06, 0X00, 0X00, 0X42, 0X43, 0XC0,
  0XDE, 0X21, 0X0C, 0X00, 0X00, 0X94, 0X01, 0X00, 0X00, 0X0B, 0X82, 0X20, 0X00,
  0X02, 0X00, 0X00, 0X00, 0X13, 0X00, 0X00, 0X00, 0X07, 0X81, 0X23, 0X91, 0X41,
  0XC8, 0X04, 0X49, 0X06, 0X10, 0X32, 0X39, 0X92, 0X01, 0X84, 0X0C, 0X25, 0X05,
  0X08, 0X19, 0X1E, 0X04, 0X8B, 0X62, 0X80, 0X14, 0X45, 0X02, 0X42, 0X92, 0X0B,
  0X42, 0XA4, 0X10, 0X32, 0X14, 0X38, 0X08, 0X18, 0X4B, 0X0A, 0X32, 0X52, 0X88,
  0X48, 0X90, 0X14, 0X20, 0X43, 0X46, 0X88, 0XA5, 0X00, 0X19, 0X32, 0X42, 0XE4,
  0X48, 0X0E, 0X90, 0X91, 0X22, 0XC4, 0X50, 0X41, 0X51, 0X81, 0X8C, 0XE1, 0X83,
  0XE5, 0X8A, 0X04, 0X29, 0X46, 0X06, 0X51, 0X18, 0X00, 0X00, 0X08, 0X00, 0X00,
  0X00, 0X1B, 0X8C, 0XE0, 0XFF, 0XFF, 0XFF, 0XFF, 0X07, 0X40, 0X02, 0XA8, 0X0D,
  0X84, 0XF0, 0XFF, 0XFF, 0XFF, 0XFF, 0X03, 0X20, 0X6D, 0X30, 0X86, 0XFF, 0XFF,
  0XFF, 0XFF, 0X1F, 0X00, 0X09, 0XA8, 0X00, 0X49, 0X18, 0X00, 0X00, 0X03, 0X00,
  0X00, 0X00, 0X13, 0X82, 0X60, 0X42, 0X20, 0X4C, 0X08, 0X06, 0X00, 0X00, 0X00,
  0X00, 0X89, 0X20, 0X00, 0X00, 0X43, 0X00, 0X00, 0X00, 0X32, 0X22, 0X48, 0X09,
  0X20, 0X64, 0X85, 0X04, 0X93, 0X22, 0XA4, 0X84, 0X04, 0X93, 0X22, 0XE3, 0X84,
  0XA1, 0X90, 0X14, 0X12, 0X4C, 0X8A, 0X8C, 0X0B, 0X84, 0XA4, 0X4C, 0X10, 0X68,
  0X23, 0X00, 0X25, 0X00, 0X14, 0X66, 0X00, 0XE6, 0X08, 0XC0, 0X60, 0X8E, 0X00,
  0X29, 0XC6, 0X20, 0X84, 0X14, 0X42, 0XA6, 0X18, 0X80, 0X10, 0X52, 0X06, 0XA1,
  0X9B, 0X86, 0XCB, 0X9F, 0XB0, 0X87, 0X90, 0XFC, 0X95, 0X90, 0X56, 0X62, 0XF2,
  0X8B, 0XDB, 0X46, 0XC5, 0X18, 0X63, 0X10, 0X2A, 0XF7, 0X0C, 0X97, 0X3F, 0X61,
  0X0F, 0X21, 0XF9, 0X21, 0XD0, 0X0C, 0X0B, 0X81, 0X82, 0X55, 0X18, 0X45, 0X18,
  0X1B, 0X63, 0X0C, 0X42, 0XC8, 0XA0, 0X36, 0X47, 0X10, 0X14, 0X83, 0X91, 0X42,
  0XC8, 0X23, 0X38, 0X10, 0X30, 0X8C, 0X40, 0X0C, 0X33, 0XB5, 0XC1, 0X38, 0XB0,
  0X43, 0X38, 0XCC, 0XC3, 0X3C, 0XB8, 0X01, 0X2D, 0X94, 0X03, 0X3E, 0XD0, 0X43,
  0X3D, 0XC8, 0X43, 0X39, 0XC8, 0X01, 0X29, 0XF0, 0X81, 0X3D, 0X94, 0XC3, 0X38,
  0XD0, 0XC3, 0X3B, 0XC8, 0X03, 0X1F, 0X98, 0X03, 0X3B, 0XBC, 0X43, 0X38, 0XD0,
  0X03, 0X1B, 0X80, 0X01, 0X1D, 0XF8, 0X01, 0X18, 0XF8, 0X81, 0X1E, 0XE8, 0X41,
  0X3B, 0XA4, 0X03, 0X3C, 0XCC, 0XC3, 0X2F, 0XD0, 0X43, 0X3E, 0XC0, 0X43, 0X39,
  0XA0, 0X80, 0XCC, 0X24, 0X06, 0XE3, 0XC0, 0X0E, 0XE1, 0X30, 0X0F, 0XF3, 0XE0,
  0X06, 0XB4, 0X50, 0X0E, 0XF8, 0X40, 0X0F, 0XF5, 0X20, 0X0F, 0XE5, 0X20, 0X07,
  0XA4, 0XC0, 0X07, 0XF6, 0X50, 0X0E, 0XE3, 0X40, 0X0F, 0XEF, 0X20, 0X0F, 0X7C,
  0X60, 0X0E, 0XEC, 0XF0, 0X0E, 0XE1, 0X40, 0X0F, 0X6C, 0X00, 0X06, 0X74, 0XE0,
  0X07, 0X60, 0XE0, 0X07, 0X48, 0X98, 0X94, 0XEA, 0X4D, 0XD2, 0X14, 0X51, 0XC2,
  0XE4, 0XB3, 0X00, 0XF3, 0X2C, 0X44, 0XC4, 0X4E, 0XC0, 0X44, 0XA0, 0X80, 0XD0,
  0X4D, 0X05, 0X02, 0X00, 0X13, 0X14, 0X72, 0XC0, 0X87, 0X74, 0X60, 0X87, 0X36,
  0X68, 0X87, 0X79, 0X68, 0X03, 0X72, 0XC0, 0X87, 0X0D, 0XAF, 0X50, 0X0E, 0X6D,
  0XD0, 0X0E, 0X7A, 0X50, 0X0E, 0X6D, 0X00, 0X0F, 0X7A, 0X30, 0X07, 0X72, 0XA0,
  0X07, 0X73, 0X20, 0X07, 0X6D, 0X90, 0X0E, 0X71, 0XA0, 0X07, 0X73, 0X20, 0X07,
  0X6D, 0X90, 0X0E, 0X78, 0XA0, 0X07, 0X73, 0X20, 0X07, 0X6D, 0X90, 0X0E, 0X71,
  0X60, 0X07, 0X7A, 0X30, 0X07, 0X72, 0XD0, 0X06, 0XE9, 0X30, 0X07, 0X72, 0XA0,
  0X07, 0X73, 0X20, 0X07, 0X6D, 0X90, 0X0E, 0X76, 0X40, 0X07, 0X7A, 0X60, 0X07,
  0X74, 0XD0, 0X06, 0XE6, 0X10, 0X07, 0X76, 0XA0, 0X07, 0X73, 0X20, 0X07, 0X6D,
  0X60, 0X0E, 0X73, 0X20, 0X07, 0X7A, 0X30, 0X07, 0X72, 0XD0, 0X06, 0XE6, 0X60,
  0X07, 0X74, 0XA0, 0X07, 0X76, 0X40, 0X07, 0X6D, 0XE0, 0X0E, 0X78, 0XA0, 0X07,
  0X71, 0X60, 0X07, 0X7A, 0X30, 0X07, 0X72, 0XA0, 0X07, 0X76, 0X40, 0X07, 0X43,
  0X9E, 0X00, 0X08, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X86,
  0X3C, 0X06, 0X10, 0X00, 0X01, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X0C,
  0X79, 0X10, 0X20, 0X00, 0X04, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X18,
  0XF2, 0X34, 0X40, 0X00, 0X0C, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X30,
  0XE4, 0X81, 0X80, 0X00, 0X18, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X20,
  0X0B, 0X04, 0X00, 0X00, 0X0F, 0X00, 0X00, 0X00, 0X32, 0X1E, 0X98, 0X14, 0X19,
  0X11, 0X4C, 0X90, 0X8C, 0X09, 0X26, 0X47, 0XC6, 0X04, 0X43, 0X22, 0X25, 0X30,
  0X02, 0X50, 0X0C, 0X45, 0X50, 0X12, 0X65, 0X50, 0X1E, 0X85, 0X50, 0X2C, 0X54,
  0X4A, 0X62, 0X04, 0XA0, 0X08, 0X0A, 0XA1, 0X40, 0XC8, 0XCE, 0X00, 0X10, 0X9E,
  0X01, 0XA0, 0X3C, 0X96, 0X82, 0X10, 0XCF, 0X03, 0X00, 0X81, 0X40, 0X20, 0X00,
  0X00, 0X00, 0X00, 0X79, 0X18, 0X00, 0X00, 0X81, 0X00, 0X00, 0X00, 0X1A, 0X03,
  0X4C, 0X90, 0X46, 0X02, 0X13, 0XC4, 0X31, 0X20, 0XC3, 0X1B, 0X43, 0X81, 0X93,
  0X4B, 0XB3, 0X0B, 0XA3, 0X2B, 0X4B, 0X01, 0X89, 0X71, 0XC9, 0X71, 0X81, 0X71,
  0XA9, 0X89, 0XC1, 0XA1, 0X01, 0X41, 0X91, 0X89, 0X21, 0X93, 0XC1, 0X31, 0XBB,
  0X91, 0XB9, 0X49, 0XD9, 0X10, 0X04, 0X13, 0X04, 0XA2, 0X98, 0X20, 0X10, 0XC6,
  0X06, 0X61, 0X20, 0X36, 0X08, 0X04, 0X41, 0X01, 0X6E, 0X6E, 0X82, 0X40, 0X1C,
  0X1B, 0X86, 0X03, 0X21, 0X26, 0X08, 0XD6, 0XC4, 0XA7, 0XCE, 0X2D, 0XED, 0X8B,
  0XAE, 0X0C, 0XEF, 0X6B, 0X2E, 0XAC, 0X0D, 0X8E, 0XAD, 0X4C, 0X6E, 0X82, 0X40,
  0X20, 0X13, 0X04, 0X22, 0XD9, 0X20, 0X10, 0XCD, 0X86, 0X84, 0X50, 0X16, 0X86,
  0X18, 0X18, 0XC2, 0XD9, 0X10, 0X3C, 0X13, 0X04, 0X8C, 0X22, 0XF6, 0X55, 0XE7,
  0X96, 0XF6, 0X45, 0X57, 0X86, 0XF7, 0X35, 0X17, 0XD6, 0X06, 0XC7, 0X56, 0X26,
  0XF7, 0X35, 0X17, 0XD6, 0X06, 0XC7, 0X56, 0X26, 0XB7, 0X01, 0X21, 0X22, 0X89,
  0X21, 0X06, 0X02, 0XD8, 0X10, 0X4C, 0X1B, 0X08, 0X08, 0X00, 0XA8, 0X09, 0X82,
  0X00, 0X6C, 0X00, 0X36, 0X0C, 0XC4, 0X75, 0X6D, 0X08, 0XB0, 0X0D, 0XC3, 0X60,
  0X65, 0X13, 0X84, 0XAC, 0XDA, 0X10, 0X6C, 0X24, 0XDA, 0XC2, 0XD2, 0XDC, 0X88,
  0X50, 0X15, 0X61, 0X0D, 0X3D, 0X3D, 0X49, 0X11, 0X4D, 0X10, 0X0A, 0X67, 0X82,
  0X50, 0X3C, 0X1B, 0X02, 0X62, 0X82, 0X50, 0X40, 0X13, 0X04, 0X42, 0XD9, 0X20,
  0X8C, 0XC1, 0X18, 0X6C, 0X58, 0X08, 0XEF, 0X03, 0X83, 0X30, 0X10, 0X83, 0X41,
  0X0C, 0X08, 0X30, 0X20, 0X83, 0X0D, 0XC1, 0X30, 0X41, 0X28, 0XA2, 0X09, 0X02,
  0XB1, 0X6C, 0X10, 0XC6, 0X00, 0X0D, 0X36, 0X2C, 0X83, 0XF7, 0X81, 0X81, 0X19,
  0X88, 0XC1, 0X70, 0X06, 0X03, 0X18, 0XA4, 0XC1, 0X06, 0XA1, 0X0C, 0XD4, 0X80,
  0XC9, 0X94, 0XD5, 0X17, 0X55, 0X98, 0XDC, 0X59, 0X19, 0XDD, 0X04, 0XA1, 0X90,
  0X36, 0X2C, 0X04, 0X1B, 0X7C, 0X6D, 0X10, 0X06, 0X60, 0X30, 0X9C, 0X01, 0X01,
  0X06, 0X69, 0XB0, 0X21, 0X70, 0X83, 0X0D, 0XC3, 0X1A, 0XBC, 0X01, 0XB0, 0XA1,
  0XB0, 0X3A, 0X38, 0XA8, 0X00, 0X1A, 0X66, 0X6C, 0X6F, 0X61, 0X74, 0X73, 0X13,
  0X04, 0X82, 0X61, 0X91, 0XE6, 0X36, 0X47, 0X37, 0X37, 0X41, 0X20, 0X1A, 0X1A,
  0X73, 0X69, 0X67, 0X5F, 0X6C, 0X64, 0X34, 0XE6, 0XD2, 0XCE, 0XBE, 0XE6, 0XE8,
  0X88, 0XD0, 0X95, 0XE1, 0X7D, 0XB9, 0XBD, 0XC9, 0XB5, 0X6D, 0X50, 0XE4, 0X60,
  0X0E, 0XE8, 0XA0, 0X0E, 0XEC, 0X00, 0XB9, 0X83, 0X39, 0XC0, 0X83, 0XA1, 0X0A,
  0X1B, 0X9B, 0X5D, 0X9B, 0X4B, 0X1A, 0X59, 0X99, 0X1B, 0XDD, 0X94, 0X20, 0XA8,
  0X42, 0X86, 0XE7, 0X62, 0X57, 0X26, 0X37, 0X97, 0XF6, 0XE6, 0X36, 0X25, 0X20,
  0X9A, 0X90, 0XE1, 0XB9, 0XD8, 0X85, 0XB1, 0XD9, 0X95, 0XC9, 0X4D, 0X09, 0X8A,
  0X3A, 0X64, 0X78, 0X2E, 0X73, 0X68, 0X61, 0X64, 0X65, 0X72, 0X4D, 0X6F, 0X64,
  0X65, 0X6C, 0X53, 0X02, 0XA4, 0X0C, 0X19, 0X9E, 0X8B, 0X5C, 0XD9, 0XDC, 0X5B,
  0X9D, 0XDC, 0X58, 0XD9, 0XDC, 0X94, 0X80, 0XAA, 0X44, 0X86, 0XE7, 0X42, 0X97,
  0X07, 0X57, 0X16, 0XE4, 0XE6, 0XF6, 0X46, 0X17, 0X46, 0X97, 0XF6, 0XE6, 0X36,
  0X37, 0X25, 0XC8, 0XEA, 0X90, 0XE1, 0XB9, 0XD8, 0XA5, 0X95, 0XDD, 0X25, 0X91,
  0X4D, 0XD1, 0X85, 0XD1, 0X95, 0X4D, 0X09, 0XB6, 0X3A, 0X64, 0X78, 0X2E, 0X65,
  0X6E, 0X74, 0X72, 0X79, 0X50, 0X6F, 0X69, 0X6E, 0X74, 0X73, 0X53, 0X02, 0X38,
  0XE8, 0X42, 0X86, 0XE7, 0X32, 0XF6, 0X56, 0XE7, 0X46, 0X57, 0X26, 0X37, 0X37,
  0X25, 0XC0, 0X03, 0X00, 0X00, 0X00, 0X00, 0X79, 0X18, 0X00, 0X00, 0X4C, 0X00,
  0X00, 0X00, 0X33, 0X08, 0X80, 0X1C, 0XC4, 0XE1, 0X1C, 0X66, 0X14, 0X01, 0X3D,
  0X88, 0X43, 0X38, 0X84, 0XC3, 0X8C, 0X42, 0X80, 0X07, 0X79, 0X78, 0X07, 0X73,
  0X98, 0X71, 0X0C, 0XE6, 0X00, 0X0F, 0XED, 0X10, 0X0E, 0XF4, 0X80, 0X0E, 0X33,
  0X0C, 0X42, 0X1E, 0XC2, 0XC1, 0X1D, 0XCE, 0XA1, 0X1C, 0X66, 0X30, 0X05, 0X3D,
  0X88, 0X43, 0X38, 0X84, 0X83, 0X1B, 0XCC, 0X03, 0X3D, 0XC8, 0X43, 0X3D, 0X8C,
  0X03, 0X3D, 0XCC, 0X78, 0X8C, 0X74, 0X70, 0X07, 0X7B, 0X08, 0X07, 0X79, 0X48,
  0X87, 0X70, 0X70, 0X07, 0X7A, 0X70, 0X03, 0X76, 0X78, 0X87, 0X70, 0X20, 0X87,
  0X19, 0XCC, 0X11, 0X0E, 0XEC, 0X90, 0X0E, 0XE1, 0X30, 0X0F, 0X6E, 0X30, 0X0F,
  0XE3, 0XF0, 0X0E, 0XF0, 0X50, 0X0E, 0X33, 0X10, 0XC4, 0X1D, 0XDE, 0X21, 0X1C,
  0XD8, 0X21, 0X1D, 0XC2, 0X61, 0X1E, 0X66, 0X30, 0X89, 0X3B, 0XBC, 0X83, 0X3B,
  0XD0, 0X43, 0X39, 0XB4, 0X03, 0X3C, 0XBC, 0X83, 0X3C, 0X84, 0X03, 0X3B, 0XCC,
  0XF0, 0X14, 0X76, 0X60, 0X07, 0X7B, 0X68, 0X07, 0X37, 0X68, 0X87, 0X72, 0X68,
  0X07, 0X37, 0X80, 0X87, 0X70, 0X90, 0X87, 0X70, 0X60, 0X07, 0X76, 0X28, 0X07,
  0X76, 0XF8, 0X05, 0X76, 0X78, 0X87, 0X77, 0X80, 0X87, 0X5F, 0X08, 0X87, 0X71,
  0X18, 0X87, 0X72, 0X98, 0X87, 0X79, 0X98, 0X81, 0X2C, 0XEE, 0XF0, 0X0E, 0XEE,
  0XE0, 0X0E, 0XF5, 0XC0, 0X0E, 0XEC, 0X30, 0X03, 0X62, 0XC8, 0XA1, 0X1C, 0XE4,
  0XA1, 0X1C, 0XCC, 0XA1, 0X1C, 0XE4, 0XA1, 0X1C, 0XDC, 0X61, 0X1C, 0XCA, 0X21,
  0X1C, 0XC4, 0X81, 0X1D, 0XCA, 0X61, 0X06, 0XD6, 0X90, 0X43, 0X39, 0XC8, 0X43,
  0X39, 0X98, 0X43, 0X39, 0XC8, 0X43, 0X39, 0XB8, 0XC3, 0X38, 0X94, 0X43, 0X38,
  0X88, 0X03, 0X3B, 0X94, 0XC3, 0X2F, 0XBC, 0X83, 0X3C, 0XFC, 0X82, 0X3B, 0XD4,
  0X03, 0X3B, 0XB0, 0XC3, 0X8C, 0XC8, 0X21, 0X07, 0X7C, 0X70, 0X03, 0X72, 0X10,
  0X87, 0X73, 0X70, 0X03, 0X7B, 0X08, 0X07, 0X79, 0X60, 0X87, 0X70, 0XC8, 0X87,
  0X77, 0XA8, 0X07, 0X7A, 0X98, 0X81, 0X3C, 0XE4, 0X80, 0X0F, 0X6E, 0X40, 0X0F,
  0XE5, 0XD0, 0X0E, 0XF0, 0X00, 0X00, 0X00, 0X71, 0X20, 0X00, 0X00, 0X12, 0X00,
  0X00, 0X00, 0X46, 0X20, 0X0D, 0X97, 0XEF, 0X3C, 0XBE, 0X10, 0X11, 0XC0, 0X44,
  0X84, 0X40, 0X33, 0X2C, 0X84, 0X05, 0X4C, 0XC3, 0XE5, 0X3B, 0X8F, 0XBF, 0X38,
  0XC0, 0X20, 0X36, 0X0F, 0X35, 0XF9, 0XC5, 0X6D, 0XDB, 0X00, 0X34, 0X5C, 0XBE,
  0XF3, 0XF8, 0X12, 0XC0, 0X3C, 0X0B, 0XE1, 0X17, 0XB7, 0X6D, 0X02, 0XD5, 0X70,
  0XF9, 0XCE, 0XE3, 0X4B, 0X93, 0X13, 0X11, 0X28, 0X35, 0X3D, 0XD4, 0XE4, 0X17,
  0XB7, 0X6D, 0X00, 0X04, 0X03, 0X20, 0X0D, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
  0X48, 0X41, 0X53, 0X48, 0X14, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0XDF,
  0XCB, 0XF8, 0X1F, 0XBB, 0X62, 0X8D, 0X9E, 0X24, 0XD2, 0X98, 0X2E, 0XB8, 0XB8,
  0X38, 0XF1, 0X44, 0X58, 0X49, 0X4C, 0XC0, 0X06, 0X00, 0X00, 0X60, 0X00, 0X00,
  0X00, 0XB0, 0X01, 0X00, 0X00, 0X44, 0X58, 0X49, 0X4C, 0X00, 0X01, 0X00, 0X00,
  0X10, 0X00, 0X00, 0X00, 0XA8, 0X06, 0X00, 0X00, 0X42, 0X43, 0XC0, 0XDE, 0X21,
  0X0C, 0X00, 0X00, 0XA7, 0X01, 0X00, 0X00, 0X0B, 0X82, 0X20, 0X00, 0X02, 0X00,
  0X00, 0X00, 0X13, 0X00, 0X00, 0X00, 0X07, 0X81, 0X23, 0X91, 0X41, 0XC8, 0X04,
  0X49, 0X06, 0X10, 0X32, 0X39, 0X92, 0X01, 0X84, 0X0C, 0X25, 0X05, 0X08, 0X19,
  0X1E, 0X04, 0X8B, 0X62, 0X80, 0X14, 0X45, 0X02, 0X42, 0X92, 0X0B, 0X42, 0XA4,
  0X10, 0X32, 0X14, 0X38, 0X08, 0X18, 0X4B, 0X0A, 0X32, 0X52, 0X88, 0X48, 0X90,
  0X14, 0X20, 0X43, 0X46, 0X88, 0XA5, 0X00, 0X19, 0X32, 0X42, 0XE4, 0X48, 0X0E,
  0X90, 0X91, 0X22, 0XC4, 0X50, 0X41, 0X51, 0X81, 0X8C, 0XE1, 0X83, 0XE5, 0X8A,
  0X04, 0X29, 0X46, 0X06, 0X51, 0X18, 0X00, 0X00, 0X08, 0X00, 0X00, 0X00, 0X1B,
  0X8C, 0XE0, 0XFF, 0XFF, 0XFF, 0XFF, 0X07, 0X40, 0X02, 0XA8, 0X0D, 0X84, 0XF0,
  0XFF, 0XFF, 0XFF, 0XFF, 0X03, 0X20, 0X6D, 0X30, 0X86, 0XFF, 0XFF, 0XFF, 0XFF,
  0X1F, 0X00, 0X09, 0XA8, 0X00, 0X49, 0X18, 0X00, 0X00, 0X03, 0X00, 0X00, 0X00,
  0X13, 0X82, 0X60, 0X42, 0X20, 0X4C, 0X08, 0X06, 0X00, 0X00, 0X00, 0X00, 0X89,
  0X20, 0X00, 0X00, 0X43, 0X00, 0X00, 0X00, 0X32, 0X22, 0X48, 0X09, 0X20, 0X64,
  0X85, 0X04, 0X93, 0X22, 0XA4, 0X84, 0X04, 0X93, 0X22, 0XE3, 0X84, 0XA1, 0X90,
  0X14, 0X12, 0X4C, 0X8A, 0X8C, 0X0B, 0X84, 0XA4, 0X4C, 0X10, 0X68, 0X23, 0X00,
  0X25, 0X00, 0X14, 0X66, 0X00, 0XE6, 0X08, 0XC0, 0X60, 0X8E, 0X00, 0X29, 0XC6,
  0X20, 0X84, 0X14, 0X42, 0XA6, 0X18, 0X80, 0X10, 0X52, 0X06, 0XA1, 0X9B, 0X86,
  0XCB, 0X9F, 0XB0, 0X87, 0X90, 0XFC, 0X95, 0X90, 0X56, 0X62, 0XF2, 0X8B, 0XDB,
  0X46, 0XC5, 0X18, 0X63, 0X10, 0X2A, 0XF7, 0X0C, 0X97, 0X3F, 0X61, 0X0F, 0X21,
  0XF9, 0X21, 0XD0, 0X0C, 0X0B, 0X81, 0X82, 0X55, 0X18, 0X45, 0X18, 0X1B, 0X63,
  0X0C, 0X42, 0XC8, 0XA0, 0X36, 0X47, 0X10, 0X14, 0X83, 0X91, 0X42, 0XC8, 0X23,
  0X38, 0X10, 0X30, 0X8C, 0X40, 0X0C, 0X33, 0XB5, 0XC1, 0X38, 0XB0, 0X43, 0X38,
  0XCC, 0XC3, 0X3C, 0XB8, 0X01, 0X2D, 0X94, 0X03, 0X3E, 0XD0, 0X43, 0X3D, 0XC8,
  0X43, 0X39, 0XC8, 0X01, 0X29, 0XF0, 0X81, 0X3D, 0X94, 0XC3, 0X38, 0XD0, 0XC3,
  0X3B, 0XC8, 0X03, 0X1F, 0X98, 0X03, 0X3B, 0XBC, 0X43, 0X38, 0XD0, 0X03, 0X1B,
  0X80, 0X01, 0X1D, 0XF8, 0X01, 0X18, 0XF8, 0X81, 0X1E, 0XE8, 0X41, 0X3B, 0XA4,
  0X03, 0X3C, 0XCC, 0XC3, 0X2F, 0XD0, 0X43, 0X3E, 0XC0, 0X43, 0X39, 0XA0, 0X80,
  0XCC, 0X24, 0X06, 0XE3, 0XC0, 0X0E, 0XE1, 0X30, 0X0F, 0XF3, 0XE0, 0X06, 0XB4,
  0X50, 0X0E, 0XF8, 0X40, 0X0F, 0XF5, 0X20, 0X0F, 0XE5, 0X20, 0X07, 0XA4, 0XC0,
  0X07, 0XF6, 0X50, 0X0E, 0XE3, 0X40, 0X0F, 0XEF, 0X20, 0X0F, 0X7C, 0X60, 0X0E,
  0XEC, 0XF0, 0X0E, 0XE1, 0X40, 0X0F, 0X6C, 0X00, 0X06, 0X74, 0XE0, 0X07, 0X60,
  0XE0, 0X07, 0X48, 0X98, 0X94, 0XEA, 0X4D, 0XD2, 0X14, 0X51, 0XC2, 0XE4, 0XB3,
  0X00, 0XF3, 0X2C, 0X44, 0XC4, 0X4E, 0XC0, 0X44, 0XA0, 0X80, 0XD0, 0X4D, 0X05,
  0X02, 0X00, 0X13, 0X14, 0X72, 0XC0, 0X87, 0X74, 0X60, 0X87, 0X36, 0X68, 0X87,
  0X79, 0X68, 0X03, 0X72, 0XC0, 0X87, 0X0D, 0XAF, 0X50, 0X0E, 0X6D, 0XD0, 0X0E,
  0X7A, 0X50, 0X0E, 0X6D, 0X00, 0X0F, 0X7A, 0X30, 0X07, 0X72, 0XA0, 0X07, 0X73,
  0X20, 0X07, 0X6D, 0X90, 0X0E, 0X71, 0XA0, 0X07, 0X73, 0X20, 0X07, 0X6D, 0X90,
  0X0E, 0X78, 0XA0, 0X07, 0X73, 0X20, 0X07, 0X6D, 0X90, 0X0E, 0X71, 0X60, 0X07,
  0X7A, 0X30, 0X07, 0X72, 0XD0, 0X06, 0XE9, 0X30, 0X07, 0X72, 0XA0, 0X07, 0X73,
  0X20, 0X07, 0X6D, 0X90, 0X0E, 0X76, 0X40, 0X07, 0X7A, 0X60, 0X07, 0X74, 0XD0,
  0X06, 0XE6, 0X10, 0X07, 0X76, 0XA0, 0X07, 0X73, 0X20, 0X07, 0X6D, 0X60, 0X0E,
  0X73, 0X20, 0X07, 0X7A, 0X30, 0X07, 0X72, 0XD0, 0X06, 0XE6, 0X60, 0X07, 0X74,
  0XA0, 0X07, 0X76, 0X40, 0X07, 0X6D, 0XE0, 0X0E, 0X78, 0XA0, 0X07, 0X71, 0X60,
  0X07, 0X7A, 0X30, 0X07, 0X72, 0XA0, 0X07, 0X76, 0X40, 0X07, 0X43, 0X9E, 0X00,
  0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X86, 0X3C, 0X06,
  0X10, 0X00, 0X01, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X0C, 0X79, 0X10,
  0X20, 0X00, 0X04, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X18, 0XF2, 0X34,
  0X40, 0X00, 0X0C, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X30, 0XE4, 0X81,
  0X80, 0X00, 0X18, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X20, 0X0B, 0X04,
  0X00, 0X00, 0X0E, 0X00, 0X00, 0X00, 0X32, 0X1E, 0X98, 0X14, 0X19, 0X11, 0X4C,
  0X90, 0X8C, 0X09, 0X26, 0X47, 0XC6, 0X04, 0X43, 0X22, 0X25, 0X30, 0X02, 0X50,
  0X12, 0XC5, 0X50, 0X04, 0X65, 0X50, 0X1E, 0X54, 0X4A, 0X62, 0X04, 0XA0, 0X08,
  0X0A, 0XA1, 0X40, 0XC8, 0XCE, 0X00, 0X10, 0X9E, 0X01, 0XA0, 0X3C, 0X96, 0X82,
  0X10, 0XCF, 0X03, 0X00, 0X81, 0X40, 0X20, 0X00, 0X00, 0X00, 0X79, 0X18, 0X00,
  0X00, 0X59, 0X00, 0X00, 0X00, 0X1A, 0X03, 0X4C, 0X90, 0X46, 0X02, 0X13, 0XC4,
  0X31, 0X20, 0XC3, 0X1B, 0X43, 0X81, 0X93, 0X4B, 0XB3, 0X0B, 0XA3, 0X2B, 0X4B,
  0X01, 0X89, 0X71, 0XC9, 0X71, 0X81, 0X71, 0XA9, 0X89, 0XC1, 0XA1, 0X01, 0X41,
  0X91, 0X89, 0X21, 0X93, 0XC1, 0X31, 0XBB, 0X91, 0XB9, 0X49, 0XD9, 0X10, 0X04,
  0X13, 0X04, 0XA2, 0X98, 0X20, 0X10, 0XC6, 0X06, 0X61, 0X20, 0X26, 0X08, 0XC4,
  0XB1, 0X41, 0X18, 0X0C, 0X0A, 0X70, 0X73, 0X13, 0X04, 0X02, 0XD9, 0X30, 0X20,
  0X09, 0X31, 0X41, 0XB0, 0X22, 0X02, 0X13, 0X04, 0X22, 0XD9, 0X20, 0X10, 0XC6,
  0X86, 0X84, 0X58, 0X98, 0X86, 0X18, 0X1A, 0XC2, 0XD9, 0X10, 0X3C, 0X13, 0X04,
  0X4C, 0XDA, 0X80, 0X10, 0X11, 0XD3, 0X10, 0X03, 0X01, 0X6C, 0X08, 0XA4, 0X0D,
  0X04, 0X04, 0X00, 0XD3, 0X04, 0X21, 0X9B, 0X36, 0X04, 0XD5, 0X04, 0X41, 0X00,
  0X48, 0XB4, 0X85, 0XA5, 0XB9, 0X11, 0XA1, 0X2A, 0XC2, 0X1A, 0X7A, 0X7A, 0X92,
  0X22, 0X9A, 0X20, 0X14, 0XCC, 0X04, 0XA1, 0X68, 0X36, 0X04, 0XC4, 0X04, 0XA1,
  0X70, 0X26, 0X08, 0X84, 0XB2, 0X41, 0XF0, 0XBC, 0X0D, 0X0B, 0X91, 0X69, 0X1B,
  0XD7, 0X0D, 0X1D, 0XB1, 0X7D, 0X1B, 0X82, 0X61, 0X82, 0X50, 0X3C, 0X13, 0X04,
  0X62, 0XD9, 0X20, 0X78, 0X63, 0XB0, 0X61, 0X19, 0X32, 0X6D, 0X0B, 0X83, 0X6E,
  0X10, 0X83, 0X61, 0X23, 0X83, 0X0D, 0X02, 0X18, 0X94, 0X01, 0X93, 0X29, 0XAB,
  0X2F, 0XAA, 0X30, 0XB9, 0XB3, 0X32, 0XBA, 0X09, 0X42, 0X01, 0X6D, 0X58, 0X88,
  0X33, 0XD0, 0XD0, 0X80, 0XDB, 0X06, 0X31, 0X20, 0X36, 0X32, 0XD8, 0X10, 0XA4,
  0XC1, 0X86, 0XC1, 0X0C, 0XD4, 0X00, 0XD8, 0X50, 0X5C, 0XD8, 0X1A, 0X50, 0X40,
  0X15, 0X36, 0X36, 0XBB, 0X36, 0X97, 0X34, 0XB2, 0X32, 0X37, 0XBA, 0X29, 0X41,
  0X50, 0X85, 0X0C, 0XCF, 0XC5, 0XAE, 0X4C, 0X6E, 0X2E, 0XED, 0XCD, 0X6D, 0X4A,
  0X40, 0X34, 0X21, 0XC3, 0X73, 0XB1, 0X0B, 0X63, 0XB3, 0X2B, 0X93, 0X9B, 0X12,
  0X18, 0X75, 0XC8, 0XF0, 0X5C, 0XE6, 0XD0, 0XC2, 0XC8, 0XCA, 0XE4, 0X9A, 0XDE,
  0XC8, 0XCA, 0XD8, 0XA6, 0X04, 0X49, 0X19, 0X32, 0X3C, 0X17, 0XB9, 0XB2, 0XB9,
  0XB7, 0X3A, 0XB9, 0XB1, 0XB2, 0XB9, 0X29, 0XC1, 0X54, 0X87, 0X0C, 0XCF, 0XC5,
  0X2E, 0XAD, 0XEC, 0X2E, 0X89, 0X6C, 0X8A, 0X2E, 0X8C, 0XAE, 0X6C, 0X4A, 0X50,
  0XD5, 0X21, 0XC3, 0X73, 0X29, 0X73, 0XA3, 0X93, 0XCB, 0X83, 0X7A, 0X4B, 0X73,
  0XA3, 0X9B, 0X9B, 0X12, 0XAC, 0X01, 0X00, 0X00, 0X00, 0X00, 0X79, 0X18, 0X00,
  0X00, 0X4C, 0X00, 0X00, 0X00, 0X33, 0X08, 0X80, 0X1C, 0XC4, 0XE1, 0X1C, 0X66,
  0X14, 0X01, 0X3D, 0X88, 0X43, 0X38, 0X84, 0XC3, 0X8C, 0X42, 0X80, 0X07, 0X79,
  0X78, 0X07, 0X73, 0X98, 0X71, 0X0C, 0XE6, 0X00, 0X0F, 0XED, 0X10, 0X0E, 0XF4,
  0X80, 0X0E, 0X33, 0X0C, 0X42, 0X1E, 0XC2, 0XC1, 0X1D, 0XCE, 0XA1, 0X1C, 0X66,
  0X30, 0X05, 0X3D, 0X88, 0X43, 0X38, 0X84, 0X83, 0X1B, 0XCC, 0X03, 0X3D, 0XC8,
  0X43, 0X3D, 0X8C, 0X03, 0X3D, 0XCC, 0X78, 0X8C, 0X74, 0X70, 0X07, 0X7B, 0X08,
  0X07, 0X79, 0X48, 0X87, 0X70, 0X70, 0X07, 0X7A, 0X70, 0X03, 0X76, 0X78, 0X87,
  0X70, 0X20, 0X87, 0X19, 0XCC, 0X11, 0X0E, 0XEC, 0X90, 0X0E, 0XE1, 0X30, 0X0F,
  0X6E, 0X30, 0X0F, 0XE3, 0XF0, 0X0E, 0XF0, 0X50, 0X0E, 0X33, 0X10, 0XC4, 0X1D,
  0XDE, 0X21, 0X1C, 0XD8, 0X21, 0X1D, 0XC2, 0X61, 0X1E, 0X66, 0X30, 0X89, 0X3B,
  0XBC, 0X83, 0X3B, 0XD0, 0X43, 0X39, 0XB4, 0X03, 0X3C, 0XBC, 0X83, 0X3C, 0X84,
  0X03, 0X3B, 0XCC, 0XF0, 0X14, 0X76, 0X60, 0X07, 0X7B, 0X68, 0X07, 0X37, 0X68,
  0X87, 0X72, 0X68, 0X07, 0X37, 0X80, 0X87, 0X70, 0X90, 0X87, 0X70, 0X60, 0X07,
  0X76, 0X28, 0X07, 0X76, 0XF8, 0X05, 0X76, 0X78, 0X87, 0X77, 0X80, 0X87, 0X5F,
  0X08, 0X87, 0X71, 0X18, 0X87, 0X72, 0X98, 0X87, 0X79, 0X98, 0X81, 0X2C, 0XEE,
  0XF0, 0X0E, 0XEE, 0XE0, 0X0E, 0XF5, 0XC0, 0X0E, 0XEC, 0X30, 0X03, 0X62, 0XC8,
  0XA1, 0X1C, 0XE4, 0XA1, 0X1C, 0XCC, 0XA1, 0X1C, 0XE4, 0XA1, 0X1C, 0XDC, 0X61,
  0X1C, 0XCA, 0X21, 0X1C, 0XC4, 0X81, 0X1D, 0XCA, 0X61, 0X06, 0XD6, 0X90, 0X43,
  0X39, 0XC8, 0X43, 0X39, 0X98, 0X43, 0X39, 0XC8, 0X43, 0X39, 0XB8, 0XC3, 0X38,
  0X94, 0X43, 0X38, 0X88, 0X03, 0X3B, 0X94, 0XC3, 0X2F, 0XBC, 0X83, 0X3C, 0XFC,
  0X82, 0X3B, 0XD4, 0X03, 0X3B, 0XB0, 0XC3, 0X8C, 0XC8, 0X21, 0X07, 0X7C, 0X70,
  0X03, 0X72, 0X10, 0X87, 0X73, 0X70, 0X03, 0X7B, 0X08, 0X07, 0X79, 0X60, 0X87,
  0X70, 0XC8, 0X87, 0X77, 0XA8, 0X07, 0X7A, 0X98, 0X81, 0X3C, 0XE4, 0X80, 0X0F,
  0X6E, 0X40, 0X0F, 0XE5, 0XD0, 0X0E, 0XF0, 0X00, 0X00, 0X00, 0X71, 0X20, 0X00,
  0X00, 0X12, 0X00, 0X00, 0X00, 0X46, 0X20, 0X0D, 0X97, 0XEF, 0X3C, 0XBE, 0X10,
  0X11, 0XC0, 0X44, 0X84, 0X40, 0X33, 0X2C, 0X84, 0X05, 0X4C, 0XC3, 0XE5, 0X3B,
  0X8F, 0XBF, 0X38, 0XC0, 0X20, 0X36, 0X0F, 0X35, 0XF9, 0XC5, 0X6D, 0XDB, 0X00,
  0X34, 0X5C, 0XBE, 0XF3, 0XF8, 0X12, 0XC0, 0X3C, 0X0B, 0XE1, 0X17, 0XB7, 0X6D,
  0X02, 0XD5, 0X70, 0XF9, 0XCE, 0XE3, 0X4B, 0X93, 0X13, 0X11, 0X28, 0X35, 0X3D,
  0XD4, 0XE4, 0X17, 0XB7, 0X6D, 0X00, 0X04, 0X03, 0X20, 0X0D, 0X00, 0X00, 0X61,
  0X20, 0X00, 0X00, 0X3A, 0X00, 0X00, 0X00, 0X13, 0X04, 0X41, 0X2C, 0X10, 0X00,
  0X00, 0X00, 0X05, 0X00, 0X00, 0X00, 0XF4, 0X46, 0X00, 0X88, 0XCC, 0X00, 0X14,
  0X42, 0X29, 0X94, 0X5C, 0XE1, 0X51, 0X29, 0X83, 0X12, 0XA0, 0X31, 0X03, 0X00,
  0X23, 0X06, 0X09, 0X00, 0X82, 0X60, 0X00, 0X65, 0X05, 0X74, 0X5D, 0XC9, 0X88,
  0X41, 0X02, 0X80, 0X20, 0X18, 0X40, 0X9A, 0X41, 0X60, 0X98, 0X32, 0X62, 0X90,
  0X00, 0X20, 0X08, 0X06, 0X86, 0X97, 0X68, 0X99, 0XA4, 0X8C, 0X18, 0X24, 0X00,
  0X08, 0X82, 0X81, 0XF1, 0X29, 0X9B, 0X56, 0X2C, 0X23, 0X06, 0X09, 0X00, 0X82,
  0X60, 0X60, 0X80, 0XC1, 0XC2, 0X6D, 0X13, 0X33, 0X62, 0X90, 0X00, 0X20, 0X08,
  0X06, 0X46, 0X18, 0X30, 0X1D, 0X87, 0X34, 0X23, 0X06, 0X09, 0X00, 0X82, 0X60,
  0X60, 0X88, 0X41, 0XD3, 0X75, 0X96, 0X33, 0X62, 0X90, 0X00, 0X20, 0X08, 0X06,
  0XC6, 0X18, 0X38, 0X9E, 0X97, 0X3C, 0X23, 0X06, 0X0F, 0X00, 0X82, 0X60, 0XD0,
  0X88, 0X01, 0X83, 0X1C, 0X42, 0X90, 0X24, 0XDF, 0X07, 0X25, 0XA3, 0X09, 0X01,
  0X30, 0X9A, 0X20, 0X04, 0XA3, 0X09, 0X83, 0X30, 0X9A, 0X40, 0X0C, 0X46, 0X2C,
  0XF2, 0X31, 0X62, 0X91, 0X8F, 0X11, 0X8B, 0X7C, 0X8C, 0X58, 0XE4, 0X33, 0X62,
  0X90, 0X00, 0X20, 0X08, 0X06, 0X08, 0X1B, 0X5C, 0X68, 0X80, 0X06, 0X61, 0X40,
  0X8C, 0X18, 0X24, 0X00, 0X08, 0X82, 0X01, 0XC2, 0X06, 0X17, 0X1A, 0XA0, 0XC1,
  0X34, 0X8C, 0X18, 0X24, 0X00, 0X08, 0X82, 0X01, 0XC2, 0X06, 0X17, 0X1A, 0XA0,
  0X01, 0X18, 0X08, 0X23, 0X06, 0X09, 0X00, 0X82, 0X60, 0X80, 0XB0, 0XC1, 0X85,
  0X06, 0X68, 0X40, 0X05, 0X08, 0X00, 0X00, 0X00, 0X00
};

typedef struct _SDL_GPRegion
{
  float x1, y1, x2, y2;
} _SDL_GPRegion;

typedef enum
{
  _SDL_GP_COMMAND_NONE = 0,
  _SDL_GP_COMMAND_DRAW,
  _SDL_GP_COMMAND_VIEWPORT,
  _SDL_GP_COMMAND_SCISSOR
} _SDL_GPCommandType;

typedef struct _SDL_GPDrawArgs
{
  _SDL_GPRegion region;
  SDL_GPPipeline pipeline;
  SDL_GPTextureUniform texture;
  Uint32 uniform_index;
  Uint32 vertex_index;
  Uint32 vertices_count;
} _SDL_GPDrawArgs;

typedef union _SDL_GPCommandArgs
{
  _SDL_GPDrawArgs draw;
  SDL_GPIRect viewport;
  SDL_GPIRect scissor;
} _SDL_GPCommandArgs;

typedef struct _SDL_GPCommand
{
  _SDL_GPCommandType cmd;
  _SDL_GPCommandArgs args;
} _SDL_GPCommand;

typedef struct _SDL_GP
{
  SDL_GPDesc desc;
  SDL_GPUTransferBuffer *vertex_transfer_buffer;
  SDL_GPUBuffer *vertex_data_buffer;
  SDL_GPShader shader;
  SDL_GPPipeline pipelines[SDL_GP_PRIMITIVE_SIZE * SDL_GP_BLENDMODE_SIZE];
  SDL_GPUSampler *nearest_samplers;
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

static _SDL_GP _gp            = { 0 };
static Uint32 _gp_initialized = 0;

// Create painter common shader.
static SDL_GPShader
_SDL_GPCreateCommonShader(SDL_GPUDevice *gpu_device)
{
  SDL_GPUShaderFormat supported_formats = SDL_GetGPUShaderFormats(gpu_device);
  SDL_GPUShaderFormat format;

  Uint8 *bytecode_vert      = NULL;
  size_t bytecode_vert_size = 0;

  Uint8 *bytecode_frag      = NULL;
  size_t bytecode_frag_size = 0;

  if (supported_formats & SDL_GPU_SHADERFORMAT_SPIRV) {
    format = SDL_GPU_SHADERFORMAT_SPIRV;

    bytecode_vert      = _shader_vert_spv;
    bytecode_vert_size = _shader_vert_spv_len;

    bytecode_frag      = _shader_frag_spv;
    bytecode_frag_size = _shader_farg_spv_len;
  } else if (supported_formats & SDL_GPU_SHADERFORMAT_MSL) {
    format = SDL_GPU_SHADERFORMAT_MSL;

    bytecode_vert      = _shader_vert_msl;
    bytecode_vert_size = _shader_vert_msl_len;

    bytecode_frag      = _shader_frag_msl;
    bytecode_frag_size = _shader_frag_msl_len;
  } else if (supported_formats & SDL_GPU_SHADERFORMAT_DXIL) {
    format             = SDL_GPU_SHADERFORMAT_DXIL;
    bytecode_vert      = _shader_vert_dxil;
    bytecode_vert_size = _shader_vert_dxil_len;

    bytecode_frag      = _shader_frag_dxil;
    bytecode_frag_size = _shader_frag_dxil_len;
  } else {
    _SDL_GPSetError(SDL_GP_ERROR_CREATE_COMMON_SHADER_FAILED);
    return (SDL_GPShader){ .id = SDL_GP_INVALID_ID };
  }

  SDL_GPShaderDesc shader_desc = {
    // Vertex shader description
    .vert_code_size            = bytecode_vert_size,
    .vert_code                 = bytecode_vert,
    .vert_entrypoint           = "main",
    .vert_format               = format,
    .vert_num_samplers         = 0,
    .vert_num_storage_textures = 0,
    .vert_num_storage_buffers  = 0,
    .vert_num_uniform_buffers  = 0,

    // Fragment shader description
    .frag_code_size            = bytecode_frag_size,
    .frag_code                 = bytecode_frag,
    .frag_entrypoint           = "main",
    .frag_format               = format,
    .frag_num_samplers         = SDL_GP_TEXTURE_SLOTS_MAX,
    .frag_num_storage_textures = 0,
    .frag_num_storage_buffers  = 0,
    .frag_num_uniform_buffers  = 0,
  };

  return SDL_GPCreateShader(&shader_desc);
}

static SDL_GPPipeline
_SDL_GP_FindOrCreatePipeline(SDL_GPPrimitiveType primitive_type,
                             SDL_GPBlendMode blend_mode)
{
  SDL_GPPipeline pipeline
      = _gp.pipelines[primitive_type * SDL_GP_BLENDMODE_SIZE + blend_mode];

  if (pipeline.id == SDL_GP_INVALID_ID) {
    pipeline = SDL_GPCreatePipeline(_gp.shader, primitive_type, blend_mode);
    _gp.pipelines[primitive_type * SDL_GP_BLENDMODE_SIZE + blend_mode]
        = pipeline;
  }

  return pipeline;
}

SDL_GP_INLINE SDL_GPUniform *
_SDL_GPNextUniform(void)
{
  if (_gp.current_uniform < SDL_GP_COMMANDS_MAX) {
    return &_gp.uniforms[_gp.current_uniform++];
  } else {
    _SDL_GPSetError(SDL_GP_ERROR_PAINTER_UNIFORMS_FULL);
    return NULL;
  }
}

SDL_GP_INLINE SDL_GPUniform *
_SDL_GPPrevUniform(void)
{
  if (_gp.current_uniform > 0) {
    return &_gp.uniforms[_gp.current_uniform - 1];
  } else {
    return NULL;
  }
}

SDL_GP_INLINE SDL_GPVertex *
_SDL_GPNextVertices(Uint32 count)
{
  if (_gp.current_vertex + count <= SDL_GP_VERTICES_MAX) {
    SDL_GPVertex *vertices = &_gp.vertices[_gp.current_vertex];
    _gp.current_vertex += count;

    return vertices;
  } else {
    _SDL_GPSetError(SDL_GP_ERROR_PAINTER_VERTICES_FULL);
    return NULL;
  }
}

SDL_GP_INLINE _SDL_GPCommand *
_SDL_GPNextCommand(void)
{
  if ((_gp.current_command < SDL_GP_COMMANDS_MAX)) {
    return &_gp.commands[_gp.current_command++];
  } else {
    return NULL;
  }
}

SDL_GP_INLINE _SDL_GPCommand *
_SDL_GPPrevCommand(Uint32 count)
{
  if (_gp.current_command - _gp.state.base_command >= count) {
    return &_gp.commands[_gp.current_command - count];
  } else {
    return NULL;
  }
}

SDL_GP_INLINE SDL_GPMat2x3
_SDL_GPDefaultProjection(int width, int height)
{
  float w = (float)width;
  float h = (float)height;

  return SDL_GPCreateMat2x3(2.0f / w, 0.0f, -1.0f, 0.0f, -2.0f / h, 1.0f);
}

SDL_GP_INLINE SDL_GPMat2x3
_SDL_GPMulProjectionTransform(SDL_GPMat2x3 *projection, SDL_GPMat2x3 *transform)
{
  float x = projection->m00;
  float y = projection->m11;

  SDL_GPMat2x3 out = { 0 };

  out.m00 = x * transform->m00;
  out.m01 = x * transform->m01;
  out.m02 = x * transform->m02 + projection->m02;

  out.m10 = y * transform->m10;
  out.m11 = y * transform->m11;
  out.m12 = y * transform->m12 + projection->m12;

  return out;
}

SDL_GP_INLINE SDL_GPVec2
_SDL_GPMat3MulVec2(SDL_GPMat2x3 *m, const SDL_GPVec2 *v)
{
  return (SDL_GPVec2){ m->m00 * v->x + m->m01 * v->y + m->m02,
                       m->m10 * v->x + m->m11 * v->y + m->m12 };
}

SDL_GP_INLINE void
_SDL_GPTransform(SDL_GPMat2x3 *matrix,
                 SDL_GPVec2 *dst,
                 const SDL_GPVec2 *src,
                 Uint32 count)
{
  for (Uint32 i = 0; i < count; ++i) {
    dst[i] = _SDL_GPMat3MulVec2(matrix, &src[i]);
  }
}

void
SDL_GPUpdateSwapchainTexture(SDL_GPUTexture *swapchain_texture)
{
  SDL_assert(swapchain_texture);
  _swapchain_texture = swapchain_texture;
}

void
SDL_GPUpdateCommandBuffer(SDL_GPUCommandBuffer *cmd_buffer)
{
  SDL_assert(cmd_buffer);
  _cmd_buffer = cmd_buffer;
}

void
SDL_GPSetup(SDL_GPDesc *desc)
{
  SDL_assert(_gp_initialized == 0);
  SDL_assert(desc);

  _last_error = SDL_GP_ERROR_NONE;

  _gp_initialized = _SDL_GP_INIT_COOKIE;

  _gp.desc.max_vertices
      = SDL_GP_DEFAULT(desc->max_vertices, SDL_GP_VERTICES_MAX);
  _gp.desc.max_commands
      = SDL_GP_DEFAULT(desc->max_commands, SDL_GP_COMMANDS_MAX);

  _gp.desc.window     = desc->window;
  _gp.desc.gpu_device = desc->gpu_device;

  // Setup resources management for shaders, pipelines and images

  _SDL_GPShaderSetup(_gp.desc.gpu_device);
  _SDL_GPPipelineSetup(_gp.desc.gpu_device, _gp.desc.window);
  _SDL_GPImageSetup(_gp.desc.gpu_device, _gp.desc.window);

  // Create a white texture

  SDL_GPUTextureFormat texture_format
      = SDL_GetGPUSwapchainTextureFormat(_image_gpu_device, _image_window);

  SDL_PixelFormat pixel_format
      = SDL_GetPixelFormatFromGPUTextureFormat(texture_format);

  const SDL_PixelFormatDetails *format_details
      = SDL_GetPixelFormatDetails(pixel_format);

  Uint32 white = SDL_MapRGBA(format_details, NULL, 255, 255, 255, 255);

  SDL_Surface *white_surface
      = SDL_CreateSurfaceFrom(2,
                              2,
                              pixel_format,
                              (Uint32[]){ white, white, white, white },
                              format_details->bytes_per_pixel * 2);

  if (white_surface == NULL) {
    SDL_GPShutdown();
    _SDL_GPSetError(SDL_GP_ERROR_CREATE_WHITE_TEXTURE_FAILED);
    return;
  }

  _gp.white_image = SDL_GPCreateImage(white_surface);

  SDL_DestroySurface(white_surface);

  // Create a GPU transfer buffer for vertex data

  SDL_GPUTransferBufferCreateInfo vertex_transfer_buffer_create_info = {
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size  = (Uint32)(_gp.desc.max_vertices * sizeof(SDL_GPVertex)),
  };

  _gp.vertex_transfer_buffer = SDL_CreateGPUTransferBuffer(
      desc->gpu_device, &vertex_transfer_buffer_create_info);

  if (_gp.vertex_transfer_buffer == NULL) {
    _SDL_GPSetError(SDL_GP_ERROR_CREATE_TRANSFER_BUFFER_FAILED);
    return;
  }

  // Create a GPU buffer for vertex data

  SDL_GPUBufferCreateInfo vertex_data_buffer_create_info = {
    .size  = (Uint32)(_gp.desc.max_vertices * sizeof(SDL_GPVertex)),
    .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
  };

  _gp.vertex_data_buffer
      = SDL_CreateGPUBuffer(desc->gpu_device, &vertex_data_buffer_create_info);

  if (_gp.vertex_data_buffer == NULL) {
    SDL_ReleaseGPUTransferBuffer(desc->gpu_device, _gp.vertex_transfer_buffer);
    _SDL_GPSetError(SDL_GP_ERROR_CREATE_VERTEX_BUFFER_FAILED);
    return;
  }

  // Create nearest sampler

  SDL_GPUSamplerCreateInfo nearest_sampler_info = {
    .min_filter     = SDL_GPU_FILTER_NEAREST,
    .mag_filter     = SDL_GPU_FILTER_NEAREST,
    .mipmap_mode    = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
  };

  _gp.nearest_samplers
      = SDL_CreateGPUSampler(desc->gpu_device, &nearest_sampler_info);

  // Create common shader

  _gp.shader = _SDL_GPCreateCommonShader(desc->gpu_device);

  // Create common pipelines

  bool is_ok = true;
  is_ok &= _SDL_GP_FindOrCreatePipeline(SDL_GP_PRIMITIVE_POINTS,
                                        SDL_GP_BLENDMODE_NONE)
               .id
           != SDL_GP_INVALID_ID;
  is_ok &= _SDL_GP_FindOrCreatePipeline(SDL_GP_PRIMITIVE_POINTS,
                                        SDL_GP_BLENDMODE_BLEND)
               .id
           != SDL_GP_INVALID_ID;
  is_ok &= _SDL_GP_FindOrCreatePipeline(SDL_GP_PRIMITIVE_LINES,
                                        SDL_GP_BLENDMODE_NONE)
               .id
           != SDL_GP_INVALID_ID;
  is_ok &= _SDL_GP_FindOrCreatePipeline(SDL_GP_PRIMITIVE_LINES,
                                        SDL_GP_BLENDMODE_BLEND)
               .id
           != SDL_GP_INVALID_ID;
  is_ok &= _SDL_GP_FindOrCreatePipeline(SDL_GP_PRIMITIVE_LINE_STRIP,
                                        SDL_GP_BLENDMODE_NONE)
               .id
           != SDL_GP_INVALID_ID;
  is_ok &= _SDL_GP_FindOrCreatePipeline(SDL_GP_PRIMITIVE_LINE_STRIP,
                                        SDL_GP_BLENDMODE_BLEND)
               .id
           != SDL_GP_INVALID_ID;
  is_ok &= _SDL_GP_FindOrCreatePipeline(SDL_GP_PRIMITIVE_TRIANGLES,
                                        SDL_GP_BLENDMODE_NONE)
               .id
           != SDL_GP_INVALID_ID;
  is_ok &= _SDL_GP_FindOrCreatePipeline(SDL_GP_PRIMITIVE_TRIANGLES,
                                        SDL_GP_BLENDMODE_BLEND)
               .id
           != SDL_GP_INVALID_ID;

  if (!is_ok) {
    _SDL_GPSetError(SDL_GP_ERROR_CREATE_COMMON_PIPELINE_FAILED);
    SDL_GPShutdown();
    return;
  }
}

void
SDL_GPShutdown()
{
  if (_gp_initialized != _SDL_GP_INIT_COOKIE) {
    return;
  }

  // Destroy common pipelines

  for (int i = 0; i < SDL_GP_PRIMITIVE_SIZE * SDL_GP_BLENDMODE_SIZE; ++i) {
    if (_gp.pipelines[i].id != SDL_GP_INVALID_ID) {
      SDL_GPPipelineDestroy(_gp.pipelines[i]);
      _gp.pipelines[i] = (SDL_GPPipeline){ .id = SDL_GP_INVALID_ID };
    }
  }

  // Destroy common shader

  if (_gp.shader.id != SDL_GP_INVALID_ID) {
    SDL_GPDestroyShader(_gp.shader);
    _gp.shader = (SDL_GPShader){ .id = SDL_GP_INVALID_ID };
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
    _gp.white_image = (SDL_GPImage){ .id = SDL_GP_INVALID_ID };
  }

  // Shutdown resources management for shaders, pipelines and images
  _SDL_GPImageShutdown();
  _SDL_GPPipelineShutdown();
  _SDL_GPShaderShutdown();

  SDL_memset(&_gp, 0, sizeof(_SDL_GP));
}

void
SDL_GPBegin(int width, int height)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);

  _gp.states[_gp.current_state++] = _gp.state;

  _gp.state.projection = _SDL_GPDefaultProjection(width, height);
  _gp.state.transform  = SDL_GPCreateMat2x3Identity();
  _gp.state.mvp        = _gp.state.projection;

  _gp.state.texture.count       = 1;
  _gp.state.texture.images[0]   = _gp.white_image;
  _gp.state.texture.samplers[0] = _gp.nearest_samplers;

  SDL_GPImage image = { .id = SDL_GP_INVALID_ID };
  for (int i = 1; i < SDL_GP_TEXTURE_SLOTS_MAX; ++i) {
    _gp.state.texture.images[i]   = image;
    _gp.state.texture.samplers[i] = _gp.nearest_samplers;
  }

  SDL_memset(&_gp.state.uniform, 0, sizeof(SDL_GPUniform));
  _gp.state.uniform = (SDL_GPUniform){ .vs_size = 0, .fs_size = 0 };

  _gp.state.blend_mode = SDL_GP_BLENDMODE_NONE;

  _gp.state.frame_size.w = width;
  _gp.state.frame_size.h = height;
  _gp.state.viewport = (SDL_GPIRect){ .x = 0, .y = 0, .w = width, .h = height };
  _gp.state.scissor  = (SDL_GPIRect){ .x = 0, .y = 0, .w = -1, .h = -1 };
  _gp.state.color    = (SDL_Color){ .r = 255, .g = 255, .b = 255, .a = 255 };

  _gp.state.thickness    = SDL_max(1.0f / width, 1.0f / height);
  _gp.state.base_vertex  = _gp.current_vertex;
  _gp.state.base_uniform = _gp.current_uniform;
  _gp.state.base_command = _gp.current_command;
}

void
SDL_GPFlush(SDL_GPUTexture *swapchain_texture)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  SDL_assert(_cmd_buffer);
  SDL_assert(_swapchain_texture);

  Uint32 end_command = _gp.current_command;
  Uint32 end_vertex  = _gp.current_vertex;

  Uint32 vertices_count
      = end_vertex - _gp.state.base_vertex; // Number of vertices to draw

  // Rewind Index
  _gp.current_command = _gp.state.base_command;
  _gp.current_uniform = _gp.state.base_uniform;
  _gp.current_vertex  = _gp.state.base_vertex;

  // Error, Nothing to draw
  if (_last_error != SDL_GP_ERROR_NONE) {
    return;
  }

  // Nothing to draw
  if (end_command <= _gp.state.base_command) {
    return;
  }

  // Upload vertex data to GPU

  SDL_GPVertex *vertex_data = (SDL_GPVertex *)SDL_MapGPUTransferBuffer(
      _gp.desc.gpu_device, _gp.vertex_transfer_buffer, true);

  if (vertex_data == NULL) {
    _SDL_GPSetError(SDL_GP_ERROR_FLUSH_FAILED);
    return;
  }

  SDL_memcpy(vertex_data,
             &_gp.vertices[_gp.state.base_vertex],
             vertices_count * sizeof(SDL_GPVertex));

  SDL_UnmapGPUTransferBuffer(_gp.desc.gpu_device, _gp.vertex_transfer_buffer);

  // Copy pass

  SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(_cmd_buffer);

  SDL_GPUTransferBufferLocation vertex_transfer_location
      = { .transfer_buffer = _gp.vertex_transfer_buffer, .offset = 0 };

  SDL_GPUBufferRegion vertex_buffer_region
      = { .buffer = _gp.vertex_data_buffer,
          .offset = (Uint32)(_gp.state.base_vertex * sizeof(SDL_GPVertex)),
          .size   = (Uint32)((vertices_count) * sizeof(SDL_GPVertex)) };

  SDL_UploadToGPUBuffer(
      copy_pass, &vertex_transfer_location, &vertex_buffer_region, true);

  SDL_EndGPUCopyPass(copy_pass);

  // Render pass

  SDL_GPUColorTargetInfo color_target_info = {
    .texture     = swapchain_texture, // _gp.desc.target_texture,
    .clear_color = { 0, 0, 0, 1 },
    .load_op     = SDL_GPU_LOADOP_CLEAR,
    .store_op    = SDL_GPU_STOREOP_STORE,
    .cycle       = false,
  };

  SDL_GPURenderPass *render_pass
      = SDL_BeginGPURenderPass(_cmd_buffer, &color_target_info, 1, NULL);

  Uint32 cur_pipeline_id   = SDL_GP_IMPOSSIBLE_ID;
  Uint32 cur_uniform_index = SDL_GP_IMPOSSIBLE_ID;
  Uint32 cur_image_ids[SDL_GP_TEXTURE_SLOTS_MAX];
  for (int i = 0; i < SDL_GP_TEXTURE_SLOTS_MAX; ++i) {
    cur_image_ids[i] = SDL_GP_IMPOSSIBLE_ID;
  }

  // Flush commands

  for (Uint32 i = _gp.state.base_command; i < end_command; ++i) {
    _SDL_GPCommand *cmd = &_gp.commands[i];

    SDL_Rect scissor         = { 0 };
    SDL_GPUViewport viewport = { 0 };

    switch (cmd->cmd) {
    case _SDL_GP_COMMAND_DRAW: {
      if (vertices_count == 0) {
        break;
      }

      bool rebind_uniforms = false;
      bool rebind_texture  = false;

      // Check if pipeline needs to be changed
      if (cmd->args.draw.pipeline.id != cur_pipeline_id) {
        cur_pipeline_id = cmd->args.draw.pipeline.id;

        // Bind pipeline
        SDL_BindGPUGraphicsPipeline(
            render_pass, SDL_GPGetGPUPipeline(cmd->args.draw.pipeline));

        // When pipeline changes we need to rebind uniforms and textures
        rebind_uniforms = true;
        rebind_texture  = true;
      }

      // Check if texture needs to be changed
      SDL_GPUTextureSamplerBinding image_bindings[SDL_GP_TEXTURE_SLOTS_MAX];

      for (int j = 0; j < SDL_GP_TEXTURE_SLOTS_MAX; ++j) {
        Uint32 image_id = SDL_GP_INVALID_ID;

        if (j < cmd->args.draw.texture.count) {
          image_id = cmd->args.draw.texture.images[j].id;
        }

        if (image_id != cur_image_ids[j]) {
          cur_image_ids[j] = image_id;

          if (image_id != SDL_GP_INVALID_ID) {
            image_bindings[j] = (SDL_GPUTextureSamplerBinding){
              .texture
              = SDL_GPGetImageGPUTexture(cmd->args.draw.texture.images[j]),
              .sampler = cmd->args.draw.texture.samplers[j]
            };
          } else {
            image_bindings[j] = (SDL_GPUTextureSamplerBinding){
              .texture = SDL_GPGetImageGPUTexture(_gp.white_image),
              .sampler = _gp.nearest_samplers,
            };
          }

          // When image changes we need to rebind textures
          rebind_texture = true;
        }
      }

      // Rebind textures if needed
      if (rebind_texture) {
        SDL_BindGPUFragmentSamplers(
            render_pass, 0, image_bindings, SDL_GP_TEXTURE_SLOTS_MAX);
      }

      // Rebind uniforms if needed
      if (rebind_uniforms && cur_uniform_index != SDL_GP_IMPOSSIBLE_ID) {
        SDL_GPUniform *uniform = &_gp.uniforms[cmd->args.draw.uniform_index];

        if (uniform->vs_size > 0) {
          SDL_PushGPUVertexUniformData(_cmd_buffer,
                                       SDL_GP_UNIFORM_SLOT_VS,
                                       &uniform->data.bytes[0],
                                       uniform->vs_size);
        }
        if (uniform->fs_size > 0) {
          SDL_PushGPUFragmentUniformData(_cmd_buffer,
                                         SDL_GP_UNIFORM_SLOT_FS,
                                         &uniform->data.bytes[0],
                                         uniform->fs_size);
        }
      }

      SDL_GPUBufferBinding vertex_buffer_binding
          = { .buffer = _gp.vertex_data_buffer,
              .offset
              = (Uint32)(cmd->args.draw.vertex_index * sizeof(SDL_GPVertex)) };

      // In every case we need to bind vertex buffers
      SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);

      SDL_DrawGPUPrimitives(
          render_pass, cmd->args.draw.vertices_count, 1, 0, 0);
      break;
    }
    case _SDL_GP_COMMAND_VIEWPORT:
      viewport = (SDL_GPUViewport){
        .x = (float)cmd->args.viewport.x,
        .y = (float)cmd->args.viewport.y,
        .w = (float)cmd->args.viewport.w,
        .h = (float)cmd->args.viewport.h,
      };
      SDL_SetGPUViewport(render_pass, &viewport);
      break;
    case _SDL_GP_COMMAND_SCISSOR:
      scissor = (SDL_Rect){
        .x = (int)cmd->args.scissor.x,
        .y = (int)cmd->args.scissor.y,
        .w = (int)cmd->args.scissor.w,
        .h = (int)cmd->args.scissor.h,
      };
      SDL_SetGPUScissor(render_pass, &scissor);
      break;
    default:
      break;
    }
  }

  // End render pass

  SDL_EndGPURenderPass(render_pass);
}

void
SDL_GPEnd()
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);

  _gp.state = _gp.states[--_gp.current_state];
}

void
SDL_GPSetProjection(float left, float right, float bottom, float top)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  float width  = right - left;
  float height = top - bottom;

  _gp.state.projection = SDL_GPCreateMat2x3(2.0f / width,
                                            0.0f,
                                            -(right + left) / width,
                                            0.0f,
                                            2.0f / height,
                                            -(top + bottom) / height);

  _gp.state.mvp = _SDL_GPMulProjectionTransform(&_gp.state.projection,
                                                &_gp.state.transform);
}

void
SDL_GPResetProjection()
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  _gp.state.projection = _SDL_GPDefaultProjection((int)_gp.state.viewport.w,
                                                  (int)_gp.state.viewport.h);

  _gp.state.mvp = _SDL_GPMulProjectionTransform(&_gp.state.projection,
                                                &_gp.state.transform);
}

void
SDL_GPPushTransform()
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);
  SDL_assert(_gp.current_transform < SDL_GP_TRANSFORMS_MAX);

  _gp.transforms[_gp.current_transform++] = _gp.state.transform;
}

void
SDL_GPPopTransform()
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);
  SDL_assert(_gp.current_transform > 0);

  _gp.state.transform = _gp.transforms[--_gp.current_transform];
  _gp.state.mvp       = _SDL_GPMulProjectionTransform(&_gp.state.projection,
                                                      &_gp.state.transform);
}

void
SDL_GPResetTransform()
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  _gp.state.transform = SDL_GPCreateMat2x3Identity();
  _gp.state.mvp       = _SDL_GPMulProjectionTransform(&_gp.state.projection,
                                                      &_gp.state.transform);
}

void
SDL_GPTranslate(float x, float y)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  // multiply by translate matrix:
  // 1.0f, 0.0f, tx,
  // 0.0f, 1.0f, ty,

  _gp.state.transform.m02
      += x * _gp.state.transform.m00 + y * _gp.state.transform.m01;
  _gp.state.transform.m12
      += x * _gp.state.transform.m10 + y * _gp.state.transform.m11;

  _gp.state.mvp = _SDL_GPMulProjectionTransform(&_gp.state.projection,
                                                &_gp.state.transform);
}

void
SDL_GPRotate(float angle)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  float c = SDL_cos(angle);
  float s = SDL_sin(angle);

  // Multiply by rotation matrix:
  //   c,   -s, 0.0f,
  //   s,    c, 0.0f,

  SDL_GPMat2x3 rotation = SDL_GPCreateMat2x3(
      c * _gp.state.transform.m00 + s * _gp.state.transform.m01,
      -s * _gp.state.transform.m00 + c * _gp.state.transform.m01,
      _gp.state.transform.m02,
      c * _gp.state.transform.m10 + s * _gp.state.transform.m11,
      -s * _gp.state.transform.m10 + c * _gp.state.transform.m11,
      _gp.state.transform.m12);

  _gp.state.transform = rotation;
  _gp.state.mvp       = _SDL_GPMulProjectionTransform(&_gp.state.projection,
                                                      &_gp.state.transform);
}

void
SDL_GPRotateAt(float angle, float ax, float ay)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  SDL_GPTranslate(ax, ay);
  SDL_GPRotate(angle);
  SDL_GPTranslate(-ax, -ay);
}

void
SDL_GPScale(float sx, float sy)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  // Multiply by scale matrix:
  //   sx, 0.0f, 0.0f,
  // 0.0f,   sy, 0.0f,

  _gp.state.transform.m00 *= sx;
  _gp.state.transform.m01 *= sy;
  _gp.state.transform.m10 *= sx;
  _gp.state.transform.m11 *= sy;

  _gp.state.mvp = _SDL_GPMulProjectionTransform(&_gp.state.projection,
                                                &_gp.state.transform);
}

void
SDL_GPScaleAt(float sx, float sy, float ax, float ay)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  SDL_GPTranslate(ax, ay);
  SDL_GPScale(sx, sy);
  SDL_GPTranslate(-ax, -ay);
}

void
SDL_GPSetPipeline(SDL_GPPipeline pipeline)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);

  _gp.state.pipeline = pipeline;

  // Reset uniforms when pipeline changes
  SDL_memset(&_gp.state.uniform, 0, sizeof(SDL_GPUniform));
}

void
SDL_GPResetPipeline()
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);

  SDL_GPPipeline pipeline = { .id = SDL_GP_INVALID_ID };

  SDL_GPSetPipeline(pipeline);
}

void
SDL_GPSetUniform(const void *vs_data,
                 size_t vs_size,
                 const void *fs_data,
                 size_t fs_size)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.state.pipeline.id != SDL_GP_INVALID_ID);

  size_t size = vs_size + fs_size;

  SDL_assert(size <= SDL_GP_UNIFORM_FLOATS_MAX * sizeof(float));

  if (vs_size > 0) {
    SDL_assert(vs_data != NULL);
    SDL_memcpy(&_gp.state.uniform.data.bytes[0], vs_data, vs_size);
  }
  if (fs_size > 0) {
    SDL_assert(fs_data != NULL);
    SDL_memcpy(&_gp.state.uniform.data.bytes[vs_size], fs_data, fs_size);
  }

  size_t old_size = _gp.state.uniform.vs_size + _gp.state.uniform.fs_size;

  if (size != old_size) {
    // Zero out the rest of the uniform data
    SDL_memset((Uint8 *)(&_gp.state.uniform) + size, 0, old_size - size);
  }

  _gp.state.uniform.vs_size = (Uint16)vs_size;
  _gp.state.uniform.fs_size = (Uint16)fs_size;
}

void
SDL_GPResetUniform()
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.state.pipeline.id != SDL_GP_INVALID_ID);

  SDL_GPSetUniform(NULL, 0, NULL, 0);
}

void
SDL_GPSetBlendMode(SDL_GPBlendMode blend_mode)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  _gp.state.blend_mode = blend_mode;
}

void
SDL_GPResetBlendMode()
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  _gp.state.blend_mode = SDL_GP_BLENDMODE_NONE;
}

void
SDL_GPSetColor(SDL_Color color)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  _gp.state.color = color;
}

SDL_Color
SDL_GPGetColor()
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  return _gp.state.color;
}

void
SDL_GPResetColor()
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  _gp.state.color = (SDL_Color){ 255, 255, 255, 255 };
}

void
SDL_GPSetImage(int channel, SDL_GPImage image)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);
  SDL_assert(channel >= 0 && channel < SDL_GP_TEXTURE_SLOTS_MAX);

  if (_gp.state.texture.images[channel].id == image.id) {
    return;
  }

  _gp.state.texture.images[channel] = image;

  // Recalculate texture count
  int texture_count = _gp.state.texture.count;
  for (int i = SDL_max(channel, texture_count - 1); i >= 0; --i) {
    if (_gp.state.texture.images[i].id != SDL_GP_INVALID_ID) {
      texture_count = i + 1;
      break;
    }
  }

  _gp.state.texture.count = texture_count;
}

void
SDL_GPResetImage(int channel)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);
  SDL_assert(channel >= 0 && channel < SDL_GP_TEXTURE_SLOTS_MAX);

  SDL_GPSetImage(channel, _gp.white_image);
}

void
SDL_GPUnsetImage(int channel)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);
  SDL_assert(channel >= 0 && channel < SDL_GP_TEXTURE_SLOTS_MAX);

  SDL_GPSetImage(channel, (SDL_GPImage){ .id = SDL_GP_INVALID_ID });
}

void
SDL_GPSetSampler(int channel, SDL_GPUSampler *sampler)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);
  SDL_assert(channel >= 0 && channel < SDL_GP_TEXTURE_SLOTS_MAX);

  _gp.state.texture.samplers[channel] = sampler;
}

void
SDL_GPUnsetSampler(int channel)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);
  SDL_assert(channel >= 0 && channel < SDL_GP_TEXTURE_SLOTS_MAX);

  _gp.state.texture.samplers[channel] = NULL;
}

void
SDL_GPResetSampler(int channel)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);
  SDL_assert(channel >= 0 && channel < SDL_GP_TEXTURE_SLOTS_MAX);

  _gp.state.texture.samplers[channel] = _gp.nearest_samplers;
}

// Set the viewport for subsequent draw calls.
void
SDL_GPViewport(int x, int y, int w, int h)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  // If no change in viewport, skip
  if (_gp.state.viewport.x == x && _gp.state.viewport.y == y
      && _gp.state.viewport.w == w && _gp.state.viewport.h == h) {
    return;
  }

  // Try to reuse previous command
  _SDL_GPCommand *cmd = _SDL_GPPrevCommand(1);
  if (cmd && cmd->cmd != _SDL_GP_COMMAND_VIEWPORT) {
    cmd = _SDL_GPNextCommand();
  }
  if (!cmd) {
    return;
  }

  SDL_memset(&cmd->args.viewport, 0, sizeof(SDL_GPIRect));

  SDL_GPIRect viewport = {
    .x = x,
    .y = y,
    .w = w,
    .h = h,
  };

  cmd->cmd           = _SDL_GP_COMMAND_VIEWPORT;
  cmd->args.viewport = viewport;

  // When viewport changes, scissor needs to be updated to keep the same region
  if (!(_gp.state.scissor.w < 0 && _gp.state.scissor.h < 0)) {
    _gp.state.scissor.x += x - _gp.state.viewport.x;
    _gp.state.scissor.y += y - _gp.state.viewport.y;
  }

  _gp.state.viewport   = viewport;
  _gp.state.thickness  = SDL_max(1.0f / w, 1.0f / h);
  _gp.state.projection = _SDL_GPDefaultProjection(w, h);
  _gp.state.mvp        = _SDL_GPMulProjectionTransform(&_gp.state.projection,
                                                       &_gp.state.transform);
}

void
SDL_GPResetViewport(void)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  SDL_GPViewport(0, 0, _gp.state.frame_size.w, _gp.state.frame_size.h);
}

// Set the scissor for subsequent draw calls.
void
SDL_GPScissor(int x, int y, int w, int h)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  // Skip if scissor is the same
  if (_gp.state.scissor.x == x && _gp.state.scissor.y == y
      && _gp.state.scissor.w == w && _gp.state.scissor.h == h) {
    return;
  }

  // Try to reuse previous command
  _SDL_GPCommand *cmd = _SDL_GPPrevCommand(1);
  if (cmd && cmd->cmd != _SDL_GP_COMMAND_SCISSOR) {
    cmd = _SDL_GPNextCommand();
  }
  if (!cmd) {
    return;
  }

  // Coordinates scissor relative to viewport
  SDL_GPIRect viewport_scissor = (SDL_GPIRect){
    _gp.state.viewport.x + x, _gp.state.viewport.y + y, w, h
  };

  // Reset scissor
  if (w < 0 && h < 0) {
    viewport_scissor.x = 0;
    viewport_scissor.y = 0;
    viewport_scissor.w = _gp.state.frame_size.w;
    viewport_scissor.h = _gp.state.frame_size.h;
  }

  SDL_memset(&cmd->args.scissor, 0, sizeof(SDL_GPIRect));

  cmd->cmd          = _SDL_GP_COMMAND_SCISSOR;
  cmd->args.scissor = viewport_scissor;

  _gp.state.scissor = (SDL_GPIRect){ .x = x, .y = y, .w = w, .h = h };
}

void
SDL_GPResetScissor()
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  _gp.state.scissor = (SDL_GPIRect){ .x = 0, .y = 0, .w = -1, .h = -1 };
}

void
SDL_GPResetState()
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  SDL_GPResetViewport();
  SDL_GPResetScissor();
  SDL_GPResetProjection();
  SDL_GPResetTransform();
  SDL_GPResetBlendMode();
  SDL_GPResetColor();
  SDL_GPResetUniform();
  SDL_GPResetPipeline();
}

static bool
_SDL_GPMergeDrawCommands(SDL_GPUGraphicsPipeline *pipeline,
                         SDL_GPTextureUniform *texture,
                         SDL_GPUniform *uniform,
                         Uint32 vertex_index,
                         Uint32 vertices_count,
                         SDL_GPUPrimitiveType primitive_type)
{
#if BXR_PAINTER_OPTIMIZE_DEPTH > 0
  _bxr_command_t *prev_cmd = NULL;
  _bxr_command_t *inter_cmds[BXR_PAINTER_OPTIMIZE_DEPTH];
  Uint32 inter_count = 0;

  // Find a command that is mergeable
  Uint32 lookup_depht = BXR_PAINTER_OPTIMIZE_DEPTH;
  for (Uint32 depth = 1; depth <= lookup_depht; ++depth) {
    _bxr_command_t *cmd = _bxr_prev_command(depth + 1);

    // Stop on nonexistent command
    if (!cmd) {
      break;
    }

    // command was optimized awaw, continue looking
    if (cmd->cmd == BXR_COMMAND_NONE) {
      lookup_depht++;
      continue;
    }

    // Stop on scissor or viewport
    if (cmd->cmd != BXR_COMMAND_DRAW) {
      break;
    }

    // Only command with the same pipeline, bindings and (TODO uniforms) can be
    // merged
    // TODO
  }

#endif
  return false;
}

static void
_SDL_GPQueueDraw(SDL_GPPipeline pipeline,
                 _SDL_GPRegion region,
                 Uint32 vertex_index,
                 Uint32 vertices_count,
                 SDL_GPPrimitiveType primitive_type)
{
  /*
  if (region.x0 > 1.0f || region.y0 > 1.0f || region.x1 < 0.0f
      || region.y1 < 0.0f) {
    _cur_vertex -= vertices_count; // rollback allocated vertices
    return;
  }
  */

  /* TODO
  // Try to merge with previous draw command
  if (primitive_type != SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP
      && primitive_type != SDL_GPU_PRIMITIVETYPE_LINESTRIP
      && _bxr_painter_merge_draw_commands(pipeline,
                                         _state.image,
                                         vertex_index,
                                         vertices_count,
                                         primitive_type)) {
    return;
  }
  */

  SDL_GPUniform *uniform = NULL;
  if (_gp.state.pipeline.id != SDL_GP_INVALID_ID) {
    pipeline = _gp.state.pipeline;
    uniform  = &_gp.state.uniform;
  }

  Uint32 uniform_index = SDL_GP_IMPOSSIBLE_ID;

  if (uniform) {
    SDL_GPUniform *prev_uniform = _SDL_GPPrevUniform();

    bool reuse_uniform
        = prev_uniform
          && (SDL_memcmp(prev_uniform, uniform, sizeof(SDL_GPUniform)) == 0);

    if (!reuse_uniform) {
      SDL_GPUniform *next_uniform = _SDL_GPNextUniform();
      if (!next_uniform) {
        _gp.current_vertex -= vertices_count; // rollback allocated vertices
        return;
      }
      *next_uniform = _gp.state.uniform;
    }

    uniform_index
        = _gp.current_uniform - 1; // - 1 since _bxr_painter_next_uniform
                                   // already incremented the index
  }

  // New draw command
  _SDL_GPCommand *cmd = _SDL_GPNextCommand();

  if (!cmd) {
    _gp.current_vertex -= vertices_count; // rollback allocated vertices
    return;
  }

  cmd->cmd                = _SDL_GP_COMMAND_DRAW;
  cmd->args.draw.pipeline = pipeline;
  cmd->args.draw.texture  = _gp.state.texture;
  // TODO region for optimization
  cmd->args.draw.uniform_index  = uniform_index;
  cmd->args.draw.vertex_index   = vertex_index;
  cmd->args.draw.vertices_count = vertices_count;
}

static void
_SDL_GPDrawSolid(SDL_GPPrimitiveType primitive_type,
                 const SDL_GPVec2 *vertices,
                 Uint32 vertices_count)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  if (vertices_count == 0) {
    return;
  }

  // Setup vertices
  Uint32 vertex_index = _gp.current_vertex;
  SDL_GPVertex *v     = _SDL_GPNextVertices(vertices_count);
  if (!v) {
    return;
  }

  float thickness      = (primitive_type == SDL_GP_PRIMITIVE_POINTS
                          || primitive_type == SDL_GP_PRIMITIVE_LINES
                          || primitive_type == SDL_GP_PRIMITIVE_LINE_STRIP)
                             ? _gp.state.thickness
                             : 1.0f;
  SDL_Color color      = _gp.state.color;
  SDL_GPMat2x3 mvp     = _gp.state.mvp;
  _SDL_GPRegion region = { FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX };

  for (Uint32 i = 0; i < vertices_count; ++i) {
    SDL_GPVec2 p = _SDL_GPMat3MulVec2(&mvp, &vertices[i]);

    region.x1 = SDL_min(region.x1, p.x - thickness);
    region.y1 = SDL_min(region.y1, p.y - thickness);
    region.x2 = SDL_max(region.x2, p.x + thickness);
    region.y2 = SDL_max(region.y2, p.y + thickness);

    v[i].position = p;
    v[i].texcoord = (SDL_GPVec2){ 0.0f, 0.0f };
    v[i].color    = color;
  }

  SDL_GPPipeline pipeline
      = _SDL_GP_FindOrCreatePipeline(primitive_type, _gp.state.blend_mode);

  // Queue draw
  _SDL_GPQueueDraw(
      pipeline, region, vertex_index, vertices_count, primitive_type);
}

void
SDL_GPClear()
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  // Setup vertices
  Uint32 vertices_count = 6;
  Uint32 vertex_index   = _gp.current_vertex;

  SDL_GPVertex *v = _SDL_GPNextVertices(vertices_count);
  if (!v) {
    return;
  }

  // Compute vertices
  const SDL_GPVec2 quad[4] = {
    { -1.0f, -1.0f }, // bottom-left
    { 1.0f, -1.0f },  // bottom-right
    { 1.0f, 1.0f },   // top-right
    { -1.0f, 1.0f }   // top-left
  };

  const SDL_GPVec2 texcoord = { 0.0f, 0.0f };
  SDL_Color color           = _gp.state.color;

  v[0] = (SDL_GPVertex){ .position = quad[0],
                         .texcoord = texcoord,
                         .color    = color };
  v[1] = (SDL_GPVertex){ .position = quad[1],
                         .texcoord = texcoord,
                         .color    = color };
  v[2] = (SDL_GPVertex){ .position = quad[2],
                         .texcoord = texcoord,
                         .color    = color };
  v[3] = (SDL_GPVertex){ .position = quad[2],
                         .texcoord = texcoord,
                         .color    = color };
  v[4] = (SDL_GPVertex){ .position = quad[3],
                         .texcoord = texcoord,
                         .color    = color };
  v[5] = (SDL_GPVertex){ .position = quad[0],
                         .texcoord = texcoord,
                         .color    = color };

  _SDL_GPRegion region = { -1.0f, -1.0f, 1.0f, 1.0f };

  SDL_GPPipeline pipeline = _SDL_GP_FindOrCreatePipeline(
      SDL_GP_PRIMITIVE_TRIANGLES, _gp.state.blend_mode);

  _SDL_GPQueueDraw(pipeline,
                   region,
                   vertex_index,
                   vertices_count,
                   SDL_GP_PRIMITIVE_TRIANGLES);
}

void
SDL_GPDraw(SDL_GPPrimitiveType primitive_type,
           const SDL_GPVertex *vertices,
           Uint32 vertices_count)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  if (vertices_count == 0) {
    return;
  }

  // Setup vertices
  Uint32 vertex_index = _gp.current_vertex;
  SDL_GPVertex *v     = _SDL_GPNextVertices(vertices_count);
  if (!v) {
    return;
  }

  float thickness      = (primitive_type == SDL_GP_PRIMITIVE_POINTS
                          || primitive_type == SDL_GP_PRIMITIVE_LINES
                          || primitive_type == SDL_GP_PRIMITIVE_LINE_STRIP)
                             ? _gp.state.thickness
                             : 1.0f;
  SDL_GPMat2x3 mvp     = _gp.state.mvp;
  _SDL_GPRegion region = { FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX };

  for (Uint32 i = 0; i < vertices_count; ++i) {
    SDL_GPVec2 p = _SDL_GPMat3MulVec2(&mvp, &vertices[i].position);

    region.x1 = SDL_min(region.x1, p.x - thickness);
    region.y1 = SDL_min(region.y1, p.y - thickness);
    region.x2 = SDL_max(region.x2, p.x + thickness);
    region.y2 = SDL_max(region.y2, p.y + thickness);

    v[i].position = p;
    v[i].texcoord = vertices[i].texcoord;
    v[i].color    = vertices[i].color;
  }

  SDL_GPPipeline pipeline
      = _SDL_GP_FindOrCreatePipeline(primitive_type, _gp.state.blend_mode);

  // Queue draw
  _SDL_GPQueueDraw(
      pipeline, region, vertex_index, vertices_count, primitive_type);
}

void
SDL_GPDrawPoints(const SDL_GPVec2 *points, Uint32 count)
{
  _SDL_GPDrawSolid(SDL_GP_PRIMITIVE_POINTS, points, count);
}

void
SDL_GPDrawPoint(SDL_GPVec2 point)
{
  SDL_GPDrawPoints(&point, 1);
}

void
SDL_GPDrawLines(const SDL_GPLine *lines, Uint32 count)
{
  _SDL_GPDrawSolid(
      SDL_GP_PRIMITIVE_LINES, (const SDL_GPVec2 *)lines, count * 2);
}

void
SDL_GPDrawLine(SDL_GPLine line)
{
  SDL_GPDrawLines(&line, 1);
}

void
SDL_GPDrawLinesStrip(const SDL_GPVec2 *points, Uint32 count)
{
  _SDL_GPDrawSolid(SDL_GP_PRIMITIVE_LINE_STRIP, points, count);
}

void
SDL_GPDrawFilledTriangles(const SDL_GPTriangle *triangles, Uint32 count)
{
  _SDL_GPDrawSolid(
      SDL_GP_PRIMITIVE_TRIANGLES, (const SDL_GPVec2 *)triangles, count * 3);
}

void
SDL_GPDrawFilledTriangle(SDL_GPTriangle triangle)
{
  SDL_GPDrawFilledTriangles(&triangle, 1);
}

void
SDL_GPDrawFilledTrianglesStrip(const SDL_GPVec2 *points, Uint32 count)
{
  _SDL_GPDrawSolid(SDL_GP_PRIMITIVE_TRIANGLE_STRIP, points, count);
}

void
SDL_GPDrawFilledRects(const SDL_GPRect *rects, Uint32 count)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  if (count == 0) {
    return;
  }

  // Setup vertices
  Uint32 total_vertices = count * 6; // 2 triangles per rect, 3 vertices each
  Uint32 vertex_index   = _gp.current_vertex;
  SDL_GPVertex *v       = _SDL_GPNextVertices(total_vertices);
  if (!v) {
    return;
  }

  // Compute vertices
  const SDL_GPRect *rect = rects;
  SDL_Color color        = _gp.state.color;
  SDL_GPMat2x3 mvp       = _gp.state.mvp;
  _SDL_GPRegion region   = { FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX };

  for (Uint32 i = 0; i < count; ++i, ++rect) {
    SDL_GPVec2 quad[4] = {
      { rect->x, rect->y + rect->h },           // bottom-left
      { rect->x + rect->w, rect->y + rect->h }, // bottom-right
      { rect->x + rect->w, rect->y },           // top-right
      { rect->x, rect->y },                     // top-left
    };

    _SDL_GPTransform(&mvp, quad, quad, 4);

    for (Uint32 j = 0; j < 4; ++j) {
      region.x1 = SDL_min(region.x1, quad[j].x);
      region.y1 = SDL_min(region.y1, quad[j].y);
      region.x2 = SDL_max(region.x2, quad[j].x);
      region.y2 = SDL_max(region.y2, quad[j].y);
    }

    const SDL_GPVec2 texcoords[4] = {
      { 0.0f, 1.0f }, // bottom-left
      { 1.0f, 1.0f }, // bottom-right
      { 1.0f, 0.0f }, // top-right
      { 0.0f, 0.0f }  // top-left
    };

    // Make two triangles to form the quad
    v[0] = (SDL_GPVertex){ .position = quad[0],
                           .texcoord = texcoords[0],
                           .color    = color };
    v[1] = (SDL_GPVertex){ .position = quad[1],
                           .texcoord = texcoords[1],
                           .color    = color };
    v[2] = (SDL_GPVertex){ .position = quad[2],
                           .texcoord = texcoords[2],
                           .color    = color };
    v[3] = (SDL_GPVertex){ .position = quad[3],
                           .texcoord = texcoords[3],
                           .color    = color };
    v[4] = (SDL_GPVertex){ .position = quad[0],
                           .texcoord = texcoords[0],
                           .color    = color };
    v[5] = (SDL_GPVertex){ .position = quad[2],
                           .texcoord = texcoords[2],
                           .color    = color };
    v += 6;
  }

  // Queue draw
  SDL_GPPipeline pipeline = _SDL_GP_FindOrCreatePipeline(
      SDL_GP_PRIMITIVE_TRIANGLES, _gp.state.blend_mode);

  _SDL_GPQueueDraw(pipeline,
                   region,
                   vertex_index,
                   total_vertices,
                   SDL_GP_PRIMITIVE_TRIANGLES);
}

void
SDL_GPDrawFilledRect(SDL_GPRect rect)
{
  SDL_GPDrawFilledRects(&rect, 1);
}

void
SDL_GPDrawTexturedRects(int channel,
                        const SDL_GPTexturedRect *rects,
                        Uint32 count)
{
  SDL_assert(_gp_initialized == _SDL_GP_INIT_COOKIE);
  SDL_assert(_gp.current_state > 0);

  if (count == 0) {
    return;
  }

  // Setup vertices
  Uint32 total_vertices  = count * 6; // 2 triangles per rect, 3 vertices each
  Uint32 vertex_index    = _gp.current_vertex;
  SDL_GPVertex *vertices = _SDL_GPNextVertices(total_vertices);
  if (!vertices) {
    return;
  }

  // Get image info
  SDL_GPImage image = _gp.state.texture.images[channel];

  int width  = SDL_GPGetImageWidth(image);
  int height = SDL_GPGetImageHeight(image);

  // Check image dimension with unlikely

  float iw = 1.0f / (float)width;
  float ih = 1.0f / (float)height;

  // Compute vertices
  SDL_GPMat2x3 mvp     = _gp.state.mvp;
  SDL_Color color      = _gp.state.color;
  _SDL_GPRegion region = { FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX };

  for (Uint32 i = 0; i < count; ++i) {
    SDL_GPVec2 quad[4] = {
      { rects[i].dst.x, rects[i].dst.y + rects[i].dst.h }, // bottom left
      { rects[i].dst.x + rects[i].dst.w,
        rects[i].dst.y + rects[i].dst.h },                 // bottom right
      { rects[i].dst.x + rects[i].dst.w, rects[i].dst.y }, // top right
      { rects[i].dst.x, rects[i].dst.y },                  // top left
    };

    _SDL_GPTransform(&mvp, quad, quad, 4);

    for (Uint32 j = 0; j < 4; ++j) {
      region.x1 = SDL_min(region.x1, quad[j].x);
      region.y1 = SDL_min(region.y1, quad[j].y);
      region.x2 = SDL_max(region.x2, quad[j].x);
      region.y2 = SDL_max(region.y2, quad[j].y);
    }

    float tl = rects[i].src.x * iw;
    float tt = rects[i].src.y * ih;
    float tr = (rects[i].src.x + rects[i].src.w) * iw;
    float tb = (rects[i].src.y + rects[i].src.h) * ih;

    SDL_GPVec2 vtexquad[4] = {
      { tl, tb }, // bottom-left
      { tr, tb }, // bottom-right
      { tr, tt }, // top-right
      { tl, tt }  // top-left
    };

    SDL_GPVertex *v = &vertices[i * 6];

    v[0] = (SDL_GPVertex){ .position = quad[0],
                           .texcoord = vtexquad[0],
                           .color    = color };
    v[1] = (SDL_GPVertex){ .position = quad[1],
                           .texcoord = vtexquad[1],
                           .color    = color };
    v[2] = (SDL_GPVertex){ .position = quad[2],
                           .texcoord = vtexquad[2],
                           .color    = color };
    v[3] = (SDL_GPVertex){ .position = quad[3],
                           .texcoord = vtexquad[3],
                           .color    = color };
    v[4] = (SDL_GPVertex){ .position = quad[0],
                           .texcoord = vtexquad[0],
                           .color    = color };
    v[5] = (SDL_GPVertex){ .position = quad[2],
                           .texcoord = vtexquad[2],
                           .color    = color };
  }

  // Queue draw
  SDL_GPPipeline pipeline = _SDL_GP_FindOrCreatePipeline(
      SDL_GP_PRIMITIVE_TRIANGLES, _gp.state.blend_mode);

  _SDL_GPQueueDraw(pipeline,
                   region,
                   vertex_index,
                   total_vertices,
                   SDL_GP_PRIMITIVE_TRIANGLES);
}

void
SDL_GPDrawTexturedRect(int channel, SDL_GPTexturedRect rect)
{
  SDL_GPDrawTexturedRects(channel, &rect, 1);
}

#endif // SDL_GP_IMPLEMENTATION

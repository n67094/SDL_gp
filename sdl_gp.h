/**
 * SDL_gp: A 2D (g)raphics (p)ainter for SDL3.
 *
 * This is a port of sokol_gp (https://github.com/edubart/sokol_gp) to SDL3,
 * with one main difference:
 *
 * sokol_gp relies on sokol, which manages resources internally. SDL_gpu is
 * lower-level and does not; So `SDL_gp` provides a simple resource management
 * system very similar to sokol's.
 *
 * I would like to thanks:
 *
 * Edubart (https://github.com/edubart) for his work on
 * sokol_gp, which was a great inspiration for this project and a greate
 * starting point to learn about graphics programming.
 *
 * The SDL team (https://github.com/libsdl-org) for their work.
 *
 * Has well as floooh (https://github.com/floooh/) for sokol.
 *
 * So thanks guys, you are awesome for making these libraries and sharing them
 * with the world.
 *
 * Copyright (c) 2026 nsix. All rights reserved.
 */

#ifndef SDL_GP_H_
#define SDL_GP_H_

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

// Resource limits
// ----------------------------------------------------------------------

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

// Painter limits
// ----------------------------------------------------------------------

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

// ----------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------

// Pool (Resource management)
// ----------------------------------------------------------------------
// This is in the public API so it can be reused for other resources if needed.

#define SDL_GP_POOL_INVALID_SLOT 0
#define SDL_GP_POOL_SLOT_SHIFT 16
#define SDL_GP_POOL_SLOT_MASK ((1 << SDL_GP_POOL_SLOT_SHIFT) - 1)

typedef struct SDL_GPPool {
  size_t size;
  int queue_top;
  Uint32 *counters; // incrementing generation counters for each slot
  int *free_queue;
} SDL_GPPool;

SDL_GPPool *SDL_GPCeatePool(size_t number_of_ids);

void SDL_GPDestroyPool(SDL_GPPool *resource);

int SDL_GPAcquirePoolSlot(SDL_GPPool *resource);

void SDL_GPReleasePoolSlot(SDL_GPPool *resource, int slot_index);

Uint32 SDL_GPGeneratePoolId(SDL_GPPool *resource, int slot_index);

int SDL_GPPoolIdToSlot(Uint32 id);

// Image
// ----------------------------------------------------------------------

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

SDL_GPImage SDL_GPLoadImage(const char *path);

SDL_GPImage SDL_GPLoadImageFrom(unsigned int width, unsigned int height,
                                void *pixels);

void SDL_GPDestroyImage(SDL_GPImage image);

SDL_GPUTexture *SDL_GPGetImageGPUTexture(SDL_GPImage image);

int SDL_GPGetImageWidth(SDL_GPImage image);

int SDL_GPGetImageHeight(SDL_GPImage image);

// Shader
// ----------------------------------------------------------------------

typedef struct SDL_GPShader {
  Uint32 id;
} SDL_GPShader;

typedef struct SDL_GPShaderDesc {
  const char *name;
  Uint32 num_samplers;
  Uint32 num_storage_textures;
  Uint32 num_storage_buffers;
  Uint32 num_uniform_buffers;
} SDL_GPShaderDesc;

SDL_GPShader SDL_GPCreateShader(SDL_GPShaderDesc *vertex_desc,
                                SDL_GPShaderDesc *fragment_desc);

void SDL_GPDestroyShader(SDL_GPShader shader);

SDL_GPUShader *SDL_GPGetGPUShaderVertex(SDL_GPShader shader);

SDL_GPUShader *SDL_GPGetGPUShaderFragment(SDL_GPShader shader);

// Pipeline
// ----------------------------------------------------------------------

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

SDL_GPPipeline SDL_GPCreatePipeline(SDL_GPUGraphicsPipeline *pipeline);

void SDL_GPPipelineDestroy(SDL_GPPipeline pipeline);

SDL_GPUGraphicsPipeline *SDL_GPGetGPUPipeline(SDL_GPPipeline pipeline);

// Painter
// ----------------------------------------------------------------------

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

typedef struct SDL_GPVertex {
  SDL_GPVec2 position;
  SDL_GPVec2 texcoord;
  SDL_GPColor color;
} SDL_GPVertex;

typedef struct SDL_GPDesc {
  SDL_GPUDevice *gpu_device;
  SDL_GPUTexture *target_texture;
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

// ----------------------------------------------------------------------
// Implementation and internal API
// ----------------------------------------------------------------------

// TODO remove next line
#define SDL_GP_IMPLEMENTATION

#ifdef SDL_GP_IMPLEMENTATION

// Pool
// ----------------------------------------------------------------------

SDL_GPPool *SDL_GPCeatePool(size_t number_of_ids) {
  // TODO
  return NULL;
}

void SDL_GPDestroyPool(SDL_GPPool *resource) {
  // TODO
}

int SDL_GPAcquirePoolSlot(SDL_GPPool *resource) {
  // TODO
  return -1;
}

void SDL_GPReleasePoolSlot(SDL_GPPool *resource, int slot_index) {
  // TODO
}

Uint32 SDL_GPGeneratePoolId(SDL_GPPool *resource, int slot_index) {
  // TODO
  return 0;
}

int SDL_GPPoolIdToSlot(Uint32 id) {
  // TODO
  return -1;
}

// Image
// ----------------------------------------------------------------------

static void _SDL_GPImageSetup() {
  // TODO
}

static void _SDL_GPImageShutdown() {
  // TODO
}

SDL_GPImage SDL_GPLoadImage(const char *path) {
  // TODO
  return (SDL_GPImage){0};
}

SDL_GPImage SDL_GPLoadImageFrom(unsigned int width, unsigned int height,
                                void *pixels) {
  // TODO
  return (SDL_GPImage){0};
}

void SDL_GPDestroyImage(SDL_GPImage image) {
  // TODO
}

SDL_GPUTexture *SDL_GPGetImageGPUTexture(SDL_GPImage image) {
  // TODO
  return NULL;
}

int SDL_GPGetImageWidth(SDL_GPImage image) {
  // TODO
  return 0;
}

int SDL_GPGetImageHeight(SDL_GPImage image) {
  // TODO
  return 0;
};

// Shader
// ----------------------------------------------------------------------

typedef enum {
  SDL_GP_VERTEX_SHADER = 0,
  SDL_GP_FRAGMENT_SHADER
} _SDL_GPShaderType;

static void _SDL_GPShaderSetup() {
  // TODO
}

static void _SDL_GPShaderShutdown() {
  // TODO
}

SDL_GPShader SDL_GPCreateShader(SDL_GPShaderDesc *vertex_desc,
                                SDL_GPShaderDesc *fragment_desc) {
  // TODO
  return (SDL_GPShader){0};
}

void SDL_GPDestroyShader(SDL_GPShader shader) {
  // TODO
}

SDL_GPUShader *SDL_GPGetGPUShaderVertex(SDL_GPShader shader) {
  // TODO
  return NULL;
}

SDL_GPUShader *SDL_GPGetGPUShaderFragment(SDL_GPShader shader) {
  // TODO
  return NULL;
};

// Pipeline
// ----------------------------------------------------------------------

static void _SDL_GPPipelineSetup() {
  // TODO
}

static void _SDL_GPPipelineShutdown() {
  // TODO
}

SDL_GPPipeline SDL_GPCreatePipeline(SDL_GPUGraphicsPipeline *pipeline) {
  // TODO
  return (SDL_GPPipeline){0};
}

void SDL_GPPipelineDestroy(SDL_GPPipeline pipeline) {
  // TODO
}

SDL_GPUGraphicsPipeline *SDL_GPGetGPUPipeline(SDL_GPPipeline pipeline) {
  // TODO
  return NULL;
};

// Painter
// ----------------------------------------------------------------------

typedef enum {
  SDL_GP_COMMAND_NONE = 0,
  SDL_GP_COMMAND_DRAW,
  SDL_GP_COMMAND_VIEWPORT,
  SDL_GPCOMMAND_SCISSOR
} _SDL_GPCommandType;

// TODO static functions

void SDL_GPSetup(SDL_GPDesc *desc) {
  // TODO
}

void SDL_GPShutdown() {
  // TODO
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

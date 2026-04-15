# SDL_gp 🏁🏎️

> A minimal, high-performance 2D graphics painter for `SDL3`.

This is a port of [sokol_gp](https://github.com/edubart/sokol_gp) to SDL3, with one main difference:

`sokol_gp` relies on sokol, which manages resources internally. SDL_gpu is lower-level and does not; So `SDL_gp` provides a simple resource management system very similar to sokol's.

// TODO add preview image

# Getting Started

If you want to jump in the right way, check out the [samples](./samples) folder.

A simple example to draw a red rectangle:

```c
#include "SDL_gp.h"

// Begin a new frame
SDL_GPBegin(WINDOW_WIDTH, WINDOW_HEIGHT);
{
    // Clear the screen to black.
    SDL_GPSetColor((SDL_Color){ 0, 0, 0, 255 });
    SDL_GPClear();

    // Draw a red filled rectangle.
    SDL_GPSetColor((SDL_Color){ 255, 0, 0, 255 });
    {
        SDL_GPDrawRectFilled((SDL_GPRect){ 10, 10, 100, 100 });
    }

    // Flush the drawing commands to the GPU.
    SDL_GPFlush(_context.swapchain_texture);
}
SDL_GPEnd();
```

# Quick API Reference

Error API:

```c
// Get the last error that occurred in SDL_gp. Returns SDL_GP_ERROR_NONE if no
// error has occurred.
SDL_GP_Error SDL_GPGetLastError(void);

// Get a human-readable string describing an SDL_GP_Error value. Returns
// "Unknown error" if the error value is not recognized.
const char *SDL_GPGetErrorMessage(SDL_GP_Error error);
```

Image API:

```c
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
```

Shader API:

```c
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
```

Pipeline API:

```c
// Create a graphics pipeline
SDL_GPPipeline SDL_GPCreatePipeline(SDL_GPShader shader,
                                      SDL_GPPrimitiveType primitive_type,
                                      SDL_GPBlendMode blend_mode);

// Destroy a graphics pipeline and free its resources.
void SDL_GPPipelineDestroy(SDL_GPPipeline pipeline);

// Get the GPU graphics pipeline associated with a SDL_gp pipeline. Returns NULL if
// the pipeline is invalid.
SDL_GPUGraphicsPipeline *SDL_GPGetGPUPipeline(SDL_GPPipeline pipeline);
```

Painter API:

```c
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
void SDL_GPDrawTextureddRect(int channel, SDL_GPTexturedRect rect);
```

# Acknowledgements

- [Edubart](https://github.com/edubart) - Creator of the original `sokol_gp`.
- [The SDL team](https://github.com/libsdl-org) - For developing and maintaining `SDL3`.

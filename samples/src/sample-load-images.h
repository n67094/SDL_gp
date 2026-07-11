/**
 * File `sample-load-images.h`.
 *
 * Copyright (c) 2026 nsix. All rights reserved.
 *
 * This example is to test loading images during the setup and render phases.
 *
 * This should create pending images which will be uploaded to the GPU when the
 * command buffer is submitted.
 */

#ifndef SAMPLE_LOAD_IMAGES_H_
#define SAMPLE_LOAD_IMAGES_H_

#include <SDL3/SDL.h>

#include "context.h"

void sample_load_images_setup(Context *context);

void sample_load_images_render(Uint64 delta_time_ms);

void sample_load_images_shutdown(void);

#endif // SAMPLE_LOAD_IMAGES_H_

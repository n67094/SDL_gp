/**
 * @file sample-blend.h
 *
 * @copyright Copyright (c) 2026 nsix. All rights reserved.
 */

#ifndef SAMPLE_BLEND_H_
#define SAMPLE_BLEND_H_

#include <SDL3/SDL.h>

#include "context.h"

void sample_blend_setup(Context *context);

void sample_blend_render(Uint64 detla_time_ms);

void sample_blend_shutdown(void);

#endif // SAMPLE_BLEND_H_

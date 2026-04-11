/**
 * @file sample-rect.h
 *
 * @copyright Copyright (c) 2026 nsix. All rights reserved.
 */

#ifndef SAMPLE_RECT_H_
#define SAMPLE_RECT_H_

#include <SDL3/SDL.h>

#include "context.h"

void sample_rect_setup(Context *context);

void sample_rect_render(Uint64 delta_time_ms);

void sample_rect_shutdown(void);

#endif // SAMPLE_RECT_H_

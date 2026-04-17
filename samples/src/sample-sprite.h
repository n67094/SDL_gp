/**
 * @file sample-sprite.h
 *
 * @copyright Copyright (c) 2026 nsix. All rights reserved.
 */

#ifndef SAMPLE_SPRITE_H_
#define SAMPLE_SPRITE_H_

#include <SDL3/SDL.h>

#include "context.h"

void sample_sprite_setup(Context *context);

void sample_sprite_render(Uint64 delta_time_ms);

void sample_sprite_shutdown(void);

#endif // SAMPLE_SPRITE_H_

/**
 * @file sample-primivite.h
 *
 * @copyright Copyright (c) 2026 nsix. All rights reserved.
 */

#ifndef SAMPLE_PRIMIVITE_H_
#define SAMPLE_PRIMIVITE_H_

#include <SDL3/SDL.h>

#include "context.h"

void sample_primitive_setup(Context *context);

void sample_primitive_render(Uint64 delta_time_ms);

void sample_primitive_shutdown(void);

#endif // SAMPLE_PRIMIVITE_H_

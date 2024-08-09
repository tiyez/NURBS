/*
 * sRGB transform (C)
 *
 * Copyright (c) 2018 Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/srgb-transform-library
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * - The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, express or
 *   implied, including but not limited to the warranties of merchantability,
 *   fitness for a particular purpose and noninfringement. In no event shall the
 *   authors or copyright holders be liable for any claim, damages or other
 *   liability, whether in an action of contract, tort or otherwise, arising from,
 *   out of or in connection with the Software or the use or other dealings in the
 *   Software.
 */

#pragma once


#ifdef __cplusplus
extern "C" {
#endif


static inline float srgb_to_linear_float(float x);
static inline double srgb_to_linear_double(double x);
// const float SRGB_8BIT_TO_LINEAR_FLOAT[1 << 8];
// const double SRGB_8BIT_TO_LINEAR_DOUBLE[1 << 8];

static inline float linear_to_srgb_float(float x);
static inline double linear_to_srgb_double(double x);
static inline int linear_to_srgb_8bit(double x);


#ifdef __cplusplus
}
#endif

#ifndef ENGINE_MATH_H
#define ENGINE_MATH_H

#include <stdint.h>
#include <stdbool.h>

#define PI      3.14159265358979323846f
#define HALF_PI 1.57079632679489661923f
#define RAD2DEG (180.0f/PI)
#define DEG2RAD (PI/180.0f)
#define EPSILON 1e-9

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

float engine_math_dot_product(float x0, float y0, float x1, float y1);
void engine_math_normalize(float *vx, float *vy);

float engine_math_clamp(float value, float min, float max);

bool engine_math_compare_floats(float value0, float value1);

float engine_math_angle_between(float px0, float py0, float px1, float py1);
float engine_math_distance_between(float px0, float py0, float px1, float py1);

uint32_t engine_math_2d_to_1d_index(uint32_t x, uint32_t y, uint32_t width);

void engine_math_2d_midpoint(float x0, float y0, float x1, float y1, float *mx, float *my);

// Rotate a point '(px, py)' about another center point '(cx, cy)'
void engine_math_rotate_point(float *px, float *py, float cx, float cy, float angle_radians);

// Scales a point from a center position
void engine_math_scale_point(float *px, float *py, float cx, float cy, float scale);

#endif  // ENGINE_MATH_H
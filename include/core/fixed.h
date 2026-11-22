#ifndef VOLVY_CORE_FIXED_H
#define VOLVY_CORE_FIXED_H

#include <stdint.h>

/*
 * Signed 20.12 fixed-point.
 * Range: approximately -524288 to +524287.9998.
 * Resolution: 1/4096 of a world unit.
 */
typedef int32_t fx32;

#define FX_SHIFT 12
#define FX_ONE   ((fx32)1 << FX_SHIFT)
#define FX_HALF  (FX_ONE / 2)

static inline fx32 fx_from_int(int32_t value)
{
    return (fx32)((int64_t)value * FX_ONE);
}

static inline int32_t fx_to_int(fx32 value)
{
    return value / FX_ONE;
}

static inline fx32 fx_mul(fx32 a, fx32 b)
{
    return (fx32)(((int64_t)a * (int64_t)b) >> FX_SHIFT);
}

static inline fx32 fx_add(fx32 a, fx32 b)
{
    return a + b;
}

static inline fx32 fx_sub(fx32 a, fx32 b)
{
    return a - b;
}

static inline fx32 fx_clamp(fx32 value, fx32 minimum, fx32 maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

#endif

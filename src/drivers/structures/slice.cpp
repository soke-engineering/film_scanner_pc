#include "slice.h"

#include <new>

Slice::Slice(const uint16_t *data,
             const uint32_t  number,
             const uint32_t  x,
             const uint32_t  y,
             const bool      new_frame)
    : data((uint16_t *)data), number(number), x(x), y(y), new_frame(new_frame)
{
    // Calculate the average pixel value of the slice
    average = 0;
    for (uint32_t i = 0; i < (x * y); i++)
    {
        average += (uint32_t)data[i];
    }
    average /= (x * y);
}

Slice::~Slice(void) {}

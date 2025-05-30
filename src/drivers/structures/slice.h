#pragma once

#include <stdint.h>

class Slice
{
  public:
    Slice(const uint16_t *data,
          const uint32_t  number,
          const uint32_t  x,
          const uint32_t  y,
          const bool      new_frame);
    ~Slice(void);

  public:
    uint16_t *data;
    uint32_t  number;
    uint32_t  x, y;
    bool      new_frame;

    // Statistics
    uint32_t average;
};

#pragma once

#include "slice.h"

#include <stdint.h>

class Frame
{
  public:
    Frame(const Slice &start, const Slice &end);
    ~Frame(void);

  public:
    uint16_t *data;
    uint32_t  x, y;
};

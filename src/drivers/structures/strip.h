#pragma once

#include "frame.h"
#include "slice.h"

#include <stdint.h>
#include <vector>

class Strip
{
  public:
    Strip(const uint32_t x, const uint32_t y, const uint32_t slice_width);
    ~Strip(void);
    Slice *addSlice(void);
    void   addFrame(const Frame &frame);
    Frame *readFrame(const Slice &start, const Slice &end);
    Frame *readFrame(const Slice &start, const uint32_t length);

  private:
    uint32_t           x, y;
    uint32_t           slice_width;
    uint16_t          *data;
    std::vector<Slice> slices;
    std::vector<Frame> frames;
};

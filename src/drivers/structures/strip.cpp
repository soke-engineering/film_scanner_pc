#include "strip.h"

#include <new>

Strip::Strip(const uint32_t x, const uint32_t y, const uint32_t slice_width)
    : slice_width(slice_width), x(x), y(y)
{
    data = new uint16_t[x * y]();

    slices.clear();
    frames.clear();
}

Strip::~Strip(void) { delete[] data; }

Slice *Strip::addSlice(void)
{
    Slice slice(data, 0, x, slice_width, false);

    // If there are slices in the vector, increment from the latest slice
    if (slices.empty() == false)
    {
        auto latest  = slices.back();
        slice.data   = latest.data + (slice.x * slice.y);
        slice.number = latest.number++;
    }

    slices.push_back(slice);
    return &slices.back();
}

void Strip::addFrame(const Frame &frame) { frames.push_back(frame); }

Frame *Strip::readFrame(const Slice &start, const Slice &end)
{
    if ((start.number > slices.size()) || (end.number > slices.size()))
    {
        return nullptr;
    }

    Frame *frame = new Frame(start, end);

    return frame;
}

Frame *Strip::readFrame(const Slice &start, const uint32_t length)
{
    // Check bounds
    if ((start.number > slices.size()) || ((start.number + length) > slices.size()))
    {
        return nullptr;
    }

    return readFrame(start, slices[start.number + length]);
}

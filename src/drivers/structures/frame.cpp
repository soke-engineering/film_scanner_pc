#include "frame.h"

Frame::Frame(const Slice &start, const Slice &end)
    : data((uint16_t *)start.data), y(start.y), x(start.x * (end.number - start.number))
{
}

Frame::~Frame(void) {}

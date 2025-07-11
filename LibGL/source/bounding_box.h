#pragma once

#include <algorithm>
#include <limits>
#include "../../LibMath/source/stdafx.h" // Your 3D vector class

class BoundingBox
{
public:
    SVector3Df min;
    SVector3Df max;

    BoundingBox()
    {
        Reset();
    }

    BoundingBox(const SVector3Df& min, const SVector3Df& max)
        : min(min), max(max) {}

    void Reset()
    {
        min = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
        max = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };
    }

    void ExpandToInclude(const SVector3Df& point)
    {
        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        min.z = std::min(min.z, point.z);

        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        max.z = std::max(max.z, point.z);
    }

    void ExpandToInclude(const BoundingBox& other)
    {
        ExpandToInclude(other.min);
        ExpandToInclude(other.max);
    }

    SVector3Df GetCenter() const
    {
        return {
            (min.x + max.x) * 0.5f,
            (min.y + max.y) * 0.5f,
            (min.z + max.z) * 0.5f
        };
    }

    SVector3Df GetExtent() const
    {
        return {
            (max.x - min.x) * 0.5f,
            (max.y - min.y) * 0.5f,
            (max.z - min.z) * 0.5f
        };
    }

    bool Intersects(const BoundingBox& other) const
    {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
            (min.y <= other.max.y && max.y >= other.min.y) &&
            (min.z <= other.max.z && max.z >= other.min.z);
    }

    bool Contains(const SVector3Df& point) const
    {
        return (point.x >= min.x && point.x <= max.x) &&
            (point.y >= min.y && point.y <= max.y) &&
            (point.z >= min.z && point.z <= max.z);
    }

    float GetWidth() const { return max.x - min.x; }
    float GetHeight() const { return max.y - min.y; }
    float GetDepth() const { return max.z - min.z; }
};

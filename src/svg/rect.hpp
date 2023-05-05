#ifndef __RECT_HPP__
#define __RECT_HPP__

#include <stdexcept>
#include <algorithm>
#include <array>
#include <cmath>
//#include <boost/variant.hpp>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "Interval.hpp"

using IntervalPair = std::tuple<Interval, Interval>;

/**
 * Represents a rectangle.
 *
 * The following points hold true for a Rectangle:
 * 1. A rectangle is simply defined by a pair of two Intervals. The set
 * of a rectangle $R$ defined by two Intervals $A$ and $B$ is the set
 * of all points $(x, y)$ such that $x$ is a member of Interval $A$'s
 * set while $y$ is a member of Interval $B$'s set.
 * 2. A rectangle is empty if its set is of length zero.
 * 3. A rectangle $A$ contains a rectangle $B$ if all points of rectangle
 * $B$'s set are also members of the set of rectangle $A$.
 * 4. The intersection of two rectangles $A$ and $B$ is a rectangle whose
 * set contains the elements that are common in the point set of $A$ and
 * that of $B$.
 * 5. The join of two rectangles $A$ and $B$ is a rectangle with the smallest
 * possible point set such that it has all points of $A$ as well as those of
 * set $B$ and is complete, meaning that for any two points in the set, all
 * the points lying between will also be a part of the set. By smallest we mean
 * that no other proper subset of that set should satisfy this requirement.
 */
class Rect
{
  public:
    Rect() = default;
    Rect(float aX, float aY, float aWidth, float aHeight)
    {
        if (aWidth < 0 || aHeight < 0)
        {
            throw std::invalid_argument("Height or Width of a rectangle can't be negative!");
        }
        x = aX;
        y = aY;
        width = aWidth;
        height = aHeight;
    }
    /**
     * Returns if the rectangle is empty
     */
    bool IsEmpty() const
    {
        IntervalPair rectIntervals = Intervals();
        return std::get<0>(rectIntervals).IsEmpty() || std::get<1>(rectIntervals).IsEmpty();
    }

    /**
     * Returns the two intervals defining the rectangle
     * Returns true if Rect contains the other Rect within it
     */
    bool Contains(Rect other) const
    {
        IntervalPair thisIntervals = Intervals();
        IntervalPair otherIntervals = other.Intervals();
        return std::get<0>(thisIntervals).Contains(std::get<0>(otherIntervals)) &&
            std::get<1>(thisIntervals).Contains(std::get<1>(otherIntervals));
    }

    /**
     * Computes the intersection of Rect with the other Rect, meaning a rectangle encompassing area
     * that is common in both.
     */
    Rect operator&(Rect other) const
    {
        IntervalPair intervalsA = Intervals();
        IntervalPair intervalsB = other.Intervals();
        Interval resultX = std::get<0>(intervalsA) & std::get<0>(intervalsB);
        Interval resultY = std::get<1>(intervalsA) & std::get<1>(intervalsB);
        if (resultX.IsEmpty() || resultY.IsEmpty())
            return Rect{0, 0, 0, 0};
        return Rect(resultX.Min(), resultY.Min(), resultX.Max() - resultX.Min(), resultY.Max() - resultY.Min());
    }
    /**
     * Returns true if two rectangles are the same
     */
    bool operator==(Rect other) const
    {
        return x == other.x && y == other.y && width == other.width && height == other.height;
    }
    /**
     * Computes the join of two rectangles, meaning a bigger rectangle that contains both of those.
     */
    Rect operator|(Rect other) const
    {
        IntervalPair intervalsA = Intervals();
        IntervalPair intervalsB = other.Intervals();
        Interval resultX = std::get<0>(intervalsA) | std::get<0>(intervalsB);
        Interval resultY = std::get<1>(intervalsA) | std::get<1>(intervalsB);
        if (resultX.IsEmpty() || resultY.IsEmpty())
            return Rect{0, 0, 0, 0};
        return Rect(resultX.Min(), resultY.Min(), resultX.Max() - resultX.Min(), resultY.Max() - resultY.Min());
    }

    float Area() const { return width * height; }

    float MaxDiffVertex(Rect other) const
    {
        float topLeftDiff = std::sqrt(std::pow(Left() - other.Left(), 2) + std::pow(Top() - other.Top(), 2));
        float topRightDiff = std::sqrt(std::pow(Right() - other.Right(), 2) + std::pow(Top() - other.Top(), 2));
        float bottomLeftDiff = std::sqrt(std::pow(Left() - other.Left(), 2) + std::pow(Bottom() - other.Bottom(), 2));
        float bottomRightDiff = std::sqrt(std::pow(Right() - other.Right(), 2) + std::pow(Bottom() - other.Bottom(), 2));

        float max1 = std::max(topLeftDiff, topRightDiff);
        float max2 = std::max(bottomLeftDiff, bottomRightDiff);

        return std::max(max1,max2);
    }

    float Left() const { return x; }
    float Right() const { return x + width; }
    float Top() const { return y; }
    float Bottom() const { return y + height; }

    float x = std::numeric_limits<float>::quiet_NaN();
    float y = std::numeric_limits<float>::quiet_NaN();
    float width = std::numeric_limits<float>::quiet_NaN();
    float height = std::numeric_limits<float>::quiet_NaN();

  private:
    IntervalPair Intervals() const { return IntervalPair(Interval(x, x + width), Interval(y, y + height)); }
};


#endif /* __RECT_HPP__ */
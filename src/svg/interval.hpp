#ifndef __INTERVAL_HPP__
#define __INTERVAL_HPP__

#include <tuple>
#include <algorithm>
/**
 * Represents an open interval.
 *
 * TODO: Just remove these mathematical definitions?
 *
 * The following statements hold true for an interval:
 * 1. An interval $I$ is defined as the set of all numbers between two
 * numbers $u$ and $v$ on the number line, but not equal to either of them.
 * 2. If $u$ and $v$ are the same numbers, the set becomes empty and thus
 * the interval is also recognized as empty.
 * 3. An interval $I$ is said to contain an interval $C$ if each element in
 * the set of interval $C$ is also a member of the set of interval $I$.
 * 4. The join of two intervals $A$ and $B$ is defined as an interval whose
 * set is defined as the smallest possible set of *complete* real numbers that
 * contain all members of $A$ as well as $B$. By complete we simply mean that
 * for any possible two members of the set, all real numbers between the two
 * must also be within the set. The word smallest here means that no other proper
 * subset of that set will satisfy all the requirements.
 * 5. The intersection of two intervals $A$ and $B$ is the one whose set is
 * the intersection of the sets of $A$ and $B$.
 */
class Interval
{
  public:
    Interval() = default;
    Interval(float u): Interval(u, u) {}
    Interval(float u, float v)
    {
        if (u <= v)
        {
            mA = u;
            mB = v;
        }
        else
        {
            mA = v;
            mB = u;
        }
    }

    /**
     * Returns the greatest possible real number that's small than every
     * member of this interval's set.
     */
    float Min() const { return mA; }

    /**
     * Returns the smallest possible real number that's greater than every
     * member of this interval's set.
     */
    float Max() const { return mB; }
    operator bool() const { return !IsEmpty(); }

    /* Computes the intersection of this interval with another one */
    Interval operator& (Interval other) const
    {
        // return an empty interval if either of the Intervals is empty
        if ((!*this) || (!other))
            return Interval();

        float u = (std::max)(this->Min(), other.Min());
        float v = (std::min)(this->Max(), other.Max());
        return (u <= v) ? Interval(u, v) : Interval();

    }

    /* Computes the join of this interval with another one, think of an interval that
     * contains both the intervals */
    Interval operator| (Interval other) const
    {
        if (this->IsEmpty() && !other.IsEmpty())
            return other;
        else if(!this->IsEmpty() && other.IsEmpty())
            return *this;
        else if(this->IsEmpty() && other.IsEmpty())
            return Interval();
        else
            return Interval((std::min)(this->Min(), other.Min()), (std::max)(this->Max(), other.Max()));
    }

    /* Returns true if one interval contains another one */
    bool Contains (Interval other) const
    {
        if (other.IsEmpty())
            return true;
        else if(this->IsEmpty())
            return false;
        else
            return this->Min() <= other.Min() && this->Max() >= other.Max();
    }

    /* Is the interval empty? having no points in its set? */
    bool IsEmpty() const { return mA == mB; }
  private:
    float mA{};
    float mB{};
};



#endif /* __INTERVAL_HPP__ */


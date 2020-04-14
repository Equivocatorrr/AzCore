/*
    File: Equations.hpp
    Author: Philip Haynes
*/

#ifndef AZCORE_MATH_EQUATIONS_HPP
#define AZCORE_MATH_EQUATIONS_HPP

#include "basic.hpp"

namespace AzCore {

template <typename T>
struct SolutionLinear
{
    T x;
    bool real = false;
};

template <typename T>
SolutionLinear<T> SolveLinear(T a, T b)
{
    SolutionLinear<T> solution;
    if (a != T(0.0))
    {
        solution.x = -b / a;
        solution.real = true;
    }
    return solution;
}

template <typename T>
struct SolutionQuadratic
{
    T x1, x2;
    bool x1Real = false, x2Real = false;
    SolutionQuadratic() = default;
    SolutionQuadratic(const SolutionLinear<T> &solution) : x1(solution.x), x1Real(solution.real) {}
};

template <typename T>
SolutionQuadratic<T> SolveQuadratic(T a, T b, T c)
{
    SolutionQuadratic<T> solution;
    if (a == T(0.0))
    {
        return SolveLinear<T>(b, c);
    }
    const T bb = square(b);
    const T ac4 = T(4.0) * a * c;
    if (bb < ac4)
    {
        // We don't care about complex answers
        return solution;
    }
    if (bb == ac4)
    {
        solution.x1 = -b / a;
        solution.x1Real = true;
        return solution;
    }
    const T squareRoot = sqrt(bb - ac4);
    a *= T(2.0);
    solution.x1 = (-b + squareRoot) / a;
    solution.x1Real = true;
    solution.x2 = (-b - squareRoot) / a;
    solution.x2Real = true;
    return solution;
}

template <typename T>
struct SolutionCubic
{
    T x1, x2, x3;
    bool x1Real, x2Real, x3Real;
    SolutionCubic() = default;
    SolutionCubic(const SolutionQuadratic<T> &solution) : x1(solution.x1), x2(solution.x2), x1Real(solution.x1Real), x2Real(solution.x2Real), x3Real(false) {}
};

template <typename T>
SolutionCubic<T> SolveCubic(T a, T b, T c, T d)
{
    SolutionCubic<T> solution;
    if (a == T(0.0))
    {
        return SolveQuadratic<T>(b, c, d);
    }
    // We'll use Cardano's formula
    // First we need to be in terms of the depressed cubic.
    // So we take our current form:
    // ax^3 + bx^2 + cx + d = 0
    // divide by a to get:
    // x^3 + ix^2 + jx + k = 0
    // where i = b/a, j = c/a, and k = d/a
    const T i = b / a;
    const T j = c / a;
    const T k = d / a;
    // substitute t for x to get
    // t^3 + pt + q
    // where t = (x + i/3), p = (j - i^2/3), and q = (k + 2i^3/27 - ij/3)
    const T p = j - i * i / T(3.0);
    const T q = -i * (T(2.0) * i * i - T(9.0) * j) / T(27.0) - k;
    const T p3 = p * p * p;
    const T sqrD = q * q + p3 * T(4.0) / T(27.0);
    const T offset = -i / T(3.0); // Since t = (x + i/3), x = t - i/3
    if (sqrD > T(0.0))
    {
        // We have a single real solution
        const T rootD = sqrt(sqrD);
        const T u = cubert((q + rootD) * T(0.5));
        const T v = cubert((q - rootD) * T(0.5));
        solution.x1 = u + v + offset;
        solution.x1Real = true;
        solution.x2Real = false;
        solution.x3Real = false;
    }
    else if (sqrD < T(0.0))
    {
        // We have 3 real solutions
        const T u = T(2.0) * sqrt(-p / T(3.0));
        const T v = acos(sqrt(T(-27.0) / p3) * q * T(0.5)) / T(3.0);
        solution.x1 = u * cos(v) + offset;
        solution.x2 = u * cos(v + T(tau64) / T(3.0)) + offset;
        solution.x3 = u * cos(v + T(tau64) * T(2.0) / T(3.0)) + offset;
        solution.x1Real = true;
        solution.x2Real = true;
        solution.x3Real = true;
    }
    else
    {
        // We have 2 real solutions
        const T u = cubert(q * T(0.5));
        solution.x1 = u * T(2.0) + offset;
        solution.x2 = -u + offset;
        solution.x1Real = true;
        solution.x2Real = true;
        solution.x3Real = false;
    }
    return solution;
}

} // namespace AzCore

#endif // AZCORE_MATH_EQUATIONS_HPP

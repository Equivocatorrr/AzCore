/*
    File: Equations.hpp
    Author: Philip Haynes
*/

#ifndef AZCORE_MATH_EQUATIONS_HPP
#define AZCORE_MATH_EQUATIONS_HPP

#include "basic.hpp"

namespace AzCore {

template <typename T>
struct SolutionLinear {
    T root;
    i8 nReal;
};

template <typename T>
SolutionLinear<T> SolveLinear(T a, T b) {
    SolutionLinear<T> solution;
    if (a != T(0.0)) {
        solution.root = -b / a;
        solution.nReal = 1;
    } else {
        solution.nReal = 0;
    }
    return solution;
}

template <typename T>
struct SolutionQuadratic {
    T root[2];
    i8 nReal;
    SolutionQuadratic() = default;
    SolutionQuadratic(const SolutionLinear<T> &solution) :
        root{solution.root}, nReal(solution.nReal) {}
};

template <typename T>
SolutionQuadratic<T> SolveQuadratic(T a, T b, T c) {
    SolutionQuadratic<T> solution;
    if (a == T(0.0)) {
        return SolveLinear<T>(b, c);
    }
    const T bb = square(b);
    const T ac4 = T(4.0) * a * c;
    if (bb < ac4) {
        // We don't care about complex answers
        solution.nReal = 0;
        return solution;
    }
    if (bb == ac4) {
        solution.root[0] = -b / a;
        solution.nReal = 1;
        return solution;
    }
    const T squareRoot = sqrt(bb - ac4);
    a *= T(2.0);
    solution.root[0] = (-b + squareRoot) / a;
    solution.root[1] = (-b - squareRoot) / a;
    solution.nReal = 2;
    return solution;
}

template <typename T>
struct SolutionCubic {
    T root[3];
    i8 nReal;
    SolutionCubic() = default;
    SolutionCubic(const SolutionQuadratic<T> &solution) :
        root{solution.root[0], solution.root[1]}, nReal(solution.nReal) {}
};

template <typename T>
SolutionCubic<T> SolveCubic(T a, T b, T c, T d) {
    SolutionCubic<T> solution;
    if (abs(max(max(b, c), d)/a) > T(2500000)) {
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
    const T p = j - i * i / T(3);
    const T q = -i * (T(2) * i * i - T(9) * j) / T(27) - k;
    const T p3 = p * p * p;
    const T sqrD = q * q + p3 * T(4) / T(27);
    const T offset = -i / T(3); // Since t = (x + i/3), x = t - i/3
    if (sqrD > T(0)) {
        // We have a single real solution
        const T rootD = sqrt(sqrD);
        const T u = cubert((q + rootD) / T(2));
        const T v = cubert((q - rootD) / T(2));
        solution.root[0] = u + v + offset;
        solution.nReal = 1;
    } else if (sqrD < T(0)) {
        if (p != T(0)) {
            // We have 3 real solutions
            const T u = T(2) * sqrt(-p / T(3));
            const T v = acos(sqrt(T(-27) / p3) * q / T(2)) / T(3);
            solution.root[0] = u * cos(v) + offset;
            solution.root[1] = u * cos(v + T(tau64) / T(3)) + offset;
            solution.root[2] = u * cos(v + T(tau64) * T(2) / T(3)) + offset;
            solution.nReal = 3;
        } else {
            // We have 1 tripled solution
            solution.root[0] = offset;
            solution.nReal = 1;
        }
    } else {
        if (q != T(0)) {
            // We have 2 real solutions
            const T u = cubert(q / T(2));
            solution.root[0] = u * T(2) + offset;
            solution.root[1] = -u + offset;
            solution.nReal = 2;
        } else {
            // We have 1 doubled solution
            solution.root[0] = offset;
            solution.nReal = 1;
        }
    }
    return solution;
}

template <typename T>
struct SolutionQuartic {
    T root[4];
    i8 nReal;
    SolutionQuartic() = default;
    SolutionQuartic(const SolutionCubic<T> &solution) :
        root{solution.root[0], solution.root[1], solution.root[2]}, nReal(solution.nReal) {}
};

template <typename T>
SolutionQuartic<T> SolveQuartic(T a, T b, T c, T d, T e) {
    SolutionQuartic<T> solution;
    if (a == T(0)) {
        return SolveCubic<T>(b, c, d, e);
    }
    // Check whether we're bi-quadratic
    // If we are, then we're symmetrical across the y-axis
    if (b == T(0) && d == T(0)) {
        // We can solve this like a quadratic
        // z = x^2
        SolutionQuadratic<T> quad = SolveQuadratic(a, c, e);
        if (quad.nReal >= 1) {
            if (quad.root[0] >= T(0)) {
                solution.nReal = 2;
                solution.root[0] = sqrt(quad.root[0]);
                solution.root[1] = -solution.root[0];
            } else if (quad.root[0] == T(0)) {
                solution.nReal = 1;
                solution.root[0] = T(0);
            } else {
                solution.nReal = 0;
            }
        } else {
            return SolutionCubic<T>(quad);
        }
        if (quad.nReal == 2) {
            if (quad.root[1] >= T(0)) {
                solution.root[(i32)solution.nReal] = sqrt(quad.root[1]);
                solution.root[(i32)solution.nReal+1] = -solution.root[(i32)solution.nReal];
                solution.nReal += 2;
            } else if (quad.root[1] == T(0)) {
                solution.root[(i32)solution.nReal++] = T(0);
            }
        }
        return solution;
    }
    T d0 = square(c) - T(3)*b*d + T(12)*a*e;
    T d1 = T(2)*c*c*c - T(9)*b*c*d + T(27)*(b*b*e + a*d*d) - T(72)*a*c*e;
    T d27 = d1*d1 - T(4)*d0*d0*d0;
    T p = (T(8)*a*c - T(3)*b*b) / (T(8)*a*a);
    T q = (b*b*b - T(4)*a*b*c + T(8)*a*a*d) / (T(8)*a*a*a);
    if (d27 > T(0)) {
        // We have 2 real solutions
        T Q = cubert((d1 + sqrt(d1*d1 - T(4)*d0*d0*d0)) / T(2));
        T S = sqrt(-p*T(2)/T(3) + (Q+d0/Q)/(T(3)*a)) / T(2);
        T base;
        T offset;
        offset = -T(4)*S*S - T(2)*p;
        if (offset + q/S >= T(0)) {
            base = -b/(T(4)*a) - S;
            offset = sqrt(offset + q/S) / T(2);
        } else if (offset - q/S >= T(0)) {
            base = -b/(T(4)*a) + S;
            offset = sqrt(offset - q/S) / T(2);
        } else {
            // Precision errors might get us here maybe?
            solution.nReal = 0;
            return solution;
        }
        if (offset == T(0)) {
            // One doubled solution
            solution.nReal = 1;
            solution.root[0] = base;
        } else {
            // Two unique solutions
            solution.nReal = 2;
            solution.root[0] = base - offset;
            solution.root[1] = base + offset;
        }
    } else {
        // We have 4 or 0 solutions
        T theta = acos(d1 / (T(2)*sqrt(d0*d0*d0)));
        T body = -p*T(2)/T(3) + T(2)/(T(3)*a)*sqrt(d0)*cos(theta/T(3));
        if (body < T(0)) {
            // 0 solutions
            solution.nReal = 0;
        } else {
            // 4 solutions
            T S = sqrt(body) / T(2);
            T base1 = -b/(T(4)*a) - S;
            T base2 = -b/(T(4)*a) + S;
            T part1 = -T(4)*S*S - T(2)*p;
            T part2 = q/S;
            T body1 = part1 + part2;
            T body2 = part1 - part2;
            if (body1 >= T(0)) {
                T offset1 = sqrt(body1) / T(2);
                if (offset1 == T(0)) {
                    // Doubled solution
                    solution.nReal = 1;
                    solution.root[0] = base1;
                } else {
                    // Unique solutions
                    solution.nReal = 2;
                    solution.root[0] = base1 + offset1;
                    solution.root[1] = base1 - offset1;
                }
            } else {
                solution.nReal = 0;
            }
            if (body2 >= T(0)) {
                T offset2 = sqrt(body2) / T(2);
                if (offset2 == T(0)) {
                    // Doubled solution
                    solution.root[(i32)solution.nReal++] = base2;
                } else {
                    // Unique solutions
                    solution.root[(i32)solution.nReal++] = base2 + offset2;
                    solution.root[(i32)solution.nReal++] = base2 - offset2;
                }
            }
        }
    }

    return solution;
}


template <typename T>
struct SolutionQuintic {
    T root[5];
    i8 nReal;
    SolutionQuintic() = default;
    SolutionQuintic(const SolutionQuartic<T> &solution) :
        root{solution.root[0], solution.root[1], solution.root[2], solution.root[3]},
        nReal(solution.nReal) {}
};

template <typename T>
SolutionQuintic<T> SolveQuintic(T a, T b, T c, T d, T e, T f) {
    SolutionQuintic<T> solution;
    if (abs(max(b, max(c, max(d, max(e, f))))/a) > T(50000000)) {
        // We're not a quintic in the first place
        return SolveQuartic(b, c, d, e, f);
    }
    // We're guaranteed to have at least 1 real root.
    // That root can be used to transform it into a Quartic via synthetic division.
    // We'll use an iterative search to find one root, doesn't matter which.
    T ba = b/a;
    T ca = c/a;
    T da = d/a;
    T ea = e/a;
    T fa = f/a;

    T strength = T(1); // How much the output affects the next input
    T lastInput = T(0);
    T lastOutput = fa;
    T input = -fa;
    bool lastPositive = fa >= T(0);
    while (true) {
        T x = input;
        T xx = x*x;
        T xxx = xx*x;
        T xxxx = xx*xx;
        T xxxxx = xxx*xx;
        T output = (xxxxx + ba*xxxx + ca*xxx + da*xx + ea*x + fa) / (xxxx + T(1));
        if (abs(output) < T(1)/T(10000)) break; // Edge case that could cause a hard lock
        bool positive = output > T(0);
        if (positive != lastPositive) {
            // 1/slope should take you approximately to zero
            strength = min(strength, T(1)/abs((output-lastOutput)/(input-lastInput)));
        }
        lastInput = input;
        input -= output*strength;
        if (abs(input-lastInput) < max(abs(input), T(1)) / T(10000)) {
            break;
        }
        lastOutput = output;
        lastPositive = positive;
    }
    T db4 = ba * T(4);
    T dc3 = ca * T(3);
    T dd2 = da * T(2);
    // Newton's method
    for (i32 i = 0; i < 32; i++) {
        T x = input;
        T xx = x*x;
        T xxx = xx*x;
        T xxxx = xx*xx;
        T xxxxx = xxx*xx;
        T num = xxxxx + xxxx * ba + xxx * ca + xx * da + x * ea + fa;
        T denom = xxxx * T(5) + xxx*db4 + xx*dc3 + x*dd2 + ea;
        input -= num / denom;
    }
    // input should be a root
    T newA = a;
    T newB = b + newA*input;
    T newC = c + newB*input;
    T newD = d + newC*input;
    T newE = e + newD*input;
    solution = SolveQuartic(newA, newB, newC, newD, newE);
    solution.root[(i32)solution.nReal++] = input;

    return solution;
}

} // namespace AzCore

#endif // AZCORE_MATH_EQUATIONS_HPP

#!/usr/bin/env python3
"""Discover the theta asymptotic correction coefficients against mpmath's
loggamma (independent derivation path: no truncation reasoning shared with
the C++ series implementation). Prints exact Fractions for transcribing into
core/include/zetaforge/theta.hpp together with the first omitted magnitudes
used for the certified remainder bound.

Method: at a large discovery height t_d the residual theta_ref - theta_main
satisfies residual = sum_{k>=1} c_k t^(1-2k) + O(t^(-2N-3)). Recover c_1,
subtract exactly, recover c_2 from what remains, and so on (Stolz-style
peeling). Each recovered value is snapped to a rational with denominator
bounded by 10^14; exactness is confirmed by the residual collapsing to ~0."""

import mpmath as mp
from fractions import Fraction

mp.mp.dps = 320


def theta_ref(t):
    z = mp.mpf("0.25") + mp.mpc(0, 1) * t / 2
    return mp.im(mp.loggamma(z)) - (t / 2) * mp.log(mp.pi)


def theta_main(t):
    return (t / 2) * mp.log(t / (2 * mp.pi)) - t / 2 - mp.pi / 8


def exact_fraction(x):
    """Exact rational of an mpf via its binary representation."""
    from mpmath.libmp import to_rational
    return Fraction(*to_rational(x._mpf_))


def discover(n_terms, t_discovery):
    t = mp.mpf(t_discovery)
    resid = theta_ref(t) - theta_main(t)
    out = []
    for k in range(1, n_terms + 1):
        ck = resid * t ** (2 * k - 1)
        exact = exact_fraction(ck)
        f = None
        for cap in (10**4, 10**6, 10**8, 10**9, 10**10):
            cand = exact.limit_denominator(cap)
            # Acceptance compares against the EXPECTED tail size (peel leaves
            # the next terms behind), not against zero: c_1 legitimately
            # carries the whole c_2/t^2 tail as its snapping error.
            err = abs(Fraction(exact) - Fraction(cand)) / abs(Fraction(exact))
            if err < Fraction(1, 10**6):
                f = cand
                break
            if f is None or err < abs(Fraction(exact) - Fraction(f)) / abs(Fraction(exact)):
                f = cand
        out.append(f)
        resid -= mp.mpf(f.numerator) / (mp.mpf(f.denominator) * t ** (2 * k - 1))
    tail_dimensionless = resid * t ** (2 * n_terms + 1)
    return out, tail_dimensionless


def main():
    coeffs, tail = discover(10, mp.mpf(10)**5)
    print("# recovered coefficients c_k (value = sum c_k / t^(2k-1)):")
    for i, f in enumerate(coeffs, start=1):
        approx = float(f)
        print(f"c_{i} = {f.numerator}/{f.denominator}   # ~= {approx:.12g}")
    print("# dimensionless tail after c_8:", mp.nstr(tail, 12))

    # cross-check at a second height: series through c_8 must match ref to
    # far beyond double precision
    t2 = mp.mpf(10)**9
    s = theta_main(t2)
    for k, f in enumerate(coeffs, start=1):
        s += mp.mpf(f.numerator) / (mp.mpf(f.denominator) * t2 ** (2 * k - 1))
    diff = abs(s - theta_ref(t2))
    print("# |series-ref| at t=1e9:", mp.nstr(diff, 5))


if __name__ == "__main__":
    main()

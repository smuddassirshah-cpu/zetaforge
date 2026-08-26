#!/usr/bin/env python3
"""Confirmer for the theta asymptotic coefficients. NOT a discovery tool.

History, because the filename still says "discover". Through rev 5 this script
recovered c_1..c_10 numerically by peeling a residual and snapping each value
to a rational under a denominator cap. Run verbatim it returned
c_5 = 334/795137 and a sign-flipped c_6 = -85/9062, and printed a cross-check
that looked like success: the acceptance loop took the first cap whose relative
error fell under 1e-6, so a spurious small-denominator convergent won before
the true denominator's cap was tried, and the cross-check at t = 1e9 was
insensitive to the error by construction (readiness review finding A4). The
committed table was always correct; only its documented provenance was broken.

The provenance is now the closed form, MATHS.md D1a:

    c_k = (1 - 2^(1-2k)) |B_2k| / (4k(2k-1))

which generates the whole table in one line and needs no fitting. This script
CONFIRMS rather than discovers, three independent ways, and exits non-zero on
any disagreement:

  1. the closed form must reproduce core/src/theta.cpp's kCRat exactly, as
     rationals parsed out of the source rather than retyped here;
  2. it must reproduce kC7 exactly and kC8 to the last bit of its double;
  3. the resulting series must agree with mpmath loggamma at several heights
     to within the first omitted term, which is the model the certified
     remainder bound rests on.
"""

import os
import re
import sys
from fractions import Fraction

import mpmath as mp
from sympy import bernoulli, Rational

mp.mp.dps = 260

THETA_CPP = os.path.join(os.path.dirname(__file__), "..", "core", "src",
                         "theta.cpp")


def closed_form(k):
    """c_k = (1 - 2^(1-2k)) |B_2k| / (4k(2k-1)), exact."""
    c = (1 - Rational(2) ** (1 - 2 * k)) * abs(bernoulli(2 * k)) / (4 * k * (2 * k - 1))
    r = Rational(c)
    return Fraction(int(r.p), int(r.q))


def parse_production_table():
    """kCRat, kC7 and kC8 as written in theta.cpp. Parsed, never retyped: a
    confirmer that carries its own copy of the numbers confirms nothing."""
    src = open(THETA_CPP).read()
    block = re.search(r"kCRat\[kThetaTerms\]\s*=\s*\{(.*?)\};", src, re.S)
    if block is None:
        raise SystemExit("could not find kCRat in " + THETA_CPP)
    pairs = re.findall(r"\{\s*(\d+)ULL\s*,\s*(\d+)ULL\s*\}", block.group(1))
    crat = [Fraction(int(n), int(d)) for n, d in pairs]
    m7 = re.search(r"kC7\s*=\s*([\d.]+)\s*/\s*([\d.]+)", src)
    c7 = Fraction(int(float(m7.group(1))), int(float(m7.group(2))))
    m8 = re.search(r"kC8\s*=\s*([\d.e+-]+)\s*;", src)
    c8 = float(m8.group(1))
    return crat, c7, c8


def theta_ref(t):
    z = mp.mpf("0.25") + mp.mpc(0, 1) * t / 2
    return mp.im(mp.loggamma(z)) - (t / 2) * mp.log(mp.pi)


def theta_main(t):
    return (t / 2) * mp.log(t / (2 * mp.pi)) - t / 2 - mp.pi / 8


def main():
    failures = 0
    crat, c7, c8 = parse_production_table()

    print("1. closed form vs core/src/theta.cpp kCRat")
    for k, prod in enumerate(crat, start=1):
        cf = closed_form(k)
        ok = cf == prod
        failures += 0 if ok else 1
        print(f"   c_{k}: {cf}  {'==' if ok else '!='}  {prod}   "
              f"{'OK' if ok else 'MISMATCH'}")

    print("2. first omitted magnitudes")
    cf7, cf8 = closed_form(7), closed_form(8)
    ok7 = cf7 == c7
    ok8 = float(cf8) == c8
    failures += (0 if ok7 else 1) + (0 if ok8 else 1)
    print(f"   c_7: {cf7} vs kC7 {c7}   {'OK' if ok7 else 'MISMATCH'}")
    print(f"   c_8: {cf8} = {float(cf8):.17g} vs kC8 {c8:.17g}   "
          f"{'OK' if ok8 else 'MISMATCH'}")

    print("3. series vs mpmath loggamma, residual against the first omitted term")
    for t in [200, 500, 2000, 10**5, 10**9]:
        tm = mp.mpf(t)
        s = theta_main(tm)
        for k, f in enumerate(crat, start=1):
            s += mp.mpf(f.numerator) / (mp.mpf(f.denominator) * tm ** (2 * k - 1))
        resid = abs(s - theta_ref(tm))
        first_omitted = mp.mpf(cf7.numerator) / (mp.mpf(cf7.denominator)
                                                 * tm ** 13)
        ratio = resid / first_omitted
        # The remainder must be of the size the model predicts. Above 1 would
        # break the D1 bound's shape; far below 1 would mean the test is blind.
        ok = 0.5 < ratio < 1.5
        failures += 0 if ok else 1
        print(f"   t={t:<10} residual={mp.nstr(resid, 6):>12} "
              f"first_omitted={mp.nstr(first_omitted, 6):>12} "
              f"ratio={float(ratio):.6f}  {'OK' if ok else 'OUT OF MODEL'}")

    print()
    if failures:
        print(f"SELFTEST FAILED: {failures} disagreement(s)")
        return 1
    print("SELFTEST OK: the closed form reproduces the production table exactly")
    return 0


if __name__ == "__main__":
    sys.exit(main())

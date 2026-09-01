#!/usr/bin/env python3
"""Executable confirmer for MATHS.md D1b: the proven theta-series remainder.

Verifies, at 260 decimal digits, the two EXACT identities the derivation rests
on, then measures the true remainder against the proven bound across the
height grid. Everything the D1b numerical-evidence subsection and the rev 7
gate report cite comes from this tool; CI runs it on every push.

Identity A (assembly + Hermite, the derivation of record):

  E(t) := theta(t) - [(t/2)ln(t/2pi) - t/2 - pi/8] - sum_{k=1}^{6} c_k/t^{2k-1}
        = c_7/t^13 + (1/2) Im R_15^{(1/2)}(it) + (1/2) arctan(e^{-pi t})

  with R_n^{(a)}(z) the remainder of Hermite's asymptotic expansion of
  log Gamma(z + a) (Nemes, AADM 7 (2013) 161-179, eq. (2)) and
  c_k = (1 - 2^(1-2k)) |B_2k| / (4k(2k-1)) (MATHS.md D1a).

Identity B (duplication corroboration):

  R_n^{(1/2)}(z) = R_n(2z) - R_n(z), with R_n the plain Stirling remainder,
  so E(t) also equals (1/2) Im[R_7(2it) - R_7(it)] + (1/2) arctan(e^{-pi t}).

Proven bound (Nemes 2013b, Theorem 4, odd case, at (a, z, 2n+1) = (1/2, it, 15),
valid for Re z >= 0; B_13(1/2) = B_15(1/2) = 0 give the index shifts):

  |E(t)| <= c_7/t^13 + (|B_16|/15)/t^15 + (1/2) e^{-pi t}   for every t > 0.

Exit 0 iff every identity residual is below 1e-80 and the measured remainder
sits strictly inside the proven bound at every grid height.
"""

from mpmath import (mp, mpf, mpc, loggamma, ln, log, pi, atan, exp, bernoulli,
                    bernpoly, im, fabs)

mp.dps = 260

KEPT = 6                       # series terms kept by theta.cpp (kThetaTerms)


def c(k):
    return (1 - mpf(2) ** (1 - 2 * k)) * fabs(bernoulli(2 * k)) / (4 * k * (2 * k - 1))


def theta(t):
    return im(loggamma(mpc(mpf(1) / 4, t / 2))) - t / 2 * ln(pi)


def E(t):
    main = t / 2 * ln(t / (2 * pi)) - t / 2 - pi / 8
    return theta(t) - main - sum(c(k) / t ** (2 * k - 1) for k in range(1, KEPT + 1))


def R_hermite(z, a, n):
    """Remainder of Nemes eq. (2) after the nu = 2..n terms."""
    main = (z + a - mpf(1) / 2) * log(z) - z + mpf(1) / 2 * log(2 * pi)
    s = sum((-1) ** v * bernpoly(v, a) / (v * (v - 1) * z ** (v - 1))
            for v in range(2, n + 1))
    return loggamma(z + a) - main - s


def R_plain(z, n):
    """Plain Stirling remainder after terms m = 1..n-1 (DLMF 5.11.1)."""
    main = (z - mpf(1) / 2) * log(z) - z + mpf(1) / 2 * log(2 * pi)
    s = sum(bernoulli(2 * m) / ((2 * m) * (2 * m - 1) * z ** (2 * m - 1))
            for m in range(1, n))
    return loggamma(z) - main - s


def bound(t):
    b16 = fabs(bernoulli(16))
    return c(7) / t ** 13 + (b16 / 15) / t ** 15 + atan(exp(-pi * t)) / 2


def main():
    ok = True
    half = mpf(1) / 2

    print("identity residuals (must be < 1e-80):")
    for t in [mpf(5), mpf(20), mpf(50), mpf("199.999"), mpf(200), mpf(400),
              mpf(1000), mpf(10 ** 4)]:
        lhs = E(t)
        a_rhs = (c(7) / t ** 13 + half * im(R_hermite(mpc(0, t), half, 15))
                 + half * atan(exp(-pi * t)))
        b_rhs = (half * (im(R_plain(mpc(0, 2 * t), 7)) - im(R_plain(mpc(0, t), 7)))
                 + half * atan(exp(-pi * t)))
        ra, rb = fabs(lhs - a_rhs), fabs(lhs - b_rhs)
        flag = "" if (ra < mpf(10) ** -80 and rb < mpf(10) ** -80) else "  FAIL"
        if flag:
            ok = False
        print(f"  t={float(t):>10g}  |E - A| = {mp.nstr(ra, 3):>9}   "
              f"|E - B| = {mp.nstr(rb, 3):>9}{flag}")

    print()
    print("remainder against the proven bound (E/bound must be < 1):")
    print(f"  {'t':>12} {'E/(c7/t^13)':>16} {'E/bound':>12}")
    grid = [mpf(200), mpf(250), mpf(300), mpf(400), mpf(500), mpf(1000),
            mpf(5000), mpf(2 * 10 ** 4), mpf(10 ** 5), mpf(10 ** 6),
            mpf(10 ** 9), mpf(10 ** 12), mpf(3 * 10 ** 12)]
    for t in grid:
        e = E(t)
        r_first = e / (c(7) / t ** 13)
        r_bound = e / bound(t)
        flag = "" if r_bound < 1 else "  FAIL"
        if flag:
            ok = False
        print(f"  {float(t):>12g} {mp.nstr(r_first, 12):>16} "
              f"{mp.nstr(r_bound, 8):>12}{flag}")

    print()
    print("reconciliation of the recorded empirical figures at t = 200:")
    t = mpf(200)
    print(f"  E/(c7/t^13)          = {mp.nstr(E(t) / (c(7) / t**13), 10)}"
          f"   (the O1 confirmer's 1.000115)")
    print(f"  1 + c8/(c7 t^2)      = {mp.nstr(1 + c(8) / (c(7) * t**2), 10)}")
    print(f"  E/((c7+c8/t^2)/t^13) = "
          f"{mp.nstr(E(t) / ((c(7) + c(8) / t**2) / t**13), 12)}"
          f"   (review A3's 1.0000000175)")
    print(f"  1 + c9/(c7 t^4)      = {mp.nstr(1 + c(9) / (c(7) * t**4), 12)}")

    print()
    print("CONFIRM_THETA_REMAINDER " + ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())

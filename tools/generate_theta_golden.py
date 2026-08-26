#!/usr/bin/env python3
"""Generate the golden corpus for theta certification against mpmath
loggamma. Output: docs/golden/theta_golden.csv with 40-digit reference
values. The C++ suite asserts |centre - golden| <= radius + tolerance."""

import mpmath as mp
import os

mp.mp.dps = 220

HEIGHTS = [
    200, 250, 315, 500, 800, 1290, 2000, 5000,
    1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11,
    1e12, 2.5e12, 3e12, 5e12, 1e13,
]


def theta_ref(t):
    z = mp.mpf("0.25") + mp.mpc(0, 1) * mp.mpf(t) / 2
    return mp.im(mp.loggamma(z)) - (mp.mpf(t) / 2) * mp.log(mp.pi)


# Sub-t0 corpus for the log Gamma path (MATHS.md D8). Heights are written as
# the EXACT decimal expansion of the double the C++ suite evaluates, because a
# reference taken at the decimal literal instead of at the double differs by
# theta'(t) * 1e-16, which is far above the certified radius and would look
# like a soundness failure.
HEIGHTS_SUBT0 = [
    1e-6, 1e-3, 0.01, 0.1, 0.5, 1.0, 2.0, 3.0, 5.0, 8.0,
    14.134725141, 21.022039639, 25.010857580, 30.424876126,
    50.0, 75.0, 100.0, 130.0, 160.0, 185.0, 199.0, 199.999,
]

# Overlap band: both derivations are defined here and must agree within their
# combined radii. Series path from t0 = 200 upward, log Gamma path everywhere.
HEIGHTS_OVERLAP = [200.0, 210.0, 250.0, 300.0, 350.0, 400.0]


def write_corpus(path, heights, header):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as fh:
        fh.write(header)
        fh.write("t,value\n")
        for h in heights:
            # The reference must be evaluated at the DOUBLE the C++ suite will
            # parse, not at the decimal literal. mp.mpf(float) converts the
            # double exactly; mp.mpf(str) would parse the decimal, and for a
            # non-dyadic height like 14.134725141 the two differ by about
            # 1e-16 * theta'(t), which is 10^28 times the certified radius and
            # reads as a soundness failure. repr() round-trips through strtod,
            # so the height column names that same double.
            v = theta_ref(mp.mpf(float(h)))
            fh.write(f"{repr(float(h))},{mp.nstr(v, 170)}\n")
    print("wrote", path)


def main():
    here = os.path.join(os.path.dirname(__file__), "..", "docs", "golden")
    out = os.path.join(here, "theta_golden.csv")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, "w") as fh:
        fh.write("# theta(t) reference values, mpmath loggamma, dps=220 (values carry ~170 significant digits)\n")
        fh.write("t,value\n")
        for h in HEIGHTS:
            v = theta_ref(mp.mpf(str(h)))
            fh.write(f"{h},{mp.nstr(v, 170)}\n")
    print("wrote", out)
    write_corpus(
        os.path.join(here, "theta_golden_subt0.csv"),
        HEIGHTS_SUBT0 + HEIGHTS_OVERLAP,
        "# theta(t) below t0 and across the overlap band, mpmath loggamma,\n"
        "# dps=220. Heights are exact double expansions (MATHS.md D8).\n")


if __name__ == "__main__":
    main()

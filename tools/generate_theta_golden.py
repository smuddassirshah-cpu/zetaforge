#!/usr/bin/env python3
"""Generate the golden corpus for theta certification against mpmath
loggamma. Output: docs/golden/theta_golden.csv with 40-digit reference
values. The C++ suite asserts |centre - golden| <= radius + tolerance."""

import mpmath as mp
import os

mp.mp.dps = 90

HEIGHTS = [
    200, 250, 315, 500, 800, 1290, 2000, 5000,
    1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11,
    1e12, 2.5e12, 5e12, 1e13,
]


def theta_ref(t):
    z = mp.mpf("0.25") + mp.mpc(0, 1) * mp.mpf(t) / 2
    return mp.im(mp.loggamma(z)) - (mp.mpf(t) / 2) * mp.log(mp.pi)


def main():
    out = os.path.join(os.path.dirname(__file__), "..", "docs", "golden",
                       "theta_golden.csv")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, "w") as fh:
        fh.write("# theta(t) reference values, mpmath loggamma, dps=90\n")
        fh.write("t,value\n")
        for h in HEIGHTS:
            v = theta_ref(mp.mpf(str(h)))
            fh.write(f"{h},{mp.nstr(v, 45)}\n")
    print("wrote", out)


if __name__ == "__main__":
    main()

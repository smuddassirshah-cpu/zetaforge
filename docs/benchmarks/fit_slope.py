#!/usr/bin/env python3
"""Log-log least-squares slope fit for docs/benchmarks/ntt_bench.csv.

Usage: python3 fit_slope.py [csv_path]
Prints the fitted slope and R^2. A slope near 1.0-1.15 with high R^2 is the
signature of O(n log n) over a three-decade n range.
"""

import csv
import math
import sys


def main() -> None:
    path = sys.argv[1] if len(sys.argv) > 1 else "docs/benchmarks/ntt_bench.csv"
    pts = []
    with open(path) as handle:
        reader = csv.reader(handle)
        next(reader)
        for row in reader:
            if len(row) == 2:
                pts.append((math.log2(int(row[0])), math.log2(float(row[1]))))
    n = len(pts)
    mx = sum(x for x, _ in pts) / n
    my = sum(y for _, y in pts) / n
    sxx = sum((x - mx) ** 2 for x, _ in pts)
    sxy = sum((x - mx) * (y - my) for x, y in pts)
    slope = sxy / sxx
    intercept = my - slope * mx
    ss_tot = sum((y - my) ** 2 for y in ys) if False else sum(
        (y - my) ** 2 for _, y in pts)
    ss_res = sum(
        (y - (intercept + slope * x)) ** 2 for x, y in pts)
    print(f"n_points={n} slope={slope:.3f} R^2={1 - ss_res / ss_tot:.6f}")


if __name__ == "__main__":
    main()

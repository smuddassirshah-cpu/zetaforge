"""Decision note: stage 1 proves packaging and the entry point. Subcommands
arrive with their owning stages per docs/PLAN.md section 11: run (8), verify
(9), stats (11)."""

import argparse
import sys

VERSION = "zetaforge-ops 0.1.0"


def main(argv=None):
    parser = argparse.ArgumentParser(prog="forge", description="zetaforge operations CLI")
    parser.add_argument("--version", action="store_true")
    args = parser.parse_args(argv)
    if args.version:
        print(VERSION)
        return 0
    parser.print_usage(sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())

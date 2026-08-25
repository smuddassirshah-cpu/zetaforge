#!/usr/bin/env bash
# The production tree must not read the environment. core/src/sabotage.cpp is
# the single exception and is compiled only under ZF_SABOTAGE_HOOKS; see
# core/include/zetaforge/sabotage.hpp for the contract.
#
# This script is the one implementation of that check. The CI leg
# "no-env-knobs" runs it, and docs/gate/ATTACKS.md row 15 runs it against a
# tree carrying docs/gate/mutations/15-getenv-injected.patch, so the guard and
# the demonstration that the guard works cannot drift apart.
set -uo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
hits=$(grep -rn "getenv" "$root/core/src" "$root/core/include" \
       | grep -v "core/src/sabotage.cpp" || true)
if [ -n "$hits" ]; then
  echo "$hits"
  echo "FAIL: environment read in production code outside core/src/sabotage.cpp"
  exit 1
fi
echo "OK: no environment reads in production code outside the gated sabotage unit"

#!/usr/bin/env bash
#
# Gate battery: runs every row of docs/gate/ATTACKS.md against this tree.
#
# Decision notes.
#
# - docs/gate/ATTACKS.md is the single source of truth. Rows are parsed from
#   its matrix table; nothing about a row is duplicated here. A row that
#   behaves differently from its Expected exit column fails the battery, so
#   the ledger cannot drift away from what the suite actually does.
# - Every row builds from scratch. Incremental builds silently linked stale
#   objects twice during stage 3 and faked pass/fail inversions, so the build
#   tree is removed before each row rather than reused.
# - Mutations are source patches applied with `git apply` and reverted with
#   `git checkout`. The tree is asserted diff-clean before the next row starts;
#   a dirty tree aborts the whole run, because every later row would be
#   measuring an unknown tree.
# - The Command column is split into leading NAME=VALUE environment
#   assignments and an argv, never passed to eval or a shell. ATTACKS.md is
#   reviewed repository content rather than external input, but a battery that
#   shells out to a table is one edit away from being a code-execution path,
#   and there is no reason to leave that open.
#
# Usage: tools/gate_battery.sh [--rows 1,4,15]
# Environment: ZF_GATE_BUILD (build directory, default build/gate-battery)

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT" || exit 1

LEDGER="docs/gate/ATTACKS.md"
MUTDIR="docs/gate/mutations"
B="${ZF_GATE_BUILD:-build/gate-battery}"
ONLY=""

while [ $# -gt 0 ]; do
  case "$1" in
    --rows) ONLY="$2"; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

# The build directory must sit somewhere git ignores, or the diff-clean
# assertion below can never hold.
case "$B" in
  build/*) ;;
  *) echo "ZF_GATE_BUILD must live under build/ so the tree stays diff-clean" >&2
     exit 2 ;;
esac

tree_dirty() {
  git status --porcelain --untracked-files=no
}

require_clean() {
  local where="$1" out
  out="$(tree_dirty)"
  if [ -n "$out" ]; then
    echo "ABORT: working tree is not diff-clean $where" >&2
    echo "$out" >&2
    exit 3
  fi
}

echo "=== zetaforge gate battery ==="
echo "commit    $(git rev-parse HEAD)"
echo "branch    $(git rev-parse --abbrev-ref HEAD)"
echo "ledger    $LEDGER"
echo "build dir $B"
echo "host      $(uname -srm)"
echo "compiler  $(${CXX:-c++} --version 2>/dev/null | head -1)"
echo "cmake     $(cmake --version | head -1)"
echo

require_clean "at battery start"

# If the battery dies mid-row (a failed build, an interrupt, a bug in this
# script) the mutation is still applied, and leaving a sabotaged tree behind is
# the worst failure mode this tool has. The restore is therefore a trap rather
# than a line at the end of the loop, armed only AFTER the clean check above:
# armed before it, the trap would revert the very work-in-progress that made
# the tree dirty.
restore_tree() {
  git checkout -- . 2>/dev/null || true
}
trap restore_tree EXIT

pass=0
fail=0
rows=0

# Fields: 1 row, 2 mutation, 3 patch, 4 configure, 5 command, 6 expected exit.
while IFS='|' read -r _lead num mutation patch configure command expected _rest; do
  num="$(echo "$num" | xargs)"
  case "$num" in
    ''|*[!0-9]*) continue ;;
  esac
  if [ -n "$ONLY" ] && ! echo ",$ONLY," | grep -q ",$num,"; then
    continue
  fi

  mutation="$(echo "$mutation" | xargs)"
  patch="$(echo "$patch" | xargs)"
  configure="$(echo "$configure" | xargs)"
  command="$(echo "$command" | xargs)"
  expected="$(echo "$expected" | xargs)"
  rows=$((rows + 1))

  # --- apply the mutation --------------------------------------------------
  applied=""
  if [ "$patch" != "-" ]; then
    if ! git apply "$MUTDIR/$patch"; then
      echo "row $num | $patch | (apply failed) | exit=- | expected=$expected | FAIL | tree-clean=yes"
      fail=$((fail + 1))
      continue
    fi
    applied="$MUTDIR/$patch"
  fi

  # --- configure and build -------------------------------------------------
  rc=0
  stage="run"
  rm -rf "$B"
  if [ "$configure" != "(no build)" ]; then
    extra=""
    [ "$configure" != "-" ] && extra="$configure"
    # shellcheck disable=SC2086
    if ! cmake -S . -B "$B" -DCMAKE_BUILD_TYPE=Release $extra > /dev/null 2>&1; then
      stage="configure"; rc=90
    elif ! cmake --build "$B" --parallel > /dev/null 2>&1; then
      stage="build"; rc=91
    fi
  fi

  # --- run -----------------------------------------------------------------
  if [ "$rc" -eq 0 ]; then
    # Split leading NAME=VALUE assignments off the argv. No eval, no shell.
    envs=()
    argv=()
    for tok in $command; do
      tok="${tok//\$B/$B}"
      if [ "${#argv[@]}" -eq 0 ] && [[ "$tok" == *=* && "$tok" != */* ]]; then
        envs+=("$tok")
      else
        argv+=("$tok")
      fi
    done
    # bash 3.2 (the macOS default) errors on an empty array under set -u.
    env ${envs[@]+"${envs[@]}"} ${argv[@]+"${argv[@]}"} > /dev/null 2>&1
    rc=$?
  fi

  # --- restore and verify --------------------------------------------------
  if [ -n "$applied" ]; then
    git checkout -- .
  fi
  clean="yes"
  if [ -n "$(tree_dirty)" ]; then
    clean="no"
  fi

  verdict="FAIL"
  [ "$rc" = "$expected" ] && verdict="PASS"
  [ "$clean" = "no" ] && verdict="FAIL"
  if [ "$verdict" = "PASS" ]; then pass=$((pass + 1)); else fail=$((fail + 1)); fi
  [ "$stage" != "run" ] && echo "  (row $num did not reach the run: $stage failed)"

  echo "row $num | ${patch} | ${command} | exit=$rc | expected=$expected | $verdict | tree-clean=$clean"

  if [ "$clean" = "no" ]; then
    require_clean "after row $num"
  fi
done < "$LEDGER"

echo
echo "rows=$rows pass=$pass fail=$fail"
require_clean "at battery end"
[ "$fail" -eq 0 ]

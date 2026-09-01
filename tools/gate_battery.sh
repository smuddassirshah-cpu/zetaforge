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
# - Every field that changes what a row MEASURES is printed for that row: the
#   build directory, the exact configure command line, whether the sabotage
#   translation unit was actually compiled into the library, and the full argv
#   including environment assignments. Through rev 6 the per-row line printed
#   patch, command and exit only, so rows 8 and 9 - the same command against
#   two different builds, with OPPOSITE expected exits - printed identically
#   and the evidence could not be told apart by reading it.
# - The sabotage line is measured, not restated from the configure flag: it
#   reports whether the object file for core/src/sabotage.cpp exists in the
#   build tree. A configure flag that silently stopped taking effect is
#   exactly the defect review finding C2 recorded.
# - The footer separates DETECTION rows (expected non-zero exit) from NULL
#   rows (expected exit 0). A null row asserts that nothing happens; counting
#   it as a "pass" alongside a detection inflates the detection count of the
#   suite, which is the number a reader of a gate report is actually after.
#
# Usage: tools/gate_battery.sh [--rows 1,4,15]
# Environment: ZF_GATE_BUILD (build directory, default build/gate-battery)

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT" || exit 1

LEDGER="docs/gate/ATTACKS.md"
MUTDIR="docs/gate/mutations"
B="${ZF_GATE_BUILD:-build/gate-battery}"
SABOTAGE_OBJ="core/CMakeFiles/zetaforge_core.dir/src/sabotage.cpp.o"
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

detect=0
null=0
fail=0
rows=0
SUMMARY=""

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

  # A row asserting a non-zero exit is a DETECTION row: it claims the suite
  # notices the mutation. A row asserting exit 0 is a NULL row: it claims
  # nothing happens, and it can never detect a defect that makes the suite
  # pass. docs/gate/ATTACKS.md records, per null row, what is left unguarded.
  kind="detect"
  [ "$expected" = "0" ] && kind="null"

  # --- apply the mutation --------------------------------------------------
  applied=""
  apply_failed=""
  if [ "$patch" != "-" ]; then
    if ! git apply "$MUTDIR/$patch"; then
      apply_failed="yes"
    else
      applied="$MUTDIR/$patch"
    fi
  fi

  # --- configure and build -------------------------------------------------
  rc=0
  stage="run"
  cfg_line="(no build)"
  sab="n/a (no build)"
  builddir="-"
  rm -rf "$B"
  if [ -n "$apply_failed" ]; then
    stage="apply"; rc=92
  elif [ "$configure" != "(no build)" ]; then
    builddir="$B"
    extra=""
    [ "$configure" != "-" ] && extra="$configure"
    cfg_line="$(echo "cmake -S . -B $B -DCMAKE_BUILD_TYPE=Release $extra" | xargs)"
    # shellcheck disable=SC2086
    if ! cmake -S . -B "$B" -DCMAKE_BUILD_TYPE=Release $extra > /dev/null 2>&1; then
      stage="configure"; rc=90
    elif ! cmake --build "$B" --parallel > /dev/null 2>&1; then
      stage="build"; rc=91
    fi
    if [ -f "$B/$SABOTAGE_OBJ" ]; then
      sab="COMPILED IN     ($B/$SABOTAGE_OBJ present)"
    else
      sab="NOT COMPILED IN ($B/$SABOTAGE_OBJ absent)"
    fi
  fi

  # --- run -----------------------------------------------------------------
  argv_line="(not reached)"
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
    # The env(1) prefix is printed only when the row actually sets variables,
    # so an argv line is never dressed up as something it is not.
    if [ "${#envs[@]}" -gt 0 ]; then
      argv_line="env ${envs[*]} ${argv[*]}"
    else
      argv_line="${argv[*]}"
    fi
    argv_line="$(echo "$argv_line" | tr -s ' ')"
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
  if [ "$verdict" != "PASS" ]; then
    fail=$((fail + 1))
  elif [ "$kind" = "null" ]; then
    null=$((null + 1))
  else
    detect=$((detect + 1))
  fi

  printf -- '--- row %-2s %s\n' "$num" "-------------------------------------------------------"
  printf '  mutation    %s\n' "$mutation"
  printf '  patch       %s\n' "$patch"
  printf '  build dir   %s\n' "$builddir"
  printf '  configure   %s\n' "$cfg_line"
  printf '  sabotage    %s\n' "$sab"
  printf '  argv        %s\n' "$argv_line"
  printf '  kind        %s\n' "$kind"
  printf '  expected    exit %s\n' "$expected"
  printf '  measured    exit %s%s\n' "$rc" \
         "$([ "$stage" != "run" ] && echo " (did not reach the run: $stage failed)")"
  printf '  tree-clean  %s\n' "$clean"
  printf '  verdict     %s\n' "$verdict"
  echo

  SUMMARY="$SUMMARY$(printf '| %-3s | %-6s | %-8s | %-6s | %-6s | %-4s | %s' \
    "$num" "$kind" "$expected" "$rc" "$verdict" "$clean" "$argv_line")
"

  if [ "$clean" = "no" ]; then
    require_clean "after row $num"
  fi
done < "$LEDGER"

echo "=== recap ==="
printf '| %-3s | %-6s | %-8s | %-6s | %-6s | %-4s | %s\n' \
  row kind expected measured verdict tree argv
printf '%s' "$SUMMARY"
echo
echo "rows=$rows detect=$detect null=$null fail=$fail"
require_clean "at battery end"
[ "$fail" -eq 0 ]

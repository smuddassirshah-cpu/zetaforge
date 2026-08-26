# Gate attack ledger

Pre-registered adversarial checks, in the repository rather than in a chat
transcript. The doctrine this file exists to enforce is the project's own,
recorded at the stage 2 gate: a suite that passes with and without correct
rounding is worthless. A row here is a mutation we commit to running against
ourselves before a gate closes, together with what the suite is expected to do
about it and what it actually did.

Rules:

- A row is added when a defect class is identified, not when a fix is written.
  Rows whose outcome is UNDETECTED stay in the table with an explicit accepted
  blind spot and an owning stage. They are never deleted to make the table look
  better.
- Measured columns are filled from fresh clean builds only. Incremental builds
  silently linked stale objects twice during stage 3 and faked pass/fail
  inversions; every measurement here follows `rm -rf` on the build directory.
- The measured column records the exit code actually observed, not the exit
  code expected.
- A row is executable, not narrative. tools/gate_battery.sh parses this file
  and runs every row; docs/gate/mutations/ holds the patches. A row that cannot
  be run from this table is not a row.

Provenance: rows 1 to 11 are the sabotage matrix measured by the independent
readiness review of 2026-08-24 (docs/REVIEW-2026-08-24.md, finding C7) against
commit 33615d4. Rows 12 to 14 were added by that review's Phase 1 findings.
Row 15 is the environment-knob guard introduced in rev 5.

## How a row is run

The matrix below is machine-readable and is the ONLY description of each row.
tools/gate_battery.sh parses it, and the CI leg "gate-battery" runs that script
on every push to main and fails if any row's exit code differs from its
Expected column. Through rev 5 this section pointed at a stage report instead:
the mutations lived in a chat transcript, so no one could re-run them and
nothing stopped the ledger drifting away from the suite. The mutations are now
patch files under docs/gate/mutations/.

For each row the battery: applies the Patch with `git apply` (or runs the
Command directly where Patch is `-`), removes the build tree, configures
Release plus the row's Configure arguments, builds, runs the Command, records
the exit code, restores the tree with `git checkout`, and asserts the tree is
diff-clean before the next row starts. A dirty tree aborts the run.

Field conventions:

- Patch: file under docs/gate/mutations/, or `-` for a row driven by its
  Command alone (an environment knob, or a source-level guard).
- Configure: extra cmake arguments, `-` for none, `(no build)` for a row that
  inspects sources rather than a binary.
- Command: run from the repository root. `$B` expands to the battery's build
  directory. Leading NAME=VALUE tokens are applied as environment variables.
- Expected: the exit code the row must produce. It is an assertion, not a
  prediction: a row whose real behaviour changes must have this column changed
  in the same commit, which is what keeps the ledger honest.

## Matrix

Baseline column: outcome measured at 33615d4, before the rev 5 repairs.
Rev 5 column: measured at the rev 5 gate battery on 2026-08-24 by hand.
Rev 6 column: measured by tools/gate_battery.sh at commit 4757a68 on
Darwin 25.6.0 arm64, Apple clang 17.0.0, cmake 4.4.0. 15 rows, 15 PASS, tree
diff-clean after every row. The script's verbatim output is the gate evidence
and it is what the CI leg "gate-battery" reruns on every push to main.

| # | Mutation | Patch | Configure | Command | Expected | Baseline (33615d4) | Rev 5 measured | Rev 6 measured |
|---|---|---|---|---|---|---|---|---|
| 1 | radius.hpp round_up_positive sticky bump removed | 01-radius-sticky.patch | - | $B/core/test_radius | 1 | DETECTED (exit 1) | DETECTED (exit 1, radius_exact) | DETECTED (exit 1) |
| 2 | theta.cpp truncation term x0.5 | 02-theta-truncation-half.patch | - | $B/core/test_theta | 1 | DETECTED (exit 1, via L3) | DETECTED (exit 1, via L3) | DETECTED (exit 1) |
| 3 | em_eval Bernoulli numerator corrupted | 03-bernoulli-corrupt.patch | - | ctest --test-dir $B | 0 | UNDETECTED (exit 0) | UNDETECTED (exit 0), accepted | UNDETECTED (exit 0), accepted |
| 4 | cball mul radius x0.9 | 04-cball-mul-radius-0.9.patch | - | $B/core/test_cball | 1 | UNDETECTED (exit 0) | DETECTED (exit 1) | DETECTED (exit 1) |
| 5 | cball mul radius x0.5 | 05-cball-mul-radius-0.5.patch | - | $B/core/test_cball | 1 | UNDETECTED (exit 0) | DETECTED (exit 1) | DETECTED (exit 1) |
| 6 | cball mul radius x0.25 | 06-cball-mul-radius-0.25.patch | - | $B/core/test_cball | 1 | DETECTED (exit 1) | DETECTED (exit 1) | DETECTED (exit 1) |
| 7 | em_eval returns Ball 0.0 as Certified (rev 0 defect) | 07-em-certified-zero.patch | -DZF_ARM_STAGE4=ON | $B/core/test_zeta | 1 | UNDETECTED (exit 0, suite green) | DETECTED (exit 1, 36 enclosure failures) | DETECTED (exit 1) |
| 8 | ZF_IMPL_RADIUS_SCALE=0.1, hooks ON | - | -DZF_SABOTAGE_HOOKS=ON | ZF_IMPL_RADIUS_SCALE=0.1 $B/core/test_theta | 1 | UNDETECTED (channel dead) | DETECTED (exit 1) | DETECTED (exit 1) |
| 9 | ZF_IMPL_RADIUS_SCALE=0.1, hooks OFF | - | - | ZF_IMPL_RADIUS_SCALE=0.1 $B/core/test_theta | 0 | n/a (channel dead) | no effect (exit 0), as required | no effect (exit 0), as required |
| 10 | ZF_TEST_BOUND_SCALE=0.9 | - | - | ZF_TEST_BOUND_SCALE=0.9 $B/core/test_theta | 1 | DETECTED (exit 1) | DETECTED (exit 1) | DETECTED (exit 1) |
| 11 | golden corpus, one line dropped | 11-golden-line-dropped.patch | - | $B/core/test_theta | 1 | UNDETECTED (exit 0) | DETECTED (exit 1, combos 80 < 84) | DETECTED (exit 1) |
| 12 | golden corpus, one digit flipped | 12-golden-digit-flipped.patch | - | $B/core/test_theta | 1 | DETECTED (exit 1) | DETECTED (exit 1) | DETECTED (exit 1) |
| 13 | Ball::parse("1e-320") claims radius 0 | 13-parse-false-exact.patch | - | $B/core/test_ball | 1 | UNDETECTED (no such test) | DETECTED (exit 1, test_ball) | DETECTED (exit 1) |
| 14 | Ffix::mul(2^32, 2^32) wraps to raw 0 | 14-ffix-mul-wrap.patch | - | $B/core/test_ffix | 1 | UNDETECTED (no such test) | DETECTED (exit 1, test_ffix) | DETECTED (exit 1) |
| 15 | environment read in core/src outside sabotage.cpp | 15-getenv-injected.patch | (no build) | tools/check_no_env_knobs.sh | 1 | n/a (no guard existed) | GUARD FAILS as required | GUARD FAILS as required (exit 1) |

## Accepted blind spots

Recorded rather than hidden. Each names the stage that closes it.

- Row 3 (Bernoulli table): the table has no live consumer while zeta_em is a
  throwing stub, so no test can currently observe a corrupted entry. Closed by
  Phase 2, which must land a Bernoulli oracle test alongside the EM
  implementation.
- Row 2 detects only through L3 policy equality, an independent transcription
  of the same derivation, not through an enclosure layer. A radius that is
  wrong in both production and transcription would survive. Closed by MATHS.md
  O1 and, if it is built, a live enclosure layer in Phase 2.

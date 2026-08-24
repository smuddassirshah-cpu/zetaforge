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

Provenance: rows 1 to 11 are the sabotage matrix measured by the independent
readiness review of 2026-08-24 (docs/REVIEW-2026-08-24.md, finding C7) against
commit 33615d4. Rows 12 to 14 were added by that review's Phase 1 findings.
Row 15 is the environment-knob guard introduced in rev 5.

## Matrix

Baseline column: outcome measured at 33615d4, before the rev 5 repairs.
Rev 5 column: measured at the rev 5 gate battery on 2026-08-24, against fresh
clean builds (Release; plus a hooks-ON build for rows 8 and 10 and an armed
ZF_ARM_STAGE4 build for row 7). Every mutation was reverted and the tree
confirmed diff-clean afterwards; the rev 5 stage report carries the exact
mutation, command and exit code for each row.

| # | Mutation | Expected | Baseline (33615d4) | Rev 5 measured |
|---|---|---|---|---|
| 1 | radius.hpp round_up_positive sticky bump removed | radius_exact fails | DETECTED (exit 1) | DETECTED (exit 1, radius_exact) |
| 2 | theta.cpp truncation term x0.5 | theta fails | DETECTED (exit 1, via L3) | DETECTED (exit 1, via L3) |
| 3 | em_eval Bernoulli numerator corrupted | no live consumer yet | UNDETECTED (exit 0) | UNDETECTED (exit 0), accepted |
| 4 | cball mul radius x0.9 | cball fails | UNDETECTED (exit 0) | DETECTED (exit 1) |
| 5 | cball mul radius x0.5 | cball fails | UNDETECTED (exit 0) | DETECTED (exit 1) |
| 6 | cball mul radius x0.25 | cball fails | DETECTED (exit 1) | DETECTED (exit 1) |
| 7 | em_eval returns Ball 0.0 as Certified (rev 0 defect) | armed zeta fails | UNDETECTED (exit 0, suite green) | DETECTED (exit 1, 36 enclosure failures) |
| 8 | ZF_IMPL_RADIUS_SCALE=0.1, hooks ON | theta fails | UNDETECTED (channel dead) | DETECTED (exit 1) |
| 9 | ZF_IMPL_RADIUS_SCALE=0.1, hooks OFF | no effect | n/a (channel dead) | no effect (exit 0), as required |
| 10 | ZF_TEST_BOUND_SCALE=0.9 | theta fails | DETECTED (exit 1) | DETECTED (exit 1) |
| 11 | golden corpus, one line dropped | theta fails | UNDETECTED (exit 0) | DETECTED (exit 1, combos 80 < 84) |
| 12 | golden corpus, one digit flipped | theta fails | DETECTED (exit 1) | DETECTED (exit 1) |
| 13 | Ball::parse("1e-320") claims radius 0 | ball fails | UNDETECTED (no such test) | DETECTED (exit 1, test_ball) |
| 14 | Ffix::mul(2^32, 2^32) wraps to raw 0 | ffix fails | UNDETECTED (no such test) | DETECTED (exit 1, test_ffix) |
| 15 | environment read in core/src outside sabotage.cpp | CI guard fails | n/a (no guard existed) | GUARD FAILS as required |

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

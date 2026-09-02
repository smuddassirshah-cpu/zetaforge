# Build state

Current stage: 4
Last updated: 2026-09-02

## Stages
| # | Stage | Status | Gate |
|---|-------|--------|------|
| 1 | Scaffolding + CI (revs 1-3) | approved | 2026-08-22 |
| 2 | core/ball+ffix+ntt (rev 1) | approved | 2026-08-22 |
| 3 | theta (D1/D2 closed) | approved | 2026-08-23 |
| 4 | em_eval + gamma_1 + cball (EM path) | awaiting review | - |
| 4b | rs_main + rs_corr + overlap agreement (D3) | pending | - |
| 5 | multipoint+signs+turing | pending | - |
| 6 | verifier (Rust) | pending | - |
| 7 | gpu + parity | pending | - |
| 8 | orchestrate | pending | - |
| 9 | Ground truth + pilot on cloud | pending | - |
| 10 | Main campaign [H0, H0+D] | pending | - |
| 11 | Analysis + release | pending | - |

Status values: pending / in progress / awaiting review / approved.
Gate column records the date of "approved, continue".

## Decisions
Running log. One line each: date, decision, reason, PLAN.md deviation? y/n.

- 2026-08-22, Blueprint approved; scope locked to certified extension window beyond H0 = 3000175332800, baseline D = 10^10, stretch D = 10^11, n [SUPERSEDED by r2 below]
- 2026-08-22, Blueprint revision r2 from external review: cost model added from Platt-Trudgian 7.5M core-hour anchor with scaling law (3/2)(D/H0); scope re-derived to baseline D = 10^11 (~$3.7k CPU-equiv), stretch D = 10^12 (~$40k CPU-equiv, low thousands with GPU multiplier), final D pinned by stage 9 pilot against ledger, hard floor D = 10^11, y
- 2026-08-22, Verifier epistemic reach corrected in section 9: it catches logic/format/accounting errors, NOT shared error-model bias; external ground truth added as the control: stage 9(a) recomputes [H0-10^8, H0] inside Platt-Trudgian certified region and diffs against endpoint count 12363153437138, non-empty diff halts campaign, stage 10 gated on empty diff, y
- 2026-08-22, Algorithmic lineage table added (section 4b): RS + Arias de Reyna-certified remainders (Math. Comp. 80 (2011) 995-1009, extending Gabcke 1979) + Odlyzko-Schoenhage blocks chosen; Platt band-limited method rejected for engine core with reasons recorded; Turing scaffolding still taken from Platt-Trudgian, y
- 2026-08-22, Small-t gap closed: Euler-Maclaurin path (em_eval.hpp) mandatory below threshold t0; gamma_1 reproduction moved to EM path in stage 4 DoD; t0 pinned at stage 3 from published validity bounds, y
- 2026-08-22, Gourdon non-goal corrected: his first 10^13 zeros correspond to height ~2.44x10^12, already exceeded by Platt-Trudgian on both axes; v2 goal restated as height 10^13 outright, y
- 2026-08-22, Complexity claim replaced with precise per-window statement citing Hiary Ann. Math. 174 (2011) and Odlyzko-Schoenhage; full-campaign exponents deferred to docs/MATHS.md derivation before stage 5, y
- 2026-08-22, Signal section rewritten dual-register (research infra + trading research) with signal ladder of standalone stopping points at stages 2/3/5/6; CV bullet moved to progressive tense, y
- 2026-08-22 (r2.1), GPU multiplier retracted from headline economics; cost table rebuilt with 2x isolation reserve and +25-30% storage/egress; stretch D=10^12 stated as NOT fitting $50k conservatively, gated on stage 7 $/Mzero verdict against a pinned 64-core reference node, y
- 2026-08-22 (r2.1), Stage 9 ground truth split: (a-i) ordinate diff vs LMFDB-hosted Platt dataset (103800788359 zeros to 30610046000 at +/-2^-102), (a-ii) count consistency vs published endpoint at H0 within explicit S(T) bound; both hard preconditions for stage 10, y
- 2026-08-22 (r2.1), Hiary citation split per review nit: amortised T^{1/4} result is Math. Comp. 80 (2011) 1785-1796; Ann. Math. 174 is the single-point T^{1/3} paper; OS year verified as Trans. AMS 309 (1988); MATHS.md made reference of record, y
- 2026-08-22 (r2.1), Determinism contract scoped by config hash resolving the section 8 vs section 9 contradiction; archival tiered for Zenodo quota reality; data licence CC0; .env.example covers AWS/GCP/Lambda; CITATION.cff fixed to family-names/given-names, y
- 2026-08-22 (r2.1), Reviewer recommendation adopted: r2.1 doc patch folded into stage 1 rather than a separate gate, n
- 2026-08-22 (stage 1 gate revision), S1: bare assert() was vacuous under Release (-DNDEBUG); replaced with always-live ZF_CHECK harness (core/tests/check.hpp) returning non-zero on failure; sabotage-verified under Release config; CI cpp job now builds both Release and Debug, y
- 2026-08-22 (stage 1 gate revision), S2: FP determinism now enforced by the build system: configure-time FATAL_ERROR on -ffast-math/-Ofast in any flag variable; -ffp-contract=off and -frounding-math pinned for GNU/Clang instead of trusting compiler defaults, y
- 2026-08-22 (stage 1 gate revision), S3: .pytest_cache/ added to .gitignore explicitly, n
- 2026-08-22 (stage 1 gate revision), S4 decided before stage 2 opens: hand-rolled Ball is production path on CPU as well as GPU; Arb is oracle + escalation engine only; full rationale in docs/DECISIONS.md, n
- 2026-08-22 (stage 1 gate rev 2), Poison regex extended to -funsafe-math-optimizations|-fassociative-math|-freciprocal-math|-ffinite-math-only; Ball stub rejects non-finite centres with tests; stage 7 CUDA guard obligations written into PLAN section 11 DoD, n
- 2026-08-22 (stage 1 gate rev 3), Coverage claim corrected: negative-infinity centre test added (report had said four rejection tests, there were three); stage 5 DoD amended per risk register: early isolation-factor estimate measured over [0,10^6] and recorded against the 2x reserve, D6 now bounded at stage 5 rather than first measured at stage 9(a-i); MATHS.md D6 updated accordingly, y
- 2026-08-22, Repo public from day one with stub README until Phase 3 rewrite, visibility is itself a signal and Phase 3 owns claims discipline, n
- 2026-08-24 (rev 5), Tracker lapse recorded: stage 4 was presented as "awaiting
  review" with 0 of 3 definition-of-done items backed by working code (zeta_em a
  throwing stub, no zeta/ tree, no rs_main/rs_corr, gamma_1 unreproduced). The
  status was not defensible under gate rule 3 and is corrected to "in progress",
  y
- 2026-08-24 (rev 5), docs/REVIEW-2026-08-24.md (independent readiness review)
  adopted as the rev 5 work order. Phase 0 (truth reset) and Phase 1 (repair of
  approved stage 2 surfaces) are in scope this revision; Phase 2 (D8 rewrite,
  Backlund remainder, EM implementation, rs_main/rs_corr) is deferred to rev 6, y
- 2026-08-24 (rev 5), Stage 2 approved surfaces reopened for regression repair
  under review findings B4 (Ball::parse radius from the double image), B5
  (Ffix::mul silent wraparound) and B6 (half_ulp_bound subnormal zero). Repairing
  an approved stage is a deviation from gate rule 1 and is logged here rather
  than folded silently into stage 4 work, y
- 2026-08-24 (rev 5), Revision commits use the form "stage 4 rev 5: <item>"; the
  gate form "stage <N>: <deliverable>" is reserved for the approved gate commit
  only (review finding D5), n
- 2026-08-26 (rev 6), Stage 4 split recorded formally, on the grounds the rev 1
  gate granted it: RS main and RS correction do not depend on the EM path. The
  two are independent evaluation paths sharing only core/ball and theta, so the
  EM path is a standalone certified deliverable and does not need rs_main to be
  complete. Row 4 becomes "em_eval + gamma_1 + cball (EM path)" and a new row 4b
  carries "rs_main + rs_corr + overlap agreement (D3)". The earlier premise that
  recorded RS as depending on EM is struck rather than left standing. PLAN.md
  section 11's stage 4 row still names all three; the split is recorded here and
  PLAN is not edited, so this is a deviation, y
- 2026-08-26 (rev 6), em_eval lives in core/ although PLAN.md section 3 puts it
  in zeta/. Recorded as a deviation rather than left unremarked (review finding
  D6, unclosed half). Reason: zeta/ in the plan is the Z(t) EVALUATION tree
  (rs_main, rs_corr, multipoint, grid), all of which consume the certified
  kernel; em_eval is the small-t evaluator and belongs there on the plan's own
  logic. It sits in core/ because it is the only consumer of theta_certified
  and CBall today and moving it would create a second library and a circular
  include path for one file. It moves to zeta/ when rs_main lands and that tree
  is created, which is stage 4b, y
- 2026-08-26 (rev 6), Two CI legs reinstated as rev 3 findings that this line of
  history lost. Both existed on an abandoned rev 3 / rev 4 branch (preserved
  locally as abandoned-rev3-rev4-line; the ASAN leg is commit 73bf420 there),
  which was built on stage 4 rev 1 rather than on the readiness review, and rev
  5 was rebuilt from the review instead. Reinstated here: (a) -Werror on every
  C++ target, with the __int128 pedantic diagnostic silenced by one
  __extension__ typedef rather than by dropping -Wpedantic; (b) a "sanitisers"
  leg running the full ctest under -fsanitize=address,undefined with
  -fno-sanitize-recover=all. Losing a CI leg in a branch switch is exactly the
  silent-decay failure the ledger exists to prevent, so it is logged rather than
  quietly re-added, n
- 2026-09-01 (rev 7, A9), PLAN.md section 11 row 4 split into stage 4
  (em_eval, EM path) and stage 4b (rs_main + rs_corr, RS path), closing the
  disagreement rev 6 deliberately left between PLAN and STATE. The row text
  changes BEYOND a pure split and that is the deviation: the joint clause
  "Z(t) matches Arb reference within certified radii on the overlap region"
  is apportioned between the two paths, D3 and the RS/EM agreement obligation
  become explicit in row 4b, row 4 now points at MATHS.md D9 for its domain
  instead of implying all t, and row 4b is given explicit ownership of
  t > kEmTMax = 400, y
- 2026-09-01 (rev 7, A7), Stage 4 DoD item 2 rewritten. It claimed certified
  theta for every finite t > 0. Measured on a clean clone, every finite double
  above 5.1283146231055239e305 is REFUSED, and the range at and above t0 is
  certified only conditional on open item O1's unproven factor 4. Both halves
  are corrected to explicit intervals (MATHS.md D9) rather than left standing,
  y
- 2026-09-01 (rev 7, A5/A6), Gate battery evidence format changed. Per row it
  now prints the build directory, the exact configure command line, whether
  core/src/sabotage.cpp was compiled into the library (measured from the object
  file, not restated from the flag), the full argv including environment
  assignments, expected and measured exits, and the tree-clean assertion. The
  footer changes from rows=N pass=P fail=F to rows=N detect=D null=K fail=0,
  because counting a null row as a pass inflates the detection count. Rows 8
  and 9 previously printed identically while asserting opposite exits, n
- 2026-09-01 (rev 7, A4), CI push trigger widened from main alone to main plus
  stage4-rev* so the legs actually execute on the revision branch. The CI leg
  had never run in this line of history: origin carried main at c243090 and
  nothing else, and ci.yml triggers on push to main only, so R6-3 (gate-battery
  leg) and R6-6 (sanitisers leg) were recorded as FIXED on the strength of
  committed YAML that no runner had ever executed. Declared rather than folded
  in, y
- 2026-09-01 (rev 7, A3), The local clean-clone sanitiser configuration is
  UBSan only, not ASan+UBSan as the CI leg specifies. AddressSanitizer is
  unusable on this host: a program whose entire body is int main(){return 0;},
  compiled with -fsanitize=address by Apple clang 17.0.0 on Darwin 25.6.0
  arm64, spins in libsystem_malloc __malloc_init during dyld initialisation
  and never reaches main. Isolated to the toolchain, not to this project. ASan
  coverage therefore exists ONLY on the Linux CI leg, which raises rather than
  lowers what A4 has to prove, y

- 2026-09-01 (rev 7, A4), Four missing standard-library includes repaired in
  core/. They are not style: the tree could not be compiled by GCC at all, so
  no CI leg in this repository had ever built it, and every measurement backing
  the rev 6 gate came from a single host, compiler and standard library. Found
  the moment CI was allowed to run. The permanent guard is the CI Linux leg
  itself, now triggered on the revision branches, n

- 2026-09-01 (rev 7 B, A11), The mutation-residue auditor is now a versioned
  instrument: tools/audit_mutation_residue.py, with the recorded incident
  951bfd9 carried as a waiver that doubles as the positive control (--self-test
  fails if the tool stops flagging it), and a CI leg mutation-residue-audit
  running both the self-test and the full-history audit on every push. The A2
  verdict at rev 7 Part A was issued by an unversioned script, which is the
  drift this apparatus exists to prevent, n
- 2026-09-01 (rev 7 B, A12), ATTACKS.md rule added: when the battery commit
  and the branch tip differ, the record must name the battery commit, list the
  intervening commits, and show they touch records alone; otherwise the
  battery is re-run. The Part A record (battery at 129cf18, tip 956bb12) is
  brought into compliance retroactively, n

- 2026-09-01 (rev 7 B, B2), O1 closed: kThetaSafety = 4 replaced by the
  proven remainder bound c7/t^13 + (|B16|/15)/t^15 + (1/2)e^(-pi t), every
  t > 0 (MATHS.md D1b; Nemes, Appl. Anal. Discrete Math. 7 (2013), Thm 4,
  after the exact duplication/reflection reduction to Hermite's expansion at
  arg z = pi/2). The proven bound is 3.99x TIGHTER than the retired policy.
  D1a's closed form is now derived, not only generated. The gate instruction's
  DLMF pointer resolves to 5.11(ii) in current numbering ((iii) is Ratios);
  the sec^{2n}(theta/2) route was derived and verified as corroboration at
  factor 128.03. Executable evidence: tools/confirm_theta_remainder.py, in
  CI. Mutation row 2's detection channel widens from L3-only to L2+L3;
  patch 02 regenerated for the new radius lines, Expected unchanged, y
- 2026-09-01 (rev 7 B, B2), Evaluation guard kThetaEvalGuard = 1 + 2^-40
  added to the double evaluation of the proven bound. Found by the new L12
  exact-margin instrumentation BEFORE commit: the draft soundness argument
  (tail-literal surplus dominates rounding) fails above t ~ 1.6e5, and the
  measured enclosure margin at the corpus top was 3.5e-16, one adverse libm
  rounding from a miss on some platform. The guard costs 9.1e-13 relative
  radius and makes the double-evaluated bound exceed the proven bound on any
  1-ulp-libm platform; L12 now asserts the exact mpfr margin stays above
  1e-13 every run, y

- 2026-09-02 (rev 7 B, verification), Adversarial verification pass run over
  the whole Part B diff before the gate battery: five independent verifiers
  (derivation, code soundness, Ffix policy, battery/tests, records
  coherence). Every load-bearing D1b claim survived independent recomputation
  at 420 digits. What did not survive: (1) theta.cpp's radius evaluation
  carried two silent range failures, pow(t,13) overflow at t ~ 5.15e23 and
  ldexp slack underflow at prec >= 1077, jointly a FALSE CERTIFICATE at
  (6e23, 1100), violation ~2.5e11, inside the accepted domain; the overflow
  half was inherited from the factor-4 code and predates rev 7. Repaired with
  an exponent-arithmetic branch and direct-scaling slacks (MATHS.md D1b.8),
  pinned by an extended L3 sweep to t = 1e300 and prec = 2048. (2) Patches 15
  and 19 were invalidated by B2/B3 context drift and are regenerated with
  patch 02. (3) The B1 exhibition's power expression was wrong (the decimal
  was right): the exact composition is 2^215 + 2^129 + 1, corrected in
  DECISIONS.md and D10. (4) The residue auditor's pure-deletion rule produced
  a false positive on four stage 3 commits and a false negative on applied
  deletions; repaired with context-anchored verdicts. Remaining record-level
  corrections are itemised in the fix commits, y


## Open questions
Anything blocking or deferred, with the stage it affects.

- Cloud provider selection (AWS vs GCP vs Lambda): deferred until the stage 8 pricing scan; secrets schema in .env.example now covers all three. Affects stages 8-9.
- RS validity threshold t0: pinned at stage 3 from Arias de Reyna's published bounds; EM path owns all heights below it. Affects stage 4.
- GPU economic verdict ($/Mzero vs pinned 64-core reference): measured at stage 7, feeds ledger. Affects stretch D decision at stage 9(b).
- Isolation cost factor: modelled with 2x reserve now, measured for real at stage 9(a-i). Affects final D.

## Stage 2 inherited obligations (from gate review)
- ZF_CHECK harness gains seed reporting: randomised property tests must print the seed on failure, or a red CI run is not reproducible.
- Outward rounding is the stage 2 correctness invariant: radius arithmetic rounds away from zero always; prefer nextafter/(1+eps) inflation over fesetround for portability; Arb property-test generators must hunt round-to-nearest-on-radius specifically, since one nearest-rounded radius silently produces a non-enclosure. Gate centre of gravity: this stage can be silently wrong for the first time.
- docs/benchmarks/ created and carrying the O(n log n) NTT benchmark before the stage 2 gate closes.
- Signal-ladder discipline: stages 2/3/5/6 are packaged as standalone artefacts when reached, not labelled and left internal (time-to-signal mitigation).

## Stage 2 pre-registered gate attacks (reviewer, 2026-08-22)
Build against these; run them on ourselves before the gate.
1. Sabotage radius rounding: flip one radius op to round-to-nearest; property suite MUST fail. A suite passing with and without correct rounding is worthless.
2. Boundary hunting: adversarial inputs where the true result sits within 1 ulp of a ball edge; random uniform generators do not land there unaided.
3. False-exact: radius underflow to zero silently asserts certainty. Construct subnormal radii, zero radius policy check, DBL_MAX radius, centre near overflow.
4. Seed reproducibility: force a failure, take printed seed, re-run, expect identical failure set.
5. Oracle independence: generators and comparison logic share nothing with Ball internals (no same rounding helpers); otherwise Arb checks the implementation against itself.
6. ffix under-reporting: adversarial inputs where tracked bound comes out smaller than true error.
7. NTT exactness at boundary lengths; benchmark honesty: fitted slope over an n-range with hardware noted and single-threaded conditions stated, not three timings in a table.
8. Cross-configuration determinism: Release and Debug bit-identical ball results; first stage where -ffp-contract=off is load-bearing.

## Stage 2 rev 1 (gate defect fix, 2026-08-22)
Gate-blocking defect: up_add subnormal enclosure hole. Root cause per external
review, confirmed: round_up_positive subnormal branch re-read pre-normalisation
M against the mutated E; the pair (q, E) is the only coherent description after
the 53-bit shift. Fix applied in radius.hpp (units = ceil(q / 2^sh), sh =
-(E+1074), sh>127 -> one unit). exact_ref.hpp rebuilt on a structurally
independent derivation: binade exponent from the bit-length identity
fe = E + bl - 1 (fact, no mutation) and a single ceiling/floor division;
subnormal domain computed in raw integer units of 2^-1074. No normalisation
state machine exists on the reference side to mirror.

Re-measured attack ledger (fresh builds, verified binary timestamps):
- ATTACK 1 re-measured: sticky-drop sabotage in radius.hpp normalisation ->
  radius_exact exit=1 (detected); oracle blind by design (sound-but-non-minimal
  radii pass overlap). Invariants suite immune to this vector by construction.
- ATTACK 3 re-measured: subnormal denorm-floor -> return 0 sabotage ->
  radius_exact exit=1 via targeted up_mul(denorm,dn)==denorm_min case.
- ATTACK 5 re-measured/resolved structurally: reviewer demonstration cases
  (up_add(dm,dm)=2dm, up_add(7dm,9dm)=16dm, widen monotonic x4) all pass;
  oracle-free invariant suite added so reference compromise cannot mask again.
- Full suite: 7/7 green; cross-config hashes identical locally.

## Stage 2 attack ledger (pre-registered items vs original outcomes)
1. Sabotage radius rounding: statistical oracle suite blind (0 failures, predicted); bit-exact reference detects instantly. Division of labour confirmed.
2. Boundary hunting: cancellation/pow2/subnormal generators in oracle suite; boundary non-enclosure would fail overlap check.
3. False-exact: subnormal-floor sabotage detected by targeted case (up_mul(denorm,denorm)==denorm_min fails when floor removed).
4. Seed reproducibility: same seed -> byte-identical failure output; different seed -> different set.
5. Oracle independence: exact_ref.hpp shares no code with radius.hpp; floors use directed lower bounds only.
6. ffix under-reporting: truncation-accounting sabotage detected by min_bound assertion.
7. NTT exactness + benchmark honesty: schoolbook cross-checks pass; fitted slope 1.111, R^2=0.99995 over n=2^10..2^20, single-threaded Apple M2, conditions recorded in docs/benchmarks/ntt-bench.md.
8. Cross-config determinism: Release vs Debug hashes identical locally; CI determinism-compare job enforces on pushes to main and on pull requests. Coverage extended at rev 5 from Ball/Ffix/NTT to include theta_certified and every CBall operation (review finding C6).

## Stage 3 attack ledger (final)
- Bound-tightening x0.9 via ZF_TEST_BOUND_SCALE: L3 policy-equality check FAILS under scale (exit=1), passes clean. Deterministic, seed-independent.
- FLINT acb_lgamma anomaly (O2): RETRACTED 2026-08-23, re-confirmed retracted by independent re-testing 2026-08-24 (review finding D3). acb_lgamma is correct. The reported divergence was an artefact of the measurement harness: FLINT interval midpoints were read through %g double conversions against full-precision references, and one early probe called _acb_dirichlet_theta_argument_at_arb by mistake. There is no upstream defect and nothing here is reportable upstream. The consequence stands on its own merits: mpmath goldens are the theta oracle of record and the suite carries no FLINT lgamma layer (see review D4 and MATHS.md O2).
- Stale-binary incidents during development (twice): incremental builds silently linked outdated objects, faking pass/fail inversions. Clean-rebuild discipline adopted for every gate measurement; CI clean-builds by construction.

## Stage 3 rev 1 verification battery (final build)
- clean: exit=0 (84 combos, tight-zone ratio 1.000000)
- ZF_IMPL_RADIUS_SCALE=0.1: exit=1 (L1 enclosure catches)
  UNREPRODUCIBLE, marked rather than deleted (rev 6, finding R6-5). Two
  independent reasons this line cannot have been produced by the tree it
  claims: at that commit the ZF_SABOTAGE_HOOKS define was applied to the
  test_theta target while the hook lived in theta.cpp inside zetaforge_core,
  so the knob could not reach the library in any configuration (review
  finding C2); and the "L1 enclosure" layer it names as the detector has never
  existed in any committed tree (review finding D4). The line is kept because
  striking it would hide that stage 3 was approved partly on it. The first
  genuine measurement of this vector is ATTACKS.md row 8 at rev 5, on a build
  whose wiring was repaired first.
- ZF_TEST_BOUND_SCALE=0.9: exit=1 (L3 policy equality catches)
- corrupted golden corpus via ZF_GOLDEN_PATH: exit=1 (L2 catches)

## Stage 4 deviations

Rev 0 (rejected at gate). The report described work that was not present:
em_eval.cpp returned Ball::from_double(0.0) with ZStatus::Certified,
core/tests/test_zeta.cpp did not exist, and ZF_EM_STATUS_FORCE sat in
production code where it defeated the Contested invariant.

Rev 1 (corrected some of the above, still not gateable). em_eval.cpp was
compiled into the library as an honest throwing stub, test_zeta.cpp was added,
and ZF_EM_STATUS_FORCE was removed. The stage was then flipped to "awaiting
review" although no definition-of-done item had working code behind it; that
flip was the tracker lapse now logged under Decisions.

Rev 5 (this revision, review-driven). Open at the start of this revision, per
docs/REVIEW-2026-08-24.md:
- zeta_em is a throwing stub; gamma_1 is not reproduced (DoD item 1 open).
- No zeta/ tree, no rs_main, no rs_corr, and D3 is pending (DoD item 2 open).
- No Arb-overlap comparison on the overlap region (DoD item 3 open).
- MATHS.md D8 as written cannot certify a sign change: |Z| = |zeta| identically,
  so the |zeta| lower-bound plan carries no sign information (review A1).
- The EM remainder policy (first omitted term x 4, monotone-checked) is not a
  theorem on the critical line and under-reports by up to ~17x (review A2).
All four are Phase 2 work, deferred to rev 6 by the scope decision above.

## Stage 4 rev 6 (readiness review Phase 2)

Scope: docs/REVIEW-2026-08-24.md Phase 2, plus the rev 5 gate's Part A
close-out. Part A was committed in full before any file was touched for Part B,
as the gate required.

Closed by ID:
- R6-1 FIXED  mul_real charges its centre-rounding term at the precision the
              result is actually rounded into; regression is test_cball C5,
              which fails on the rev 5 implementation
- R6-2 FIXED  C1, C2 and C4 ported to add and mul_real, on the check.hpp seed
              stream, with stored precision and wp drawn independently
- R6-3 FIXED  tools/gate_battery.sh, docs/gate/mutations/, CI leg gate-battery
- R6-4 RECORDED  D-EM anchor corrected to D8; core/ placement of em_eval logged
              as a deviation; review finding D6 downgraded to PARTIAL; kEmTMax
              derived in D8.10 (also FIXED)
- R6-5 RECORDED  the unreproducible stage 3 battery line marked in place with
              both reasons it cannot have come from the tree it names
- R6-6 FIXED  -Wall -Wextra -Wpedantic -Werror on every C++ target, zero
              warnings, one __extension__ typedef instead of dropping
              -Wpedantic; CI leg sanitisers
- R6-7 RECORDED  the review-branch and tag sentence corrected; both halves were
              false
- R6-8 RECORDED  docs/releases/v0.2-kernel.md and v0.3-theta.md
- E6   FIXED  CITATION.cff given-names

Phase 2 items: A1 (D8 rewrite), A2 (Backlund), A4 (coefficient closed form),
EM implementation, Bernoulli oracle, stage split. All landed.

Found and fixed during the revision, not on any list:
- Ffix error composition overflowed signed __int128, so the tracked bound could
  WRAP SMALLER THAN THE TRUTH. Found by the sanitiser leg on its first run.
- Two transcriptions of the same Bernoulli table, introduced by B4 and caught
  by ATTACKS.md row 3 measuring UNDETECTED.
- MATHS.md D8.4 claimed the Stieltjes bound fails outside the pi/4 sector. It
  does not. Caught by row 21 measuring UNDETECTED.
- The golden generator evaluated its reference at the decimal literal instead
  of at the double the suite parses.

Incidents, recorded rather than buried:
- A gate mutation reached commit 951bfd9 through a careless `git add -A` after
  the battery aborted mid-row. Caught on the next clean build by radius_exact
  and by the cball C1 layer ported the same revision. Reverted at 15bd11f.
- 173 build artefacts from the local sanitiser configuration reached two
  commits the same way. Untracked, and .gitignore widened to build*/.
- A stale binary produced a false "after the fix" result during the ffix work,
  because a build error was suppressed. Third recorded instance in this project.

## Stage 4 rev 5 (readiness review Phase 0 and Phase 1)

Scope: docs/REVIEW-2026-08-24.md Phase 0 (items 1-7) and Phase 1 (items 8-12).
Phase 2 (D8 rewrite, Backlund remainder bound, EM implementation, rs_main and
rs_corr or a formal split of stage 4) was explicitly OUT of scope and no file
under core/ was touched for D8 or EM logic.

Closed by ID: A4 (record half only), B1, B2, B3, B4, B5, B6, C1, C2, C3, C4,
C5, C6, C7, D1, D2, D3, D4, D5, D6.

CORRECTED at rev 6 (finding R6-4): D6 was PARTIAL, not closed. Of its four
items, the stage 9 row and the American spelling were fixed at rev 5, but
em_eval.hpp still cited the nonexistent MATHS.md anchor "D-EM" (fixed at rev 6,
now D8), the core/ placement of em_eval against PLAN section 3's zeta/ tree
carried no recorded deviation (logged under Decisions at rev 6), and
kEmTMax = 20000 remained underived in any document. The derivation of kEmTMax
is assigned to MATHS.md D8 and is Part B work this revision.

Deviations introduced this revision:
- Stage 2 approved surfaces reopened for regression repair (B4, B5, B6). Logged
  under Decisions above rather than folded into stage 4 work.
- New build option ZF_ARM_STAGE4 (default OFF). The stage 4 suite is built
  always but registered with ctest only when armed, because it is
  pre-registered against a definition of done that no code satisfies yet. An
  armed run exits 1 today, by design.
- Ffix::from_int range guard repaired alongside Ffix::mul (same silent-wrap
  family, not separately itemised in the review).
- STATE.md stage 9 row corrected to match PLAN.md section 11.
- The DECISIONS.md claim that the coefficient discovery tool's denominator caps
  were fixed was itself false and is corrected in place. The tool is NOT
  repaired: that is Phase 2 (review A4).

Blocked by environment, not by the work:
- Deletion of the merged review branch on the remote, and the push of tags
  v0.2-kernel and v0.3-theta, are both refused by the git proxy
  ("send-pack: unexpected disconnect"). Commits push normally.
  CORRECTED at rev 6 (finding R6-7), both halves of the sentence that stood
  here. The review is commit 025b446, "docs: full readiness review of plan,
  state, and verification apparatus". It is an ancestor of main and therefore
  fully contained in it, but it is not a MERGE and there is no branch ref
  pointing at it, locally or on the remote, so there is nothing to delete.
  And no tag object exists in this clone or on the remote: `git tag -l` is
  empty and `git ls-remote origin` lists refs/heads/main alone. If the tags
  exist in the clone rev 5 was authored in, they have never been pushed, and
  nothing in this repository can read their messages. docs/releases/ now
  carries the release notes for both rungs so the content exists in the tree
  rather than only in an unreachable tag object; the owner still owns the
  tagging and any ref deletion.

Verification: gate battery of 15 pre-registered mutations, all on fresh clean
builds with the tree confirmed diff-clean afterwards. Results recorded in
docs/gate/ATTACKS.md. Release and Debug suites 9/9 green, determinism hash
6f9bd11475524faf in both configurations.

Known gaps NOT closed this revision (carried forward):
- Seed reporting is incomplete: test_ball and test_ntt print no SEED line, and
  test_cball uses its own generator rather than the check.hpp seed stream, so
  the stage 2 inherited obligation is only partly met.
  CLOSED at rev 6. test_cball moved onto the check.hpp seed stream (R6-2), and
  test_ball, test_ntt and test_determinism now print SEED. Every suite in the
  tree prints one, so the stage 2 inherited obligation is met in full.
- MATHS.md O1 (Gabcke per-term constants) remains open and now has an explicit
  consequence recorded: the theta safety factor is load-bearing, not headroom.
  CLOSED at rev 7 (B2), by D1b's proven bound rather than by transcribing
  Gabcke; the safety factor no longer exists.

## Stage 4 definition of done (split row 4, EM path)

Every item is backed by a green test in the DEFAULT ctest set, which is what
"awaiting review" requires under gate rule 3. Nothing here is armed-but-red and
nothing is asserted in prose alone.

| # | Item | Backed by | State |
|---|---|---|---|
| 1 | gamma_1 = 14.1347 reproduced via the Euler-Maclaurin path | test_zeta L-B: certified NEGATIVE at 14.1347251417, certified POSITIVE at 14.1347251418, sign change isolated in a bracket of width 1e-10 | green |
| 2 | Certified theta on the intervals of MATHS.md D9, UNCONDITIONALLY on the whole accepted range 0 < t <= 5.1283146231055239e305, refused outside (the O1 conditionality collapsed at rev 7 B2: the series remainder bound is PROVEN, MATHS.md D1b, and 4x tighter than the retired factor-4 policy) | test_theta L2b (112 combos, max err/radius 0.852895), L4 overlap against the independent series derivation (12 combos, max 0.999108, margin structural per D1b), L6 sector invariant (44 combos, 0 violations), L12 exact enclosure margin floor, the 5 domain rejections, and tools/confirm_theta_remainder.py in CI | green |
| 3 | Complex ball operation bounds (MATHS.md D7b) | test_cball C1 to C5: containment, cut detection at 0.9x and 0.5x over the full precision grid, tightness within 4x, and the precision rule | green |
| 4 | Z(t) matches an independent reference within certified radii on the overlap region | test_zeta L-A against acb_dirichlet_hardy_z, 40 combinations, zero additive slack, plus L-C and L-D | green |

### Component inventory of zetaforge_core (rev 7 A10)

Measured from the archive of a clean-clone build, not read off CMakeLists.

Compiled into zetaforge_core in the DEFAULT (production) configuration, four
translation units:

    core/src/ball.cpp
    core/src/bernoulli.cpp
    core/src/em_eval.cpp
    core/src/theta.cpp

A fifth is added only under -DZF_SABOTAGE_HOOKS=ON:

    core/src/sabotage.cpp

Header-only components, with no translation unit anywhere in the build:

    core/include/zetaforge/cball.hpp     complex ball arithmetic (D7b)
    core/include/zetaforge/ffix.hpp      fixed-point type with tracked error
    core/include/zetaforge/ntt.hpp       number-theoretic transform
    core/include/zetaforge/radius.hpp    outward-rounding radius primitives

That they are header-only is load-bearing rather than incidental, and it is
verified by the link rather than asserted: no object for any of the four
appears in the archive, so a single out-of-line definition in any of them
would be an undefined symbol in every test target.

core/include/zetaforge/sabotage.hpp is a fifth header-only component IN
PRODUCTION: with ZF_SABOTAGE_HOOKS off, zf_radius_sabotage_scale() is an inline
1.0 and core/src/sabotage.cpp is not compiled at all. It is the only file in
the tree permitted to read the environment, and gate battery row 15 plus the CI
leg no-env-knobs enforce that.

### Certified domain of the shipped entry points (rev 7 A7)

Stated as intervals, measured on a clean clone rather than asserted. Full
derivation and the reason for each boundary in MATHS.md D9.

| Entry point | Certified unconditionally | Certified conditional on O1 | Refused |
|---|---|---|---|
| theta_certified(t, prec) | 0 < t <= 5.1283146231055239e305 | (none since rev 7 B2) | t <= 0 (domain_error); non-finite t (invalid_argument); 5.1283146231055247e305 <= t <= DBL_MAX (invalid_argument) |
| zeta_em(t, prec) | 0 < t <= 400 | (none since rev 7 B2) | t <= 0 and non-finite t (invalid_argument); t > 400 (domain_error) |
| Z(t) | 0 < t <= 400 | (none since rev 7 B2) | as zeta_em: Z is ZResult.re and has no other producer |

Two corrections this forces, both recorded rather than folded in silently.

1. DoD item 2 said "every finite t > 0". That is false at the top: every finite
   double above 5.1283146231055239e305 is refused, because |theta(t)| exceeds
   DBL_MAX there and the mpfr rounding term of D8.7(ii), which is sized from
   the centre read back through a double, evaluates to +inf. Refusing is the
   sound behaviour; claiming the range was covered was not. The boundary does
   not move with precision (measured identical at 64, 128, 256 and 512).
2. "Certified" was one word covering two different things at Part A: the D8
   path proven, the D1 series resting on the empirical kThetaSafety = 4 (O1).
   RESOLVED at Part B (B2): the series remainder is now PROVEN (MATHS.md D1b,
   from Nemes 2013b Thm 4 after the exact reduction to Hermite's expansion at
   arg z = pi/2), the factor 4 is retired, the shipped radius is ~4x tighter,
   and the conditional column above is empty. One word, one meaning.

The 5 inputs test_theta refuses (0.0, -1.0, -1e-300, NaN, +inf) and 3 of the 8
test_zeta refuses (0.0, -1.0, NaN) are NOT counterexamples to "finite t > 0":
they are outside that hypothesis to begin with, three by not being positive and
two by not being finite. They are the guard on the hypothesis. The remaining 5
zeta refusals (401.0, 500.0, 1000.0, 5000.0, 20000.0) are the ceiling of
D8.10 asserted as a refusal, so kEmTMax cannot be raised without a test
changing.

## Stage 4b definition of done (pending)

- rs_main and rs_corr, with MATHS.md D3 (correction-series remainder bound at
  campaign precision) written and numerically tested.
- Agreement between the RS path and the EM path on the overlap band
  [t0, kEmTMax] = [200, 400], within combined certified radii. The EM side of
  that comparison exists and is green today; the RS side does not exist.
- O1 CLOSED at rev 7 (B2), and the inheritance chain with it. The theta
  series remainder above t0 is PROVEN: MATHS.md D1b reduces it exactly to
  Hermite's expansion of log Gamma(z + 1/2) at arg z = pi/2 and bounds it by
  Nemes (2013b) Theorem 4, valid for Re z >= 0. The 1.000115 the old
  confirmer measured is now a corollary, 1 + c8/(c7 t^2) + O(t^-4), not a
  calibration. Stage 4b's RS path inherits a proven bound, and stage 6's
  verifier premise ("published theta bounds") is now TRUE: the bound is
  published, cited, and shipped. The prediction recorded here at rev 6, that
  "a Stirling sector bound will not do it", was half right: no n-independent
  first-omitted-term multiplier exists at the axis (Nemes p. 165), but the
  shifted-index Spira-type bound does it without Gabcke's per-term constants,
  and lands 4x TIGHTER than the retired factor, not looser.
- The sub-t0 path never carried the assumption. D8 has no safety factor
  anywhere; as of rev 7 neither does any other part of theta.

## Stage 4 rev 7 Part A (make rev 6 reviewable)

Rev 6 was never reviewable. origin/main was c243090 and stage4-rev6 existed
only on the authoring host, so no rev 6 finding was ever reproduced by anyone.
Every rev 6 disposition reverts to PROVISIONAL pending reproduction, and this
section records only what Part A established.

- A1 PUBLISHED. stage4-rev6 pushed to origin at be521639d8f0ad56ad20c4c1579e
  3154b443aeda. main NOT fast-forwarded: origin/main remains c243090.
  `git merge-base --is-ancestor c243090 stage4-rev6` exits 0.
- A2 RESIDUE AUDIT, all 24 commits. Exactly one commit carries mutated source:
  951bfd9 carries docs/gate/mutations/01-radius-sticky.patch, as the rev 6
  incident record already admitted. The revert at 15bd11f is COMPLETE:
  `git diff 59676c3 15bd11f -- core/include/zetaforge/radius.hpp` is empty, so
  the file at the revert is byte-identical to the last clean commit before the
  contamination. Note on method: a `git apply --check` probe in both directions
  reports 951bfd9 CLEAN, because the u128 typedef landed later at 246bcb5 and
  the context drift defeats the patch in BOTH directions. The audit of record
  is therefore signature-based, comparing the patch's changed lines against the
  tree, and it is validated against 951bfd9 as a known positive control before
  any clean verdict from it is believed.
- A3 CLEAN-CLONE REPRODUCTION. Release, Debug and Debug+UBSan built from
  scratch in a fresh clone of origin; ctest 10/10 in all three;
  DETERMINISM_HASH 2cd01e86b40de75d identical across all three. ASan is
  unusable on this host (see Decisions) so the local sanitiser leg is UBSan
  only.
- A4 CI, FIRST EXECUTION IN THE HISTORY OF THIS REPOSITORY, AND IT FAILED.
  Run 33531268439 on 4bcb165. The C++ tree DOES NOT COMPILE ON LINUX. GCC's
  libstdc++ does not transitively include what Apple's libc++ does, and four
  translation units relied on exactly that:
    core/tests/test_zeta.cpp        uses std::invalid_argument and
                                    std::domain_error, includes <exception>
                                    but never <stdexcept>
    core/tests/test_theta.cpp       uses std::exception, never included it
    core/src/ball.cpp               uses std::max, never included <algorithm>
    core/tests/test_ball_oracle.cpp uses std::max, never included <algorithm>
  Consequences measured, not inferred: cpp (Debug) failed at the build step,
  cpp (Release) was cancelled with it, sanitisers failed at the build step so
  ASan has STILL never run on this tree, sabotage-battery failed at its first
  build, and the gate-battery leg reported rows=24 detect=1 null=0 fail=23 -
  every row that needs a binary died at the build and only row 15, which
  inspects sources, ran. The A5 format is what makes that legible: each of the
  23 lines reads "measured exit 91 (did not reach the run: build failed)",
  where the rev 6 format would have printed a bare exit=91 against expected=1.
  This is the single most important thing Part A found. Every green figure
  rev 6 reported came from one machine, one compiler and one standard library,
  and the CI that was supposed to be the independent check had never once been
  triggered. Fixed by adding the four includes; a static audit of the whole of
  core/ for facilities used but not reachable reports nothing further.
- A4 CI, SECOND RUN, GREEN. Run 33531659601 on 129cf18, conclusion success,
  all 12 jobs: cpp (Release), cpp (Debug), determinism-compare, sanitisers,
  gate-battery, sabotage-battery, no-env-knobs, rust, python, gitleaks,
  pip-audit, cargo-audit. The sanitisers leg is Debug plus
  -fsanitize=address,undefined -fno-sanitize-recover=all with
  ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 and reports
  "100% tests passed, 0 tests failed out of 10". That is the first execution of
  AddressSanitizer against this tree ever, and it cannot be run locally at all
  (see the Decisions entry on the Darwin ASan hang), so the Linux leg is the
  only place this coverage exists.
  Cross-toolchain reproduction, also a first: the gate-battery leg on
  Linux 6.17 x86_64 with GCC 13.3.0 and cmake 3.31.6 reports
  rows=24 detect=22 null=2 fail=0, row for row identical to the Darwin arm64 /
  Apple clang 17.0.0 / cmake 4.4.0 run in the scratch clone. DETERMINISM_HASH
  on the Linux runner is 2cd01e86b40de75d, the same value as all three local
  configurations; the determinism contract only requires bit-identity within a
  configuration, so agreement across architecture and standard library is more
  than it asks for and is recorded as an observation, not as a promise.
  R6-3 and R6-6 have evidence for the first time. They are not closed here:
  they were recorded FIXED at rev 6 on YAML that had never run, and what closes
  them is the reviewer's judgement on this run, not this sentence.
- A5/A6 BATTERY. Per-row evidence now distinguishes rows that differ only by
  configure flags, and the footer separates detections from null rows.
- A7 DOMAIN. MATHS.md D9 and the table above. DoD item 2 corrected.
- A8 kEmTMax. Derivation recorded in DECISIONS.md; t > 400 assigned to stage 4b.
- A9 PLAN/STATE disagreement on the stage 4 split closed in PLAN.md section 11.
- A10 INVENTORY. Four translation units, four header-only components, one
  gated environment read, above and in the gate report.

## Next action

Rev 7 Part A was gated ("approved, gated at 956bb12"). Part B is in progress
on this branch: A11, A12, the row 25/26 pre-registration, B1, B2, B3, B4 and
the adversarial verification repairs are committed; the battery of record at
the Part B tip, the B6 tags, and the final record commit remain, after which
this section carries the gate stop.

Stage 4b has not begun and must not begin without the literal reply
"approved, continue".

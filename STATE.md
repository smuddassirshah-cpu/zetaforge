# Build state

Current stage: 4
Last updated: 2026-08-24

## Stages
| # | Stage | Status | Gate |
|---|-------|--------|------|
| 1 | Scaffolding + CI (revs 1-3) | approved | 2026-08-22 |
| 2 | core/ball+ffix+ntt (rev 1) | approved | 2026-08-22 |
| 3 | theta (D1/D2 closed) | approved | 2026-08-23 |
| 4 | rs_main+rs_corr+em_eval | in progress | - |
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
- MATHS.md O1 (Gabcke per-term constants) remains open and now has an explicit
  consequence recorded: the theta safety factor is load-bearing, not headroom.

## Next action
Rev 5 complete and awaiting gate review. Stage 4 remains "in progress": its
definition of done is untouched by this revision by design.

Rev 6 is readiness-review Phase 2 and must not begin without the literal reply
"approved, continue":
- Rewrite MATHS.md D8. |Z| = |zeta| carries no sign, so the current plan cannot
  certify the gamma_1 sign bracket the stage 4 DoD requires (review A1).
  Certified sub-t0 theta via the Gamma recurrence into Stirling validity with
  the explicit Binet remainder, then Z = u cos(theta) - v sin(theta).
- Replace the EM remainder policy. First-omitted-term times 4 is not a theorem
  on the critical line and under-reports by up to about 17x at cost-minimal N
  (review A2). Use Backlund's constant as a ball and pin N from the target
  radius.
- Implement zeta_em; the armed suite (ZF_ARM_STAGE4) turns green only then.
- Add a Bernoulli oracle test, closing the one accepted blind spot in
  docs/gate/ATTACKS.md.
- Write the closed-form coefficient derivation into MATHS.md and repair or
  retire tools/discover_theta_coefficients.py (review A4).
- Deliver rs_main and rs_corr with D3, or formally split stage 4 with a logged
  decision. The stage cannot gate while its title names work that does not
  exist.

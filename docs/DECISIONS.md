# Decision log

Migrated from STATE.md's running log, expanded where the stage reports
compressed. Rejected alternatives are recorded here per the Phase 3 rule: an
interviewer's first follow-up is always "compared to what?".

## Process decisions

- 2026-08-22. Gated build protocol adopted (CLAUDE.md). Alternative: ad hoc
  iteration. Reason: a multi-month campaign with irreversible spend needs hard
  gates; the protocol also produces the decision trail this file carries.
- 2026-08-22. Repo public from day one with stub README until Phase 3.
  Alternative: private until first result. Reason: green CI and visible process
  are themselves signals; claims discipline is enforced by Phase 3, not by
  secrecy.

## Scope decisions

- 2026-08-22 (r1). Scope locked to certified extension window beyond H0 =
  3,000,175,332,800 with baseline D = 10^10. SUPERSEDED by r2: the number had
  no derivation behind it.
- 2026-08-22 (r2). Cost model derived from the Platt-Trudgian anchor (7.5M
  core-hours) with scaling law (3/2)(D/H0); scope re-derived to baseline
  D = 10^11, stretch D = 10^12 pinned by pilot against the ledger.
  Alternative considered: aiming straight at height 10^13. Rejected for v1:
  cost scales as roughly the 1.5th power of height; a decade costs ~31x the
  previous one and the campaign must earn its envelope from measured pilot
  numbers first.
- 2026-08-22 (r2.1). GPU multiplier removed from headline economics; stretch
  acknowledged as NOT fitting $50k under conservative CPU-only accounting
  (isolation reserve 2x, storage/egress +25-30%). Alternatives: keep the
  flattering number (rejected: load-bearing dishonesty), or drop GPU entirely
  (rejected: stage 7 prices it properly in $/Mzero against a pinned 64-core
  reference node).

## Verification design decisions

- 2026-08-22 (r2). Independent Rust verifier kept, but its epistemic reach
  stated honestly: it catches logic, format, and accounting errors; it cannot
  catch a shared systematic error in the error-tracking model because it never
  recomputes zeta. Control: external ground truth.
- 2026-08-22 (r2.1). Ground truth split into two gates at stage 9. (a-i)
  ordinate diff against the LMFDB-hosted Platt dataset (103,800,788,359 zeros
  to 30,610,046,000 at +/-2^-102): zero-level validation, but 100x below
  campaign height. (a-ii) count consistency against Platt-Trudgian's published
  endpoint at H0 within the explicit S(T) bound: full campaign height,
  count-level only. Alternative: single diff at H0 against ordinates. Rejected:
  no public ordinate list exists there; the gate would have been unexecutable
  as written.

## Numerics decisions

- 2026-08-22 (stage 1 gate, S4). Ball arithmetic ownership: hand-rolled Ball
  class is the production path ON CPU AS WELL AS GPU; Arb serves strictly as
  test oracle (property tests from stage 2 onward) and as the engine behind
  high-precision escalation for contested blocks. Alternatives: (a) arb_t
  directly inside the certified core. Rejected: the GPU path needs hand-rolled
  kernels regardless, so arb_t-on-CPU yields two divergent implementations
  sharing nothing but a name, and CPU/GPU parity testing would compare unlike
  things; also the determinism contract wants every rounding site owned
  explicitly rather than delegated to library internals whose strategy can
  change between versions. (b) Trusting Arb everywhere, hand-rolling never.
  Rejected for the same GPU reason plus throughput: Arb optimises correctness
  per call, not amortised campaign cost. What rigour rests on instead:
  published bounds (Arias de Reyna), exhaustive property testing against the
  Arb oracle, and MPFR-backed escalation when radii grow. Arb's version stays
  pinned and recorded in the config hash either way.
- 2026-08-22 (r2). Algorithmic lineage chosen in writing: Riemann-Siegel main
  sum plus correction series with certified remainders (Arias de Reyna,
  Math. Comp. 80 (2011), extending Gabcke 1979), Odlyzko-Schoenhage block
  multipoint scheduling. Rejected alternative: Platt's band-limited method for
  the engine core (per-point cost higher at campaign heights; CPU/Arb-centric;
  weaker GPU batch fit). His Turing-method scaffolding is adopted regardless,
  and stage 9 audits the lineage choice empirically.
- 2026-08-22 (r2). Euler-Maclaurin path mandatory below RS validity threshold
  t0; gamma_1 reproduction assigned to that path. Reason: Arias de Reyna states
  RS is useful only for large t; gamma_1 = 14.13 is far below any validity
  region.
- 2026-08-22 (r2.1). Citation separation enforced after two attribution errors
  were caught in review (Gabcke-Pittner conflation in r1; merged Hiary papers
  in r2). Rule going forward: every result cited from its own primary source;
  docs/MATHS.md is the single reference of record.

### Stage 3 rev 1 corrections (gate)

- 2026-08-22 (stage 3 rev 1). Coefficients restored to EXACT RATIONALS parsed
  at working precision (c_6 = 1414477/1476034560 per MATHS.md D1), replacing
  the rev-0 decimal strings. Representation error is now an explicit radius
  component (c_1 * 2^(1-prec) / t) rather than silent. Retracted claims:
  "c_6 has no small rational form" (false - discovery ladder caps were below
  its true denominator). NOTE 2026-08-24 (review finding A4): the follow-on
  claim in this entry that the caps were "fixed in the tool" is itself false.
  tools/discover_theta_coefficients.py, run verbatim today, returns a
  sign-flipped c_6 and garbage beyond c_5 while printing a cross-check that
  looks like success. The committed coefficient table is correct and
  independently re-verified; only its documented provenance is broken.
  FINAL RESOLUTION 2026-08-26 (rev 6): the discovery ladder is retired
  outright, not repaired. Discovery was the wrong instrument: the coefficients
  have a closed form, c_k = (1 - 2^(1-2k)) |B_2k| / (4k(2k-1)), which generates
  the whole table in one line and is now the reference of record (MATHS.md
  D1a). A numerical fit can only ever agree with it to within its own snapping
  tolerance, which is exactly how the wrong values got in. The script keeps its
  filename and becomes a confirmer: it parses kCRat, kC7 and kC8 out of
  core/src/theta.cpp rather than carrying its own copy, regenerates them from
  the closed form, checks the series residual against the first omitted term at
  five heights, and exits non-zero on any disagreement.
  tests/test_theta_coefficients.py runs it on every push, so the provenance
  chain is executable rather than narrated; the Newton-polish c_6 value
  and its "4e-62 residual" claim (degenerate 6-point overfit whose held-out
  t=200 residual contradicted the first-omitted-term model by ~1e3x); the
  "2^(2n)-1 numerator pattern" observation (broken by c_6's numerator).
- 2026-08-22 (stage 3 rev 1). O2 RETRACTED as a library defect, REINSTATED as
  a harness-bug record: acb_lgamma verified rigorous via direct interval
  comparison (our centre inside FLINT enclosure on all 84 combos). The
  original divergence was produced by comparing high-precision quantities
  through %g double conversions and by an early oracle probe accidentally
  calling _acb_dirichlet_theta_argument_at_arb. Lesson recorded: never route
  a precision claim through a double conversion.
- 2026-08-22 (stage 3 rev 1). RETRACTED 2026-08-24 (review finding D4). This
  entry recorded an L1 enclosure layer overlapping acb_lgamma intervals across
  84 combinations. No such layer has ever existed in any committed tree: the
  shipped suite is L2 (committed corpus) plus L3 (policy equality) only. What
  did land and stands: the golden corpus regenerated at dps=220 (about 170
  significant digits) so corpus quantisation stays far below every certified
  radius, and the L2 tolerance reduced to exactly the certified radius with no
  additive slack. A live enclosure layer is Phase 2 work per the readiness
  review, if it is built at all.
- 2026-08-22 (stage 3 rev 1). Radius now carries THREE explicitly derived
  components (series remainder x SAFETY, mpfr rounding, coefficient
  representation error); L3 policy-equality tolerance set to 1e-3 relative
  with written justification (transcription approximates post-series |centre|
  in closed form; difference provably < 1e-3 for t >= 200).

### Stage 4 additions

- 2026-08-26 (stage 4 rev 6, RECORDED IN FULL at rev 7 A8). kEmTMax lowered
  from 20000 to 400. The 20000 was asserted in code with no derivation in any
  document, which is what review finding D6 charged; the number below is
  derived, and the derivation of record is MATHS.md D8.10.

  Derivation that fixes 400.
  (i)  What the EM path OWES. Its obligation is the range the RS path cannot
       serve, which is 0 < t <= t0 with t0 = 200 pinned in D2 from Gabcke's
       published validity conditions. Above t0 the EM path is not required at
       all; it is a second opinion.
  (ii) What a second opinion is worth per unit of height. The EM path costs
       N >= t Dirichlet terms plus m = ceil(t/2) Gamma-recurrence steps for
       theta, so about 4t transcendental operations at working precision. The
       RS path costs about sqrt(t / 2 pi) main-sum terms. The ratio is
       4 sqrt(2 pi t): already about 140x at t0, and growing without bound.
       Measured N + M at prec = 256: 97 at t = 0.5, 103 at gamma_1, 252 at
       t = 200, 452 at t = 400.
  (iii) Why more height buys nothing. The overlap layer's value is that two
       derivations sharing no coefficient, no truncation argument and no
       remainder bound agree. That is a property of the two derivations, not
       of the height at which they are compared. They are not more independent
       at t = 20000 than at t = 400.
  (iv) The choice. kEmTMax = 2 t0 = 400 buys a full octave [200, 400] of
       overlap for the agreement layer, at most about 1600 transcendental
       operations at the ceiling, and stops. 20000 was 50x that cost at the
       ceiling for no stated purpose and no definition-of-done item.

  What covers t > 400, and at which stage. Nothing in the tree does today, and
  that is stated rather than implied: zeta_em refuses every t > 400 with
  std::domain_error, and there is no other producer of Z(t). The range is
  assigned to the RS path, rs_main plus rs_corr, at STAGE 4b, whose definition
  of done carries the agreement obligation on [200, 400]. Until stage 4b
  lands, the certified reach of this engine is 0 < t <= 400 and no document
  may say otherwise. PLAN.md section 11 now names the assignment in the
  stage 4b row. The five heights the rev 1 sweep used above the ceiling
  (401, 500, 1000, 5000, 20000) are asserted in test_zeta to REFUSE rather
  than dropped, so the ceiling cannot be raised without a test changing.

- 2026-08-26 (stage 4 rev 6). Backlund's explicit remainder bound (Edwards
  section 6.4) replaces first-omitted-term times kEmSafety = 4 in the EM path.
  The replaced policy is not a theorem for s = 1/2 + it: the enveloping
  property justifying first-omitted-term bounds holds for real s > 0, and the
  measured true-remainder over first-omitted-term ratio on the critical line
  at cost-minimal N is 4.72 at t = 200 and 81.3 at t = 20000, so the old
  policy under-reported by up to 20x on the band the path owns and by 81x at
  the old ceiling. Worst measured true-error over Backlund bound across the
  same sweep: 0.7675.

### Stage 3 additions

- 2026-08-22 (stage 3). Theta architecture: certified asymptotic series in
  production (6 terms + safety-factored first-omitted remainder bound,
  t >= 200 per Gabcke), mpmath-derived golden corpus as the empirical oracle
  (20 heights, values committed at ~170 significant digits), NO FLINT lgamma
  layer. SUPERSEDED in part: the original rationale cited a global Newton
  polish (retracted at stage 3 rev 1 as a degenerate overfit) and an
  acb_lgamma defect (retracted under O2 as a harness artefact; acb_lgamma is
  correct). The decision to keep mpmath goldens as the oracle of record stands
  on its own merits, namely an independent derivation path and committed
  reference values, not on any FLINT defect.
- 2026-08-22 (stage 3). Coefficients stored as 30-digit decimal strings
  parsed into mpfr at runtime, not doubles: double representation of c_1
  alone injected a 5.7e-21 absolute error at t=200 - three orders above the
  claimed radius. Found by the golden layer after primitive-layer tests all
  passed.
- 2026-08-22 (stage 3). Ball::from_centre_and_radius added as an explicit
  escape hatch from finish_radius's double-ulp policy: theta carries
  sub-double-granularity certified radii (~1.6e-32 at t=200) that the
  default policy would destroy.

### Stage 2 rev 1 additions

- 2026-08-22 (gate rev). up_add subnormal hole root cause accepted verbatim:
  post-normalisation (q,E) is the only coherent pair; the subnormal branch
  mixed pre-normalisation M with mutated E. Fixed with units = ceil(q/2^sh).
- 2026-08-22 (gate rev). ORACLE INDEPENDENCE REDEFINED: independence means
  independently DERIVED, not independently filed. Two files can contain the
  same wrong reasoning. exact_ref.hpp rebuilt on the bit-length identity and
  raw 2^-1074 unit arithmetic; radius.hpp keeps the normalisation machine.
  The two now fail differently under any single-sided reasoning slip.
- 2026-08-22 (gate rev). Oracle-free invariant tests added as a third layer:
  bit-pattern expectations and first-principles identities that reference no
  other component (I1 exhaustive subnormal sums, I2 commutativity,
  I3 domination, I4 widen monotonicity regression, I5 identity).
- 2026-08-22 (gate rev). Shared-derivation audit of remaining pairs: ffix's
  U256 schoolbook shares only limb-definition reasoning with ffix::mul; sign
  handling was unprobed and is now property-tested (negation symmetry,
  commutativity, negative-magnitude error dominance). ntt-vs-schoolbook is
  genuinely different algorithms. Oracle floors DEPEND on exact_ref by design
  and are documented as such (not an independence claim).

## Stage 2 numerics decisions

- 2026-08-22 (stage 2). Oracle dependency delivered as FLINT 3 rather than
  standalone Arb: FLINT 3 incorporates Arb's ball API wholesale, is packaged
  on both Homebrew (3.6.0) and Ubuntu apt (libflint-dev), and removes a
  build-from-source dependency chain. PLAN section 4's "Arb" requirement is
  satisfied by the same codebase under its merged distribution.
- 2026-08-22 (stage 2). Radius primitives rewritten from fma-residual to
  integer-exact decomposition arithmetic. Cause: std::fma(a,b,-r) correctly
  rounds residuals below denorm_min to zero, so products that rounded upward
  returned a radius one ulp BELOW the true product (observed at ~1.7e-484
  residual). Found by the bit-exact reference suite, not by the Arb overlap
  suite, which cannot see sound-but-non-minimal radii. Lesson recorded:
  statistical oracles verify enclosure, never minimality; minimality requires
  an exact reference.
- 2026-08-22 (stage 2). Test references consolidated into exact_ref.hpp after
  two independent reference bugs (missing exponent compensation; uint64
  mantissa-product wraparound producing 36k phantom failures). Rule: every
  mantissa product widens explicitly at the multiplication site.
- 2026-08-22 (stage 2). Statistical mul_floor replaced by component-wise
  truncated-product floors: the long-double replica inflated past the true
  bound and false-failed a correct implementation. Floors must be provable
  lower bounds, not approximations of one.

## Infrastructure decisions

- 2026-08-22 (stage 1 gate rev 2). Fast-math poison regex extended from
  -ffast-math|-Ofast to include -funsafe-math-optimizations,
  -fassociative-math, -freciprocal-math, and -ffinite-math-only, after review
  demonstrated that -fassociative-math configured cleanly under the old guard.
  -fassociative-math permits exactly the reduction reassociation the
  determinism contract forbids; -ffinite-math-only breaks NaN/Inf handling
  that interval arithmetic relies on. CUDA-side obligations for stage 7 logged
  in PLAN section 11: scan CMAKE_CUDA_FLAGS*, forbid nvcc -use_fast_math,
  require --fmad=false (nvcc defaults to fmad=true; likeliest parity failure).
- 2026-08-22 (stage 1 gate rev 2). Ball stub validation gap closed early:
  non-finite centres now rejected alongside bad radii. The stub is replaced at
  stage 2 but a scaffolding component should not accept values its successor
  would call invalid.
- 2026-08-22 (r2.1). Determinism contract scoped to configurations: blocks
  carry a config hash; bit-identity holds per configuration; cross-
  architecture results reconciled at elevated precision before the chain
  advances. Resolves the r1 contradiction between section 8's bit-identity
  claim and section 9's cross-arch admission.
- 2026-08-22 (r2.1). Archival tiering adopted (Zenodo concept DOI cluster as
  integrity anchor; Academic Torrents plus object storage for bulk ordinates).
  Reason: Zenodo quotas (50 GB standard, 200 GB on request) cannot hold
  6.4 TB; the r2 scope increase made the old single-deposit plan impossible.
- 2026-08-22 (r2.1). Data licence CC0 (code remains MIT). Reason: ordinates are
  facts; CC0 maximises reuse; citation norm preserved via CITATION.cff.

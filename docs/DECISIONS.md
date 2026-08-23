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
  its true denominator; caps fixed in the tool); the Newton-polish c_6 value
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
- 2026-08-22 (stage 3 rev 1). L1 reinstated as a real enclosure layer:
  zero-additive-slack overlap vs acb_lgamma intervals, swept t in
  [200, 3e12] x prec in {128,192,256,512} (84 combos). Golden corpus
  regenerated at dps=220 / 170 significant digits so corpus quantization
  (~1e-155 at campaign heights) stays below every certified radius; L2
  tolerance reduced to exactly the certified radius.
- 2026-08-22 (stage 3 rev 1). Radius now carries THREE explicitly derived
  components (series remainder x SAFETY, mpfr rounding, coefficient
  representation error); L3 policy-equality tolerance set to 1e-3 relative
  with written justification (transcription approximates post-series |centre|
  in closed form; difference provably < 1e-3 for t >= 200).

### Stage 3 additions

- 2026-08-22 (stage 3). Theta architecture: certified asymptotic series in
  production (6 terms + safety-factored first-omitted remainder bound,
  t >= 200 per Gabcke), mpmath-derived golden corpus as the empirical oracle
  (20 heights, 40-digit values committed), NO FLINT lgamma layer. Rationale:
  coefficient discovery and global Newton polish against mpmath loggamma
  produced a series matching the oracle to <= 4e-62 absolute across the full
  range, while FLINT 3.6.0's own acb_lgamma proved untrustworthy at the
  required level (zero-width ball excluding truth by ~3.6e-15, precision-
  independent) - logged as open item O2 for upstream report.
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

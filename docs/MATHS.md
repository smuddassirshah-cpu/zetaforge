# MATHS.md: derivations and references of record

Single reference of record for every mathematical claim used by the engine.
Rule adopted after review: each result is cited from its own primary source;
merging two results into one citation is a defect. Status column tracks whether
the derivation has been written and numerically tested. No stage may rely on a
claim whose status here is "pending" past the stage that needs it.

## Derivation obligations

| ID | Obligation | Needed by | Status |
|---|---|---|---|
| D1 | theta(t) truncation bound: series theta = ThetaMain(t) + sum_{k=1..6} c_k t^(1-2k); remainder claimed <= 4*(c7 + c8/t^2)/t^13 for t >= 200. Coefficients c1..c6 are exact rationals parsed at working precision from the D1 table below; c7 = 8191/2555904 is the first omitted magnitude. Factor 4 is deliberate safety headroom standing in for Gabcke's exact per-term constants (open item O1). Radius also carries two explicit secondary terms: mpfr rounding at working precision (centre_scale * 2^(2-prec)) and coefficient representation error (c_1 * 2^(1-prec) / t). Empirical validation: 84-combo sweep (21 heights x 4 precisions) shows tight-zone error/bound ratio <= 1.000000 with zero additive slack vs mpmath-derived goldens parsed at 1200 bits | stage 3 rev 1 | done (provisional constant, see O1) |
| D1a | Coefficient provenance: c_1..c_5 are exact small rationals; c_6 = 1414477/1476034560 exactly (MATHS.md D1 table is canonical). A Newton-polish pass suggested a different c_6 and "no small rational form"; that pass was a degenerate 6-point fit whose held-out agreement was coincidental, and its residuals contradicted the first-omitted-term model by ~1e3x at t=200 - diagnosed as overfit, retracted. The original discovery ladder failed to recover c_6's rational because limit_denominator caps (<= 10^8) sat below the true denominator (~1.476e9); caps extended | stage 3 rev 1 | done |
| D2 | RS validity threshold t0 = 200 pinned from Gabcke 1979 Thm 1 p.139 as quoted in Arias de Reyna Math. Comp. 80 (2011); NOT from observed agreement. Below t0 theta_certified throws; EM path owns the range (stage 4). Working precision derived: required delta_theta <= 1e-10 rad at campaign height T=3e12 where theta ~ 3.7e13 and theta' ~ 13.4 (so phase error maps to delta_t ~ 7.5e-12, far inside isolation tolerance); mpfr centre precision must satisfy theta * 2^(1-p) <= delta_theta -> p >= log2(7.4e23) ~= 79.3 -> 128-bit working precision chosen with multi-operation headroom; certified radius carries the mpfr rounding term explicitly | stage 3 | done |
| D3 | Correction-series remainder bound at campaign precision | stage 4 | pending |
| D4 | Multipoint window scheduling: total-campaign exponent derived from the per-window results below; no asserted exponents | stage 5 precondition | pending |
| D5 | Turing-method window accounting with explicit S(T) bounds (Trudgian lineage) | stage 5 | pending |
| D6 | Isolation cost factor model (evaluations per zero); early low-height estimate measured at stage 5 over [0,10^6], campaign-height figure confirmed at stage 9(a-i); both compared against the 2x reserve | stage 5 (estimate), stage 9(a-i) (confirmed) | pending |
| D7 | Ball operation error bounds: centre rounding enters via half_ulp_bound of the rounded result; radius terms outward-rounded by integer-exact primitives (radius.hpp); no binary op exact. Soundness enforced structurally: bit-exact integer reference suite (test_radius), not statistical | stage 2 | done-by-construction + test_radius |
| D7b | Complex ball (CBall) operation bounds: componentwise deviation bounds for add, mul and mul_real, each carrying an explicit centre-rounding term charged at the precision the result is actually rounded into (mul and mul_real round into temporaries at wp and store at wp; add rounds in place at the stored precision), all radius arithmetic performed with the outward integer-exact primitives of radius.hpp. Derivation below. Soundness AND tightness are both tested: corner containment cannot see an over-wide radius, so test_cball layer C4 additionally asserts the claimed radius lies within a small multiple of the exact deviation bound; layer C5 pins the precision rule | stage 4 | done, tests core/tests/test_cball.cpp layers C1-C5 |
| D8 | Sub-t0 Z(t): below t0=200, compute zeta(1/2+it) directly by Euler-Maclaurin (no theta needed); Z(t)=Re[e^{i theta}]zeta collapses to |zeta| when theta is not separated; instead return the complex ball and let callers extract Re via e^{i theta} above t0 or use |zeta| as a lower bound for enclosure purposes below t0. EM remainder: first omitted Bernoulli term x SAFETY, monotone-checked at runtime | stage 4 | pending |

## D7b derivation: complex ball operation bounds

Notation. A complex ball is a centre pair (ar, ai) with componentwise radii
(rr, ri); it denotes every z = x + iy with |x - ar| <= rr and |y - ai| <= ri.
Note this is a RECTANGLE, not a disc, and the bounds below are componentwise
throughout. Write a = (ar + p) + i(ai + q) with |p| <= rr_a, |q| <= ri_a, and
b = (br + u) + i(bi + v) with |u| <= rr_b, |v| <= ri_b.

Addition. The exact sum is (ar + br) + (p + u) + i[(ai + bi) + (q + v)], so
the deviation is bounded componentwise by rr_a + rr_b and ri_a + ri_b. The
stored centre is the mpfr sum at the component precision, which rounds; that
adds |result| * 2^(1-p) per component, using a magnitude ceiling of the
rounded result. Omitting this term (the state through rev 1) makes two exact
inputs whose sum needs one more bit than the working precision produce a
zero-radius ball around a rounded centre.

Multiplication. Expanding a*b and separating the centre product,

  Re(ab) = (ar br - ai bi)
           + [ar u + p br + p u] - [ai v + q bi + q v]
  Im(ab) = (ar bi + ai br)
           + [ar v + p bi + p v] + [ai u + q br + q u]

The parenthesised terms are the new centre. The remainder gives

  |dev Re| <= |ar| rr_b + |ai| ri_b + rr_a |br| + ri_a |bi|
              + rr_a rr_b + ri_a ri_b
  |dev Im| <= |ar| ri_b + |ai| rr_b + rr_a |bi| + ri_a |br|
              + rr_a ri_b + ri_a rr_b

No product of two centre magnitudes appears, because that product IS the
centre. The rev 1 implementation used L1(a) * L1(b) with both centre
magnitudes included, which is sound but always exceeds the output centre
magnitude, so every product ball contained zero and no downstream value could
ever be signed. Measured on the C4 fixture, that bound was about 141x the
exact deviation bound.

Centre rounding for mul: three mpfr operations produce each component (two
products and one addition or subtraction), so each component carries
(|t1| + |t2| + |result|) * 2^(1-wp) with magnitude ceilings.

Multiplication by a real ball (cf, cfr). Re(a * c) = ar cf + ar dc + p cf +
p dc with |dc| <= cfr, giving |dev Re| <= |ar| cfr + rr_a |cf| + rr_a cfr,
and |dev Im| <= |ai| cfr + ri_a |cf| + ri_a cfr. Both are attained at a corner,
so the bound is tight. Through rev 5 the implementation used the shared
cfr(|ar| + |ai|) in place of the first term of each, which dominates the
per-component term and therefore carries slack: at |ar| = 2.5, |ai| = 1.5 the
claimed radius was 1.44x the attained deviation, enough for a 0.9x radius cut
to survive exact corner containment. Since a bound with slack cannot be
falsified by the cut test the gate doctrine relies on, mul_real now uses the
per-component terms that mul has always used (R6-2). One mpfr multiplication
per component contributes |result| * 2^(1-wp), and that product is formed in a
fresh temporary at wp and swapped in, exactly as mul does, so 2^(1-wp) is the
precision the result is genuinely rounded into.

Precision rule (all three operations). The centre-rounding term is charged at
the precision the result is actually rounded into, never at a nominal working
precision the operation does not use. mul and mul_real build their results in
temporaries at wp and swap them in, so both charge at wp and leave every
component stored at wp; add rounds in place and charges each component at that
component's own stored precision, which is wp for any value that has already
passed through mul or mul_real.

Rev 6 correction (R6-1). Through rev 5, mul_real multiplied in place at each
component's STORED precision while still charging round_term at wp. Where the
stored precision was below wp the charge under-reported the error actually
committed and the operation was unsound, not merely loose: a ball at 53 bits
with centre 1 and radius 0, times cf = 1 + 2^-100 held at 200 bits with
cfr = 0 and wp = 200, returned centre 1.0 with radius ~2^-199 while the true
product sits 2^-100 away. The sentence above previously described the wp
charge as though the code took it there; it now does. Regression: test_cball
layer C5, which fails on the rev 5 implementation and passes on this one.

Rounding directions. Every radius quantity above is combined with up_add and
up_mul from radius.hpp, which round outward and are bit-exact by construction
(D7). Centre magnitude ceilings are taken with a direction chosen from the
sign of the value, so the conversion always rounds away from zero; taking the
absolute value AFTER a round-up conversion rounds negative values toward zero
and under-estimates every term the magnitude multiplies. Results pass through
inflate, so no complex operation claims exactness.

## References of record

- Platt, Trudgian. The Riemann hypothesis is true up to 3x10^12. Bull. London
  Math. Soc. 53 (2021), 792-797. [Anchor: H0, endpoint zero count, compute
  envelope]
- Platt. Isolating some non-trivial zeros of zeta. Math. Comp. 86 (2017),
  2449-2467. [Band-limited lineage rejected for the core; isolation practice]
- Odlyzko, Schoenhage. Fast algorithms for multiple evaluations of the Riemann
  zeta function. Trans. Amer. Math. Soc. 309 (1988), 797-809. [Multiple-
  evaluation paradigm]
- Hiary. An amortized-complexity method to compute the Riemann zeta function.
  Math. Comp. 80 (2011), 1785-1796. [Amortised T^{1/4+o(1)} per point over
  T^{1/4} windows: the multipoint design follows this]
- Hiary. Fast methods to compute the Riemann zeta function. Ann. of Math.
  174 (2011), 891-946. [Single-point T^{1/3+o(1)}: relevant to isolation
  refinement, not block scheduling]
- Arias de Reyna. High precision computation of Riemann's zeta function by the
  Riemann-Siegel formula, I. Math. Comp. 80 (2011), 995-1009. [Certified term
  and remainder bounds for RS; part II, arXiv:2201.00342, tracks floating-
  point error sources]
- Gabcke. Neue Herleitung und explizite Restabschatzung der Riemann-Siegel-
  Formel. Dissertation, Gottingen, 1979. [Historical primary for the expansion
  and first ten remainder bounds on the critical line]
- Trudgian. Improvements to Turing's method II. Rocky Mountain J. Math. 46
  (2016). [Explicit S(T) bounds for window accounting]
- LMFDB, Riemann zeta zeros computed by D. Platt: 103,800,788,359 zeros up to
  height 30,610,046,000, absolute precision +/- 2^-102, completeness verified
  by rigorous Turing's method. beta.lmfdb.org/data/riemann-zeta-zeros/
  [Stage 9(a-i) ground truth]
- Hiary, Ireland, Kyi. A method for verifying the generalized Riemann
  hypothesis. arXiv:2408.00187, accepted Math. Comp. [Background for v2 GRH]

## Open items

- O1: transcribe Gabcke's explicit per-term remainder constants (dissertation
  tables quoted in Arias de Reyna 2011) to replace the factor-4 headroom in
  D1's bound with an exact constant.
- O2 (RETRACTED as library defect; REINSTATED as harness-bug record): the
  reported acb_lgamma-vs-mpmath divergence was a double-conversion artefact of
  the measurement harness - FLINT interval midpoints were printed through
  %g doubles while reference values were compared at full precision, and one
  early oracle probe accidentally called _acb_dirichlet_theta_argument_at_arb
  (which returns pi*t^2-scale quantities for this call pattern), poisoning the
  first comparison set. acb_lgamma itself verified rigorous: direct interval
  comparison after parse-precision fixes shows our centre inside the FLINT
  enclosure at every (t, prec) combination (84 combos). Reproduction of the
  false positive: compare arb midpoint via mpfr_get_d against a full-precision
  reference and read the difference at double granularity. Independent
  re-testing on 2026-08-24 (review finding D3) reconfirmed the retraction:
  acb_lgamma is rigorous at every precision probed, there is no upstream
  defect, and no upstream report is warranted.

## Verified external constants

- H0 = 3,000,175,332,800; zeros below H0 = 12,363,153,437,138 (Platt-Trudgian
  2021).
- gamma_1 = 14.134725141... (reproduction target, stage 4, EM path).

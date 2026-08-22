# MATHS.md: derivations and references of record

Single reference of record for every mathematical claim used by the engine.
Rule adopted after review: each result is cited from its own primary source;
merging two results into one citation is a defect. Status column tracks whether
the derivation has been written and numerically tested. No stage may rely on a
claim whose status here is "pending" past the stage that needs it.

## Derivation obligations

| ID | Obligation | Needed by | Status |
|---|---|---|---|
| D1 | theta(t) truncation bound: certified error as function of retained terms | stage 3 | pending |
| D2 | RS validity threshold t0 from Arias de Reyna's published bounds | stage 3 (pins stage 4) | pending |
| D3 | Correction-series remainder bound at campaign precision | stage 4 | pending |
| D4 | Multipoint window scheduling: total-campaign exponent derived from the per-window results below; no asserted exponents | stage 5 precondition | pending |
| D5 | Turing-method window accounting with explicit S(T) bounds (Trudgian lineage) | stage 5 | pending |
| D6 | Isolation cost factor model (evaluations per zero), to be compared against the stage 9(a-i) measured factor | stage 9(a-i) | pending |

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

## Verified external constants

- H0 = 3,000,175,332,800; zeros below H0 = 12,363,153,437,138 (Platt-Trudgian
  2021).
- gamma_1 = 14.134725141... (reproduction target, stage 4, EM path).

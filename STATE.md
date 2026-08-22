# Build state

Current stage: 1
Last updated: 2026-08-22

## Stages
| # | Stage | Status | Gate |
|---|-------|--------|------|
| 1 | Scaffolding + CI (incl. r2.1 patch) | awaiting review | - |
| 2 | core/ball+ffix+ntt | pending | - |
| 3 | theta | pending | - |
| 4 | rs_main+rs_corr | pending | - |
| 5 | multipoint+signs+turing | pending | - |
| 6 | verifier (Rust) | pending | - |
| 7 | gpu + parity | pending | - |
| 8 | orchestrate | pending | - |
| 9 | Pilot [0,10^9] on cloud | pending | - |
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
- 2026-08-22, Repo public from day one with stub README until Phase 3 rewrite, visibility is itself a signal and Phase 3 owns claims discipline, n

## Open questions
Anything blocking or deferred, with the stage it affects.

- Cloud provider selection (AWS vs GCP vs Lambda): deferred until the stage 8 pricing scan; secrets schema in .env.example now covers all three. Affects stages 8-9.
- RS validity threshold t0: pinned at stage 3 from Arias de Reyna's published bounds; EM path owns all heights below it. Affects stage 4.
- GPU economic verdict ($/Mzero vs pinned 64-core reference): measured at stage 7, feeds ledger. Affects stretch D decision at stage 9(b).
- Isolation cost factor: modelled with 2x reserve now, measured for real at stage 9(a-i). Affects final D.

## Stage 2 inherited obligations (from gate review)
- ZF_CHECK harness gains seed reporting: randomised property tests must print the seed on failure, or a red CI run is not reproducible.
- Outward rounding is the stage 2 correctness invariant: radius arithmetic rounds away from zero always; prefer nextafter/(1+eps) inflation over fesetround for portability; Arb property-test generators must hunt round-to-nearest-on-radius specifically, since one nearest-rounded radius silently produces a non-enclosure.
- docs/benchmarks/ created and carrying the O(n log n) NTT benchmark before the stage 2 gate closes.

## Next action
Await formal "approved, continue" on stage 1. On approval: stage 2,
core/ball+ffix+ntt, introducing GMP/MPFR/Arb as first named dependencies with
property tests against Arb, honouring the three inherited obligations above.

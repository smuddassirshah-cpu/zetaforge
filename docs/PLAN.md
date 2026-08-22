# zetaforge Blueprint

Status: approved 2026-08-22. This document is the single source of architectural
truth for the build. Phase 2 requires zero architectural decisions; if a
question arises that this file does not answer, Phase 1 failed and the file
must be amended before work continues.

## 1. Scope and purpose

zetaforge is an open-source engine that verifies the Riemann hypothesis on the
critical line with machine-checkable certificates. v1 delivers a rigorous,
independently verifiable extension of the peer-reviewed verification frontier:
Platt-Trudgian (Bull. LMS 2021) proved RH up to height H0 = 3,000,175,332,800;
zetaforge certifies the next contiguous window [H0, H0+D], releasing every
zero ordinate, per-block certificates, and spacing/correlation statistics from
the new data. D is derived from the cost model in section 5: baseline 10^11,
stretch 10^12 (a 33 percent record extension), pinned by measured pilot
throughput against the budget ledger. Any positive extension is a record; the
differentiator is rigour, reproducibility, and open data.

Note on prior informal work: Gourdon's 2004 computation of the first 10^13
zeros corresponds to height about 2.44x10^12, which Platt-Trudgian already
exceed on both axes (higher height, more zeros, peer-reviewed). The relevant
frontier is therefore purely incremental beyond H0, and v2 targets height
10^13 outright.

### Non-goals

- Height 10^13 (v2 and later; the v1 record is the extension window beyond H0)
- A general arbitrary-precision library; Arb/MPC/MPFR are dependencies, not competitors
- Dirichlet L-function families / GRH
- Any analytic proof claims; any web service or endpoints
- Metal or other local GPU ports: Apple Silicon runs the CPU path for development only

## 2. Signal

Written for both registers a quant firm actually hires from.

For research infra / model validation: certified numerics with deterministic,
replayable artefacts and an independent cross-toolchain checker is model
validation in pure form, executed on a problem where the ground truth is
externally audited.

For trading research: the project demonstrates judgement under uncertainty.
Scope is derived from an explicit cost model against a published anchor, not
asserted; the deepest risk (a shared systematic error in the error-tracking
model) is answered with external ground truth rather than more self-referential
testing; every claim is gated on falsifiable evidence.

Signal ladder (each gate is a place one can stop without having failed):
stage 2 yields a standalone certified ball-arithmetic kernel library with
benchmarks; stage 3 a certified theta evaluator plus derivation notes;
stage 5 a complete certified verification of [0,10^6], already a reproducible
scientific artefact; stage 6 a general-purpose certificate verifier. The full
record lands at stage 10.

Draft CV bullet (progressive form until stage 10 completes): "Building and
running a certified high-performance zeta verifier: extending the
peer-reviewed Riemann Hypothesis verification frontier with an open,
independently checkable certificate pipeline (C++/CUDA/Rust/Python), including
the certificate format, its separate-toolchain verifier, and an external
ground-truth validation against Platt-Trudgian's certified results."

## 3. Component tree

```
zetaforge/
├── core/                [C++23]    ball arithmetic kernel
│   ├── ball.hpp           centre(mpfr)+radius(double), directed rounding, precision-ladder escalation
│   ├── ffix.hpp           fixed-point FFT-friendly type, tracked error inflation
│   ├── ntt.hpp            number-theoretic transform convolution, O(n log n), exact
│   └── theta.hpp          Riemann-Siegel theta(t), certified truncation bounds
├── zeta/                [C++23]    Z(t) = e^{i theta} zeta(1/2+it) evaluation
│   ├── rs_main.hpp        Riemann-Siegel main sum as blocked exponential sums
│   ├── rs_corr.hpp        RS correction series, certified via Arias de Reyna bounds (see 4)
│   ├── em_eval.hpp        Euler-Maclaurin evaluation for small t, where RS is invalid
│   ├── multipoint.hpp     Odlyzko-Schoenhage block evaluation; cost stated precisely in 4
│   └── grid.hpp           adaptive sampling; refinement near sign ambiguities
├── certify/             [C++23]    zero accounting
│   ├── signs.hpp          certified sign-change detection, isolation intervals
│   ├── turing.hpp         Turing method (Trudgian bounds): predicted vs observed N(T)
│   └── emit.hpp           certificate writer: immutable block records, hash-chained
├── gpu/                 [CUDA C++] kernels behind identical interfaces
│   ├── sum_kernels.cu     vectorised exponential-sum accumulation
│   ├── ntt_kernels.cu     batched transforms
│   └── parity.hpp         CPU/GPU bit-compatibility harness
├── verifier/            [Rust]     INDEPENDENT checker (deliberately different toolchain)
│   ├── format.rs          strict binary certificate schema, no panic paths
│   ├── check.rs           re-derives sign logic from transcripts + published theta bounds
│   └── cli.rs             single binary; exit 0 iff whole chain valid
├── orchestrate/         [Python]   campaign runner, never on certified hot path
│   ├── planner.py         window -> <=30-min work units, DAG, checkpoint schedule
│   ├── fleet.py           spot provisioning, capped-backoff retries, cost ledger
│   └── store.py           content-addressed object-store sync, resume state
├── analysis/            [Python]   science from released data
│   ├── spacing.py         nearest-neighbour spacing vs GUE
│   ├── paircorr.py        two-point correlation vs Montgomery-Odlyzko
│   └── lehmer.py          Lehmer-pair and large-value scans
├── tools/               [Python]   CLI: forge run|verify|stats
├── tests/                 unit + integration + property tests per component
├── docs/                  PLAN.md, DECISIONS.md, MATHS.md, benchmarks/
└── .github/workflows/     build matrix, tests, lint, dependency audit
```

## 4. Language selection

| Component | Language | Justification |
|---|---|---|
| core/zeta/certify | C++23 | Numeric hot path; mature GMP/MPFR C interop; deterministic FP control |
| gpu | CUDA C++ | Rented NVIDIA fleet is the only cost-effective mass-throughput option; CPU parity path mandatory |
| verifier | Rust | Implementation independence: a bug cannot exist twice across toolchains; safe parsing of untrusted blobs; static binary |
| orchestrate/analysis/tools | Python | Glue + cloud SDKs + scipy stack; provably off the certified path |
| Rejected: Julia everywhere | Julia | CUDA + GMP interop immaturity relative to C++ |

### 4b. Algorithmic lineage (mathematics)

The same rejected-alternatives discipline applied to the numerics, not just the
languages. Platt-Trudgian's rigour rests on a band-limited argument-principle
method (Platt, Math. Comp. 86 (2017), 2449-2467); this plan chooses a different
lineage and must say why.

| Option | Verdict | Reason |
|---|---|---|
| Chosen: Riemann-Siegel main sum + correction series with certified remainders (Arias de Reyna, Math. Comp. 80 (2011), 995-1009, extending Gabcke 1979) + Odlyzko-Schoenhage block multipoint evaluation | adopted | Best known amortised cost per point at fixed height; blocked exponential sums map naturally to GPU batching; every remainder carries a published rigorous bound, and Arias de Reyna's part II tracks all floating-point error sources |
| Rejected: Platt's band-limited method | rejected for engine core | Per-point cost higher at campaign heights; designed around CPU interval arithmetic in Arb; weaker fit to GPU batch structure. Turing-method scaffolding is still taken from Platt-Trudgian regardless, and the stage 9 ground-truth window audits this lineage choice empirically against their certified results |
| Mandatory adjunct: Euler-Maclaurin evaluation below threshold t0 | mandatory | RS is invalid at small t (Arias de Reyna states RS is useful only for large t); gamma_1 = 14.13 sits far below any RS validity region, so stage 4 exercises the EM path there and pins t0 from the published bounds |

Cost statement, precisely: Hiary's amortised method computes zeta(1/2+it) at up
to T^{1/4} points within a window of width T^{1/4} near height T at average
cost T^{1/4+o(1)} per point, matching the Odlyzko-Schoenhage result over that
interval (Hiary, Ann. Math. 174 (2011), 891-946; Odlyzko-Schoenhage 1988).
Total-campaign exponents depend on window scheduling and are derived, not
asserted, in docs/MATHS.md before stage 5 begins.

## 5. Data

- Sourcing: self-computed. Anchors: Platt-Trudgian's published terminal zero
  count (12,363,153,437,138 zeros at H0) for boundary continuity;
  Arb-generated small-range references for validation; Odlyzko tables for
  methodology comparison only.
- Storage: binary little-endian fixed-width ordinates internally;
  gzip-chunked release to Zenodo with SHA-256 manifests. Certificates:
  immutable, content-addressed.
- Lifecycle: raw transcripts (gitignored, object store) -> verified
  certificates (object store + Zenodo DOI) -> derived statistic CSVs
  (committed, KB-scale).
- Volume: driven by D, which is now derived rather than defaulted; see the
  cost model below.

### Cost model (scope-defining)

Anchor, published: Platt-Trudgian verified [0, H0] at roughly 7.5M CPU core-hours.
Work density grows like the square root of height per unit interval, so the work
in a narrow window [H0, H0+D] relative to their total is approximately
(3/2)(D/H0).

| Extension D | Share of P-T total work | CPU core-hour equivalent | CPU-equivalent cost at $0.01/core-hr |
|---|---|---|---|
| 10^10 | 0.5% | ~37,500 | ~$375 |
| 10^11 (baseline) | 5% | ~375,000 | ~$3.7k |
| 10^12 (stretch) | 54% (exact scaling (4/3)^{1.5}) | ~4.05M | ~$40k |

The original baseline of 10^10 was dinner money wrapped in fleet orchestration;
it was picked by default, not by derivation, and is rejected. The GPU multiplier
target from stage 7 (at least 20x throughput) applies on top of these figures,
so the stretch row lands in the low single-digit thousands of dollars even with
no further optimisation. Final D is pinned by measured cost-per-height from the
stage 9 pilot and the budget ledger, subject to one hard floor: the campaign may
stop early only after clearing a minimum committed record margin of D = 10^11.
The $50k+ envelope prices the full stretch plus pilot plus contingency.

### Data volume

- D = 10^11 implies about 4x10^11 ordinates, roughly 6.4 TB raw internally;
  release chunked to Zenodo with manifests. Drives the streaming verifier
  design (no full load).

## 6. Interfaces and data flow

planner -> fleet -> workers run C++/CUDA engine -> block files -> store ->
verifier -> verdict + Merkle root -> released dataset -> analysis.

| Boundary | Payload | Direction |
|---|---|---|
| engine -> store | {block_id, height range, ordinates[], sign-transcript digest, params, code_version} | push, idempotent |
| store -> verifier | same files, treated as untrusted | pull |
| verifier -> release | verdict JSON, Merkle root | publish |
| dataset -> analysis | read-only binaries + manifest | consume |

Invariant: Python never produces a number that enters a certificate; the
certified path never imports Python.

## 7. Security and threat surface

Class B (keyed): cloud credentials only; no external users, endpoints, or user
data.

- Secrets: cloud provider keys via environment variables loaded solely by
  orchestrate; `.env` gitignored from commit zero; `.env.example` committed;
  least-privilege IAM documented in docs/DECISIONS.md
- Entry points: certificate files are untrusted input to the Rust verifier
  (strict schema, fuzz-tested, no panic paths); CLI args validated once at
  the boundary
- Skipped blocks with reason: authn/authZ and rate limiting (no service);
  data protection beyond provider defaults (no personal data); SQL (none)
- Logging: structured; instance ids and spend only; never credentials

## 8. Failure modes and operations

- Spot preemption: work units <=30 min, checkpoint every K min,
  resume-from-checkpoint, capped exponential backoff, planner rebalances;
  kill-tested in stage 8
- Numerical ambiguity: radius exceeds tolerance -> escalate precision ladder
  (double -> double-double -> MPFR) in place; unresolved -> block marked
  contested, routed to high-precision CPU worker; contested blocks hard-gate
  final assembly, never silently dropped
- Object store: idempotent content-addressed PUTs; GET retried 3x backoff
  then loud failure
- Error policy: nothing swallowed; typed exit codes; user-facing messages
  reveal no internals
- Config: dev/prod separation; all tunables in committed config, no hardcoded
  flags
- Concurrency: shared-nothing process-per-worker; no cross-process mutable
  state; GPU stream model documented
- Determinism: pinned FP reduction order, no -ffast-math; same input implies
  bit-identical certificates (replay-verifiable); one GPU arch pinned per
  campaign segment

## 9. Limitations and trade-offs

- Modest extension magnitude; the record's value is rigour, reproducibility,
  and open data, stated plainly in the README
- Deepest technical risk: systematic error in the fixed-point error-tracking
  model itself. Stated plainly: the independent verifier does NOT control this
  risk. It re-derives sign logic, counts, and schema from transcripts, so it
  catches logic, format, and accounting errors; optimistic radii would flow
  through unchallenged. The control for shared error-model bias is external
  ground truth: stage 9 recomputes [H0 - 10^8, H0] inside Platt-Trudgian's
  certified region (about 375 CPU core-hours, a few dollars) and diffs zero
  ordinates and counts against their published endpoint
  (12,363,153,437,138). A non-empty diff halts the campaign before any new
  territory is claimed. Arb property-tests and MPFR escalation remain as
  secondary layers
- Cross-GPU-architecture determinism not guaranteed; segments pinned per
  arch, mismatches reconciled at elevated precision
- Single-window statistics evidence behaviour at height T only
- Spot-price spikes shrink stretch D before ever touching baseline D

## 10. Repository structure

Tree as in section 3, plus:

- LICENSE (MIT)
- CONTRIBUTING.md
- CITATION.cff
- .gitignore (.env, checkpoints/, data/raw/)
- .env.example
- README.md stub until the Phase 3 rewrite

CI exists from stage 1.

## 11. Execution stages

Each stage is one review gate. Tests ship inside each stage. Security work
sits inside the stage creating the exposure.

| # | Stage | Definition of done |
|---|---|---|
| 1 | Scaffolding + CI | Build matrix green for C++/CUDA stub, Rust crate, Python package; gitleaks + pip-audit wired; Class-B floor active from commit zero |
| 2 | core/ball+ffix+ntt | Property tests vs Arb on randomised suites; O(n log n) benchmark recorded in docs/benchmarks |
| 3 | theta | Matches mpmath to required digits; truncation-bound proofs written in docs/MATHS.md and numerically tested |
| 4 | rs_main+rs_corr+em_eval | gamma_1 = 14.1347 reproduced via the Euler-Maclaurin path; RS path certified above threshold t0 pinned at stage 3 from Arias de Reyna's published validity bounds; Z(t) matches Arb reference within certified radii on the overlap region |
| 5 | multipoint+signs+turing | End-to-end certified verification of [0,10^6] (~1.75M zeros including Gram-block failures); certificates emitted |
| 6 | verifier (Rust) | Accepts stage-5 chain; mutation tests detect every single-bit flip; fuzz clean |
| 7 | gpu + parity | Bit-compatible on pinned arch; >=20x throughput benchmark vs CPU |
| 8 | orchestrate | Simulated-preemption dry-run passes; resume proven by kill-test; ledger within 5% |
| 9 | Ground truth + pilot on cloud | (a) cross-validation window [H0-10^8, H0] recomputed and diffed against the Platt-Trudgian endpoint count: diff must be EMPTY, else campaign halts; (b) pilot window computed to measure cost-per-height and pin final D against the ledger; chain green throughout; data archived |
| 10 | Main campaign [H0, H0+D] | Hard precondition: stage 9(a) diff empty. Boundary zero-count continuity vs Platt-Trudgian; full chain verified; contested blocks resolved |
| 11 | Analysis + release | Notebooks reproduce figures from released data alone; Zenodo DOI; pre-push gate passed |

Counterexample protocol: a suspected off-line zero freezes the campaign,
triggers a max-precision triple-check by an independent path, and publishes
whatever is true.

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
the new data. Baseline D = 10^10 (about 4x10^10 new zeros), stretch
D = 10^11, chosen dynamically by the budget ledger. Any positive extension is
a record; the differentiator is rigour, reproducibility, and open data.

### Non-goals

- Beating Gourdon's informal 10^13 (v2 and later)
- A general arbitrary-precision library; Arb/MPC/MPFR are dependencies, not competitors
- Dirichlet L-function families / GRH
- Any analytic proof claims; any web service or endpoints
- Metal or other local GPU ports: Apple Silicon runs the CPU path for development only

## 2. Signal

Proves to a quant hiring manager: sole ownership of a numerically rigorous HPC
campaign: ball arithmetic, FFT-based multipoint evaluation, spot-fleet
orchestration, independent verification design, and statistical analysis of
large scientific datasets.

Draft CV bullet: "Extended the peer-reviewed Riemann Hypothesis verification
frontier with an open, independently checkable certified-computation pipeline
(C++/CUDA/Rust/Python); designed the certificate format and its separate
toolchain verifier."

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
│   ├── rs_corr.hpp        Gabcke-Pittner correction terms, certified remainder
│   ├── multipoint.hpp     Odlyzko-Schoenhage block evaluation, amortised O(T^{1/4+eps})/point
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
- Volume: D=10^10 implies about 4x10^10 ordinates, roughly 640 GB raw;
  release chunked. Drives the streaming verifier design (no full load).

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
  model itself; mitigated by Arb property-tests, MPFR escalation, and the
  independent verifier, never eliminated
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
| 4 | rs_main+rs_corr | Z(t) matches Arb reference within certified radii; first zero gamma_1 reproduced; Gram points sampled |
| 5 | multipoint+signs+turing | End-to-end certified verification of [0,10^6] (~1.75M zeros including Gram-block failures); certificates emitted |
| 6 | verifier (Rust) | Accepts stage-5 chain; mutation tests detect every single-bit flip; fuzz clean |
| 7 | gpu + parity | Bit-compatible on pinned arch; >=20x throughput benchmark vs CPU |
| 8 | orchestrate | Simulated-preemption dry-run passes; resume proven by kill-test; ledger within 5% |
| 9 | Pilot [0,10^9] on cloud | Chain green; cost within 1.5x projection; data archived |
| 10 | Main campaign [H0, H0+D] | Boundary zero-count continuity vs Platt-Trudgian; full chain verified; contested blocks resolved |
| 11 | Analysis + release | Notebooks reproduce figures from released data alone; Zenodo DOI; pre-push gate passed |

Counterexample protocol: a suspected off-line zero freezes the campaign,
triggers a max-precision triple-check by an independent path, and publishes
whatever is true.

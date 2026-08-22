# Build state

Current stage: 1
Last updated: 2026-08-22

## Stages
| # | Stage | Status | Gate |
|---|-------|--------|------|
| 1 | Scaffolding + CI | pending | - |
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

- 2026-08-22, Blueprint approved; scope locked to certified extension window beyond H0 = 3000175332800, baseline D = 10^10, stretch D = 10^11, n
- 2026-08-22, Repo public from day one with stub README until Phase 3 rewrite, visibility is itself a signal and Phase 3 owns claims discipline, n

## Open questions
Anything blocking or deferred, with the stage it affects.

- Cloud provider selection (AWS vs GCP vs Lambda): deferred until the stage 8 pricing scan; secrets schema in .env.example supports either. Affects stages 8-9.

## Next action
Stage 1: repository scaffolding and CI. CMake skeleton for core/, Cargo crate
for verifier/, pyproject for orchestrate/analysis/tools, GitHub Actions matrix
with gitleaks and pip-audit wired from first push.

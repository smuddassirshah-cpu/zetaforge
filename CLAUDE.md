# zetaforge build protocol

This repo is built under a gated protocol. Read docs/PLAN.md and STATE.md
before any work, every session.

## Coding standard
- Efficient, non-verbose, best practice. O(n) or O(n log n) where achievable;
  justify anything worse in a code comment at the site.
- No per-line explanatory annotation. A short decision-notes block at the top
  of non-trivial files only.
- British English in all comments, docs, and identifiers where language-neutral.
- No em-dashes anywhere.
- Every stage ships with its tests. Untested code does not pass a gate.
- Numerical determinism is contractual (PLAN.md section 8): pinned FP reduction
  order, no -ffast-math, bit-identical certificates from identical inputs.

## Security floor (non-negotiable, all stages)
- No secret in code, config, or any commit, ever. Git history is permanent:
  a committed secret means rotation, not deletion. Secrets load from
  environment variables per PLAN.md section 7; .env stays gitignored.
- Parameterised queries only. String-built SQL is a protocol violation.
  (zetaforge holds no database; if one ever appears this rule applies.)
- External input is validated at the boundary per PLAN.md section 7 before
  use. Certificate files are untrusted input to verifier/; no eval/exec/shell
  interpolation on external data; subprocess calls take argument lists.
- Errors are handled or propagated, never swallowed. No empty catch blocks.
  A TODO in an error path does not pass a gate.
- Never log secrets, tokens, or personal data.

## Gating protocol (non-negotiable)
1. Work exclusively on the current stage named in STATE.md. Never touch a
   later stage, even trivially.
2. Within a stage, delegate independent units to subagents in parallel where
   the environment supports it; integrate results yourself.
3. On stage completion: run the stage's tests, update STATE.md (status,
   files touched, decisions made, open questions), then STOP. Present a
   concise stage report: what was built, how it meets the definition of done,
   test results, a security note (any input point, secret, dependency, or
   endpoint this stage introduced, and how PLAN.md section 7 was honoured),
   and anything that deviated from PLAN.md and why.
4. Do not begin the next stage without the literal reply "approved, continue".
   Feedback short of that means revise the current stage.
5. Any deviation from PLAN.md must be flagged in the stage report and logged
   in STATE.md under Decisions. Silent deviation is a protocol violation.
6. Commit at each approved gate with message "stage <N>: <deliverable>".

## Session recovery
If context is fresh, reconstruct state ONLY from STATE.md and git log.
Never assume memory of prior sessions.

## Permissions
Ask before: adding dependencies not named in PLAN.md, restructuring the
repo tree, deleting files, force-pushing, or anything touching credentials.

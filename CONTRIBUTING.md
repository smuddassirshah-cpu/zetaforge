# Contributing

This repository is built under a hard-gated protocol: one stage at a time,
tests ship with every stage, nothing proceeds past a gate without explicit
approval. See [CLAUDE.md](CLAUDE.md) and [STATE.md](STATE.md).

- Coding standard: efficient, non-verbose, best practice. O(n) or O(n log n)
  where achievable; justify anything worse at the site.
- Numerical determinism is contractual: never reassociate floating-point
  reductions; fast-math is forbidden.
- Secrets never enter code, config, or history. Configuration comes from the
  environment; `.env.example` documents every variable.
- External input is validated at the boundary. Certificate files are untrusted.
- Run the test suite before proposing changes.
- British English in comments and docs. No em-dashes anywhere.

During the gated build, unsolicited pull requests are unlikely to be merged;
open an issue instead. After Phase 3 the protocol relaxes to normal review flow.

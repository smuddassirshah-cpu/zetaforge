#!/usr/bin/env python3
"""Mutation residue audit: no commit may carry an applied gate mutation.

Decision notes.

- A gate mutation reached commit 951bfd9 through a careless `git add -A`
  (reverted at 15bd11f, incident recorded in STATE.md). This tool answers, for
  every commit and every patch under docs/gate/mutations/, whether the MUTATED
  text is present in that commit's tree. It exists in tools/ because the rev 7
  Part A audit was run from an unversioned script, and an unversioned
  instrument issuing a gate finding is exactly the drift this repository's
  apparatus is meant to prevent (rev 7 finding A11).

- The method is SIGNATURE-BASED, not apply-based, and that is load-bearing. A
  `git apply --check` probe in both directions reported the known contaminated
  commit 951bfd9 as clean, because the u128 typedef landed later at 246bcb5
  and the context drift defeats the patch in BOTH directions, so "does not
  apply" was read as "not contaminated". This tool instead compares the
  patch's CHANGED lines against the tree: a commit is CONTAM for a patch when
  every line the patch inserts is present and at least one line it removes is
  absent.

- PURE DELETIONS are judged with a context anchor (rev 7 verification
  finding). A patch that only removes lines has no inserted signature, and
  "removed lines absent" cannot distinguish "mutation applied" from "these
  lines were never written in this old tree": the first draft of this rule
  flagged four stage 3 commits as contaminated by the golden-line-dropped
  patch, whose 178-digit corpus line simply did not exist before the corpus
  was regenerated. The rule now requires every CONTEXT line of the hunk to be
  present before a deletion verdict is considered: context present and any
  removed line missing means CONTAM, context present and all removed lines
  present means CLEAN, context absent means N/A (the file era predates the
  patch). Requiring only SOME removed lines missing also repairs the inverse
  failure, an applied deletion hiding behind removed lines that occur
  verbatim elsewhere in the file.

- The known incident is a waiver AND the positive control. 951bfd9 is
  permanently contaminated: history is immutable and the ledger records it.
  The audit therefore passes that exact (commit, patch) pair with a note, and
  --self-test asserts the tool STILL FLAGS it, so the tool cannot decay into
  one that reports everything clean. Any other contamination fails the audit.

Usage:
  tools/audit_mutation_residue.py --self-test      controls: 951bfd9 CONTAM,
                                                   15bd11f CLEAN
  tools/audit_mutation_residue.py --all            audit every commit
                                                   reachable from HEAD
  tools/audit_mutation_residue.py --range A..B     audit a rev-list range

Exit codes: 0 clean (waived incident included), 1 unwaived contamination,
2 the tool cannot run (controls unreachable, no catalogue).
"""

import argparse
import collections
import subprocess
import sys

# Permanently contaminated commits, recorded rather than hidden. Each entry is
# the full sha and the exact set of patches known applied there.
KNOWN_CONTAMINATED = {
    "951bfd960fad0ffc93d2c4fc438fdee8d5869611": {"01-radius-sticky.patch"},
}
CONTROL_CLEAN = "15bd11fac013865905697c0129d631fb248c5845"
# The stage 3 gate commit: its golden corpus predates the 178-digit
# regeneration, so the golden-line-dropped deletion patch must read N/A there,
# never CONTAM. This is the negative control for the pure-deletion rule, which
# in its first draft flagged four stage 3 commits (rev 7 verification finding).
CONTROL_PREDATES = "b87b583c24cb4f1120cb6df1ce6abe06d62ea985"


def git(*args):
    r = subprocess.run(["git", *args], capture_output=True, text=True)
    return r.returncode, r.stdout


def show(cache, commit, path):
    key = (commit, path)
    if key not in cache:
        rc, out = git("show", f"{commit}:{path}")
        cache[key] = None if rc else out
    return cache[key]


def parse_patch(text):
    """-> {path: (removed, inserted, context)}, stripped changed lines plus the
    hunks' context lines (needed to anchor pure-deletion verdicts)."""
    spec = collections.defaultdict(lambda: ([], [], []))
    path = None
    in_hunk = False
    for ln in text.splitlines():
        if ln.startswith("+++ b/"):
            path = ln[6:].strip()
            in_hunk = False
        elif ln.startswith(("--- ", "diff ", "index ")):
            in_hunk = False
        elif ln.startswith("@@"):
            in_hunk = True
        elif path and ln.startswith("-"):
            t = ln[1:].strip()
            if t:
                spec[path][0].append(t)
        elif path and ln.startswith("+"):
            t = ln[1:].strip()
            if t:
                spec[path][1].append(t)
        elif path and in_hunk and ln.startswith(" "):
            t = ln[1:].strip()
            if t:
                spec[path][2].append(t)
    return spec


def verdict(cache, commit, spec):
    """CONTAM / CLEAN / N/A for one patch against one commit's tree."""
    votes = []
    for path, (removed, inserted, context) in spec.items():
        body = show(cache, commit, path)
        if body is None:
            votes.append("N/A")
            continue
        lines = {l.strip() for l in body.splitlines()}
        r_in = sum(1 for l in removed if l in lines)
        i_in = sum(1 for l in inserted if l in lines)
        c_all = all(l in lines for l in context)
        if inserted and i_in == len(inserted) and (not removed or r_in < len(removed)):
            votes.append("CONTAM")
        elif not inserted and removed:
            # Pure deletion: only a present context anchor licenses a verdict
            # at all; without it the file era predates the patch.
            if context and c_all and r_in < len(removed):
                votes.append("CONTAM")
            elif c_all and r_in == len(removed):
                votes.append("CLEAN")
            else:
                votes.append("N/A")
        elif removed and r_in == len(removed):
            votes.append("CLEAN")
        elif not removed and inserted and i_in < len(inserted):
            votes.append("CLEAN")   # pure insertion absent
        else:
            votes.append("N/A")
    if "CONTAM" in votes:
        return "CONTAM"
    if "CLEAN" in votes:
        return "CLEAN"
    return "N/A"


def load_catalogue():
    rc, out = git("ls-tree", "--name-only", "HEAD", "docs/gate/mutations/")
    patches = [p for p in out.split() if p.endswith(".patch")]
    if not patches:
        print("no mutation catalogue at HEAD", file=sys.stderr)
        sys.exit(2)
    cat = {}
    for p in patches:
        rc, body = git("show", f"HEAD:{p}")
        if rc:
            print(f"cannot read {p} at HEAD", file=sys.stderr)
            sys.exit(2)
        cat[p.rsplit("/", 1)[-1]] = parse_patch(body)
    return cat


def audit(commits, cat):
    cache = {}
    bad = []
    for c in commits:
        r = {name: verdict(cache, c, spec) for name, spec in cat.items()}
        contam = sorted(n for n, v in r.items() if v == "CONTAM")
        clean = sum(1 for v in r.values() if v == "CLEAN")
        na = sum(1 for v in r.values() if v == "N/A")
        waived = KNOWN_CONTAMINATED.get(c, set())
        unwaived = [n for n in contam if n not in waived]
        if contam and not unwaived:
            note = "CONTAMINATED (recorded incident, waived): " + ", ".join(contam)
        elif unwaived:
            note = "CONTAMINATED, UNWAIVED: " + ", ".join(unwaived)
            bad.append((c, unwaived))
        else:
            note = "clean"
        print(f"{c[:9]} clean={clean:<3} contam={len(contam):<2} n/a={na:<3} {note}")
    return bad


def main():
    ap = argparse.ArgumentParser()
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--self-test", action="store_true")
    g.add_argument("--all", action="store_true")
    g.add_argument("--range")
    args = ap.parse_args()

    cat = load_catalogue()

    if args.self_test:
        for c in list(KNOWN_CONTAMINATED) + [CONTROL_CLEAN, CONTROL_PREDATES]:
            rc, _ = git("cat-file", "-e", f"{c}^{{commit}}")
            if rc:
                print(f"control commit {c[:9]} unreachable (shallow clone?)",
                      file=sys.stderr)
                sys.exit(2)
        cache = {}
        for c, expected in KNOWN_CONTAMINATED.items():
            got = {n for n, s in cat.items() if verdict(cache, c, s) == "CONTAM"}
            if got != expected:
                print(f"SELF-TEST FAIL: {c[:9]} expected CONTAM {sorted(expected)}, "
                      f"got {sorted(got)}", file=sys.stderr)
                sys.exit(1)
            print(f"self-test: {c[:9]} flagged {sorted(got)} as required")
        got = {n for n, s in cat.items() if verdict(cache, CONTROL_CLEAN, s) == "CONTAM"}
        if got:
            print(f"SELF-TEST FAIL: {CONTROL_CLEAN[:9]} (the revert) reported "
                  f"CONTAM {sorted(got)}", file=sys.stderr)
            sys.exit(1)
        print(f"self-test: {CONTROL_CLEAN[:9]} (the revert) clean as required")
        got = {n for n, s in cat.items()
               if verdict(cache, CONTROL_PREDATES, s) == "CONTAM"}
        if got:
            print(f"SELF-TEST FAIL: {CONTROL_PREDATES[:9]} (predates the "
                  f"corpus regeneration) reported CONTAM {sorted(got)}",
                  file=sys.stderr)
            sys.exit(1)
        print(f"self-test: {CONTROL_PREDATES[:9]} (pre-corpus era) not "
              f"flagged, as required")
        return

    if args.all:
        rc, out = git("rev-list", "--reverse", "HEAD")
    else:
        rc, out = git("rev-list", "--reverse", args.range)
    if rc:
        print("rev-list failed", file=sys.stderr)
        sys.exit(2)
    commits = out.split()
    print(f"auditing {len(commits)} commits against {len(cat)} patches "
          f"(catalogue from HEAD)")
    bad = audit(commits, cat)
    if bad:
        print("\nUNWAIVED CONTAMINATION:", file=sys.stderr)
        for c, names in bad:
            print(f"  {c}: {', '.join(names)}", file=sys.stderr)
        sys.exit(1)
    print("\nno unwaived contamination")


if __name__ == "__main__":
    main()

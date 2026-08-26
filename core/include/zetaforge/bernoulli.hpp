#pragma once

// Exact Bernoulli numbers B_{2n} as GMP rationals.
//
// Decision notes.
//
// Two certified series consume these: the Stirling expansion of log Gamma
// (MATHS.md D8) and the Euler-Maclaurin tail for zeta (MATHS.md D8, Backlund
// remainder). Both need exact values, not a transcribed decimal table: the
// terms are combined at working precisions of several hundred bits and a
// truncated constant would put an unaccounted error inside a certified radius.
//
// Values come from the standard recurrence
//
//   sum_{k=0}^{m} C(m+1, k) B_k = 0   ->   B_m = -1/(m+1) sum_{k<m} C(m+1,k) B_k
//
// evaluated in exact rational arithmetic, so the only thing that can be wrong
// is the recurrence itself. That is checked two ways: against the ten values
// transcribed in em_eval.cpp (kBNum/kBDen, which predate this file and are the
// subject of docs/gate/ATTACKS.md row 3), and against FLINT's bernoulli table
// in test_theta. A transcription error in the committed table and an error in
// the recurrence would have to agree to survive both.
//
// Cost is O(M^2) exact rational operations to reach B_{2M}, done once and
// cached. The cache is not thread-safe; the certified path is single-threaded
// today and stage 7 owns parallel evaluation.

#include <gmp.h>

namespace zetaforge {

// Exact B_{2n} for n >= 1 into out, which the caller has initialised.
// Throws std::out_of_range above kBernoulliMaxN.
void bernoulli_2n(unsigned n, mpq_t out);

// Upper limit of the table. The Stirling series at 512-bit working precision
// stops near n = 100 and the Euler-Maclaurin tail near n = 70; 256 leaves room
// for both without an unbounded cache.
constexpr unsigned kBernoulliMaxN = 256;

}  // namespace zetaforge

cmake_minimum_required(VERSION 3.24)
project(zetaforge LANGUAGES CXX)

# Decision note: the CUDA option exists from stage 1 so the CI story is wired
# before the kernels exist (stage 7). The EXISTS guard keeps the build honest
# until gpu/ materialises with its own gate.

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Determinism contract (docs/PLAN.md section 8): refuse to configure if any
# fast-math style flag is present anywhere in the user's flags, and pin the
# contraction and rounding-mode assumptions ourselves rather than trusting
# compiler defaults, which differ between GCC and Apple Clang (gate finding S2).
foreach(_flagvars
    CMAKE_CXX_FLAGS
    CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_RELWITHDEBINFO
    CMAKE_CXX_FLAGS_MINSIZEREL)
  if("${${_flagvars}}" MATCHES
      "-ffast-math|-Ofast|-funsafe-math-optimizations|-fassociative-math|-freciprocal-math|-ffinite-math-only")
    message(FATAL_ERROR
      "${_flagvars} contains a fast-math style flag; this violates the "
      "zetaforge determinism contract (PLAN.md section 8). Remove it.")
  endif()
endforeach()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -ffp-contract=off -frounding-math)
endif()

option(ZETAFORGE_BUILD_CUDA "Build GPU kernels (requires CUDA toolkit)" OFF)
if(ZETAFORGE_BUILD_CUDA AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/gpu/CMakeLists.txt)
  enable_language(CUDA)
  add_subdirectory(gpu)
endif()

# Stage 2 dependencies, all named in docs/PLAN.md section 4. Arb is consumed
# as merged into FLINT 3 (see DECISIONS.md).
#
# MPFR/GMP resolve via pkg-config. FLINT does not: Ubuntu's libflint-dev
# resolves headers through pkg-config but omits the link library, so FLINT is
# located explicitly with name fallbacks instead.
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPFR REQUIRED IMPORTED_TARGET mpfr)
pkg_check_modules(GMP REQUIRED IMPORTED_TARGET gmp)
find_path(FLINT_INCLUDE_DIR NAMES flint/flint.h)
find_library(FLINT_LIBRARY NAMES flint libflint libflint3)
if(NOT FLINT_INCLUDE_DIR OR NOT FLINT_LIBRARY)
  message(FATAL_ERROR "FLINT 3 not found: install libflint-dev / brew install flint")
endif()
message(STATUS "FLINT: ${FLINT_LIBRARY} (+ ${FLINT_INCLUDE_DIR})")

enable_testing()
add_subdirectory(core)

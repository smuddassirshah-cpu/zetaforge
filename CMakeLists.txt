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
  if("${${_flagvars}}" MATCHES "-ffast-math|-Ofast")
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

enable_testing()
add_subdirectory(core)

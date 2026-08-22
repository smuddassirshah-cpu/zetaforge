cmake_minimum_required(VERSION 3.24)
project(zetaforge LANGUAGES CXX)

# Decision note: the CUDA option exists from stage 1 so the CI story is wired
# before the kernels exist (stage 7). The EXISTS guard keeps the build honest
# until gpu/ materialises with its own gate.

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(ZETAFORGE_BUILD_CUDA "Build GPU kernels (requires CUDA toolkit)" OFF)
if(ZETAFORGE_BUILD_CUDA AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/gpu/CMakeLists.txt)
  enable_language(CUDA)
  add_subdirectory(gpu)
endif()

enable_testing()
add_subdirectory(core)

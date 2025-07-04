# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

Integral is a high-performance chess engine written in C++20 that uses an efficiently updatable neural network (NNUE) for evaluation. The engine implements negamax search with alpha-beta pruning and Lazy SMP for multi-threading.

## Build Commands

```bash
# Build with native CPU optimizations (recommended)
make native

# Build for specific architectures
make avx512         # AVX512 optimizations
make avx2_bmi2      # AVX2 + BMI2 (fast PEXT)
make avx2           # AVX2 (slow PEXT)
make debug          # Debug build with symbols

# Clean build artifacts
make clean

# Build with data generation support
make DATAGEN=ON
```

## Testing Commands

```bash
# Run all tests
./integral test

# Run specific test suites
./integral test see      # Static Exchange Evaluation tests
./integral test perft    # Move generation correctness tests

# Run benchmark
./integral bench         # Default depth 12
./integral bench depth 10   # Custom depth

# Run perft from specific position
./integral
position fen "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
go perft 5
```

## Code Formatting

```bash
# Format all source files
find src -name "*.cc" -o -name "*.h" | xargs clang-format -i

# Check formatting without modifying
find src -name "*.cc" -o -name "*.h" | xargs clang-format --dry-run --Werror
```

## Architecture Overview

### Core Components

- **src/chess/**: Core chess logic
  - `board.h/cc`: Board representation and move generation
  - `move_gen.h/cc`: Move generation using magic bitboards
  - `fen.h/cc`: FEN parsing and board setup

- **src/engine/**: Engine implementation
  - `evaluation/`: NNUE evaluation system
    - Architecture: `(768x12 -> 1536)x2 -> (16 -> 32 -> 1)x8`
    - Horizontally mirrored perspective network
    - 12 factorized king buckets, 8 output buckets
  - `search/`: Search algorithms
    - Negamax with alpha-beta pruning
    - Lazy SMP parallel search
    - Transposition tables and move ordering
  - `uci/`: UCI protocol implementation

- **src/data_gen/**: Self-play data generation for NNUE training

- **src/magics/**: Magic bitboard implementation for fast move generation

- **third-party/**: External dependencies
  - `fathom/`: Syzygy tablebase support
  - `fmt/`: Modern C++ formatting (header-only)

### Key Design Patterns

1. **SIMD Abstractions**: The codebase uses extensive SIMD optimizations with architecture-specific implementations in `shared/simd/`.

2. **Neural Network**: The NNUE implementation supports incremental updates for efficiency. Networks are embedded in the binary or downloaded during build.

3. **Move Generation**: Uses magic bitboards for sliding piece attacks. Attack tables are generated at compile time when possible.

4. **Thread Safety**: Search uses thread-local storage for position evaluation. Shared data structures (transposition table, move history) use atomic operations.

### Build System

- Primary: CMake (requires 3.16+)
- Secondary: Makefile wrapper around CMake
- Compiler: GCC 13+ or Clang 10+ (C++20 required)
- The build system auto-detects CPU features for native builds, particularly checking for fast PEXT support

## Development Guidelines

1. **Code Style**: Follow the existing style (Google-based with modifications). Use `.clang-format` for consistency.

2. **Testing**: Add tests to `src/tests/` for new functionality. Test move generation changes with perft.

3. **Performance**: This is a performance-critical application. Profile changes and run benchmarks to ensure no regression.

4. **NNUE Updates**: When modifying evaluation, ensure compatibility with the existing network architecture or update network loading code accordingly.

5. **UCI Compliance**: Maintain strict UCI protocol compliance for GUI compatibility.
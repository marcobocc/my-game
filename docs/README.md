# README

## Conventions

### Right-Handed Coordinate System

The engine uses a **right-handed coordinate system** where:

- **X-axis**: Points to the right
- **Y-axis**: Points upward
- **Z-axis**: Points toward the viewer (out of the screen)

```
    Y
    |
    |
    |_______ X
   /
  /
 Z
```

### Vertex Winding Order

All mesh geometry must use **counter-clockwise vertex winding order** when viewed from the front face.

For example, a triangle with vertices A, B, C:

```
     A
    / \
   /   \
  /     \
 /       \
B --- --- C

✅ (Correct) Counter-Clockwise:
Vertices: A → B → C (counter-clockwise)

❌ (Incorrect) Clockwise:
Vertices: A → C → B (clockwise)
```

## Building and Running

### Prerequisites

- CMake 3.22 or later
- vcpkg package manager
- Vulkan SDK
- C++23 compatible compiler

### Build and Run

```bash
# Build and run a specific target (optionally: run linting with clang-tidy)
python3 build.py <target> [--tidy]

# Clean build artifacts
python3 build.py clean
```

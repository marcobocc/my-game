## Building with Tracy enabled

Pass `--tracy` to the build script:

```
python3 build.py --tracy editor
```

This sets `-DTRACY_ENABLE=ON`. 

## Building the Tracy GUI

The profiler UI must be built from source to avoid protocol mismatches with the client.

**Configure** (one time):
```
cmake -S vendor/tracy/profiler -B build/tracy-profiler -DCMAKE_BUILD_TYPE=Release
```

**Build:**
```
cmake --build build/tracy-profiler --parallel
```

**Find the binary:**
```
find build/tracy-profiler -maxdepth 2 -type f -perm +111 -name "tracy*"
```

## Connecting

1. Start the Tracy GUI binary.
2. Start the editor built with `--tracy`.
3. Click **Connect** in the Tracy GUI.

`TRACY_ON_DEMAND` is enabled, so Tracy only captures data while the GUI is connected. There is no overhead when disconnected.

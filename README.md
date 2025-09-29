# Dagor Engine Utilities

A collection of standalone format utilities adapted from the Dagor Engine.

To build the libraries and examples run:

```bash
git clone --recursive https://github.com/moleium/dagutils
cd dagutils

cmake -B build -G Ninja
cmake --build build --config Release
```

This will compile the core library `dagutils` and place the example binaries in the `build/examples/` directory.

### Formats

- Datablock
- Bitstream

### Examples

The `examples` directory has projects demonstrating how to use each component

Copyright (c) 2023, Gaijin Entertainment
All rights reserved.
#!/bin/bash

# Idea:
# Unpack to wasm version, apply patches
# Unpack to native version, apply patches
# Build both (wasm will fail)
# Copy wasm library out (libicuuc)
# Copy executables from native to wasm

# find . -type f -executable -exec file {} \; | grep ELF |cut -d : -f 1 | xargs -I X cp -p X ../icu-wasm/X
# find . -type f -executable -exec file {} \; | grep ELF |cut -d : -f 1 | xargs -I X touch ../icu-wasm/X

# Build wasm again
# It should succeed. Copy libicudata out

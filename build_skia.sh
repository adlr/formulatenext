#!/bin/bash

SKIAROOT="$(readlink -f $(dirname "$0")/skia/skia)"
BUILD_DIR="${SKIAROOT}/formulate_out" "${SKIAROOT}/modules/canvaskit/compile.sh"

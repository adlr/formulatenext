#!/bin/bash
# Copyright 2018 Google LLC
#
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -ex


BASE_DIR=`cd $(dirname ${BASH_SOURCE[0]}) && pwd`
export PATH="$PATH:$(readlink -f "${BASE_DIR}/depot_tools")"
cd "$BASE_DIR/skia/skia"

BUILD_DIR=out/Debug

# Inspired by https://github.com/Zubnix/skia-wasm-port/blob/master/build_bindings.sh
./bin/gn gen ${BUILD_DIR} \
  --args="cc=\"clang-4.0\" \
  cxx=\"clang++-4.0\" \
  extra_cflags_cc=[\"-frtti\"] \
  is_debug=true \
  is_official_build=false \
  is_component_build=false \
  skia_enable_gpu=false \
  werror=true"

# Build all the libs, we'll link the appropriate ones down below
NINJA=`which ninja`
${NINJA} -C ${BUILD_DIR} libskia.a

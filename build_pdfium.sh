#!/bin/bash
# Copyright 2018 Google LLC
#
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -ex


BASE_DIR=`cd $(dirname ${BASH_SOURCE[0]}) && pwd`
export PATH="$PATH:$(readlink -f "${BASE_DIR}/depot_tools")"
. $BASE_DIR/emsdk/emsdk_env.sh
cd "$BASE_DIR/pdfium/pdfium"

BUILD_DIR=out/Debug

# Inspired by https://github.com/Zubnix/skia-wasm-port/blob/master/build_bindings.sh
gn gen ${BUILD_DIR} \
  --args="is_debug=false \
  is_official_build=false \
  use_goma=false \
  pdf_use_skia=false \
  pdf_use_skia_paths=false \
  pdf_enable_xfa=false \
  pdf_enable_v8=false \
  pdf_is_standalone=true \
  is_component_build=false \
  clang_use_chrome_plugins=false \
  use_libjpeg_turbo=true \
  use_glib=false \
  target_os=\"wasm\" \
  target_cpu=\"wasm\" \
  pdf_is_complete_lib=true \
  "

# Build all the libs, we'll link the appropriate ones down below
NINJA=`which ninja`
${NINJA} -C ${BUILD_DIR} pdfium

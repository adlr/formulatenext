#!/bin/bash

. ./setup_env.sh

ENGINE=../engine/src

BUILD_DIR=out/Debug

set +ex

cd "${ENGINE}"

# skia_use_icu && skia_use_harfbuzz && skia_pdf_subset_harfbuzz
#  ar=\"${EMAR}\" \
# cc=\"${EMCC}\" 
#  cxx=\"${EMCXX}\" \

  # extra_cflags_cc=[\"-frtti\"] \
  # extra_cflags=[\"-s\", \"WARN_UNALIGNED=1\",
  #   \"-DSKNX_NO_SIMD\", \"-DSK_DISABLE_AAA\", \"-DSK_DISABLE_READBUFFER\",
  #   \"-DSK_DISABLE_EFFECT_DESERIALIZATION\",
  #   \"-DSK_DISABLE_LEGACY_SHADERCONTEXT\",
  #   \"-DSK_RELEASE\", \"-DGR_GL_CHECK_ALLOC_WITH_GET_ERROR=0\",
  # ] \
#  skia_use_system_freetype2=false \
  # skia_use_system_libpng=false \
  # skia_use_system_libjpeg_turbo=false \
  # skia_use_system_zlib=false\
  # skia_use_system_harfbuzz=false \
  # skia_use_system_icu=false \
  # use_PIC=false \
  # werror=true \
gn --export-compile-commands gen ${BUILD_DIR} \
  --args="\
  is_debug=false \
  is_official_build=false \
  is_component_build=false \
  target_cpu=\"wasm\" \
  \
  skia_use_angle=false \
  skia_use_dng_sdk=false \
  skia_use_egl=true \
  skia_use_expat=false \
  skia_use_fontconfig=false \
  skia_use_freetype=true \
  skia_enable_fontmgr_custom=true \
  skia_use_libheif=false \
  skia_use_libjpeg_turbo=true \
  skia_use_libpng=true \
  skia_use_libwebp=false \
  skia_use_lua=false \
  skia_use_piex=false \
  skia_use_vulkan=false \
  skia_use_wuffs=true \
  skia_use_zlib=true \
  \
  skia_use_icu=true \
  skia_use_harfbuzz=true \
  skia_pdf_subset_harfbuzz=true \
  skia_enable_gpu=true \
  skia_gl_standard = \"webgl\" \
  skia_enable_fontmgr_empty=false \
  skia_enable_fontmgr_custom_empty=false   \
  skia_enable_sksl_interpreter=true \
  \
  skia_enable_skshaper=true \
  skia_enable_ccpr=false \
  skia_enable_nvpr=false \
  skia_enable_skparagraph=true \
  skia_enable_pdf=true \
  target_cpu = \"wasm\" \
  target_os = \"wasm\" \
  is_clang=true \
  \
  \
  use_goma=false \
  pdf_use_skia=false \
  pdf_use_skia_paths=false \
  pdf_enable_xfa=false \
  pdf_enable_v8=false \
  pdf_is_standalone=true \
  clang_use_chrome_plugins=false \
  use_libjpeg_turbo=true \
  use_system_libpng=true \
  use_glib=false \
  pdf_is_complete_lib=true
  "

ninja -C ${BUILD_DIR}

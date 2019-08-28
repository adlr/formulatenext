#!/bin/bash

. ./setup_env.sh

cd opencv/opencv
python ./platforms/js/build_js.py build_wasm --build_wasm


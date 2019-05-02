#!/bin/bash

set -ex

PATH="$PATH:$(readlink -f depot_tools)"

sudo apt install xz-utils
mkdir -p pdfium
cd pdfium
fetch pdfium

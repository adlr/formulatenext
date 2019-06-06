#!/bin/bash

set -ex

TESTOBJS=testobjs

cd "${TESTOBJS}"
rm -f *.gcda
(cd .. && make -f Makefile.test -j4 && ${TESTOBJS}/test)
lcov --directory . \
     --base-directory . \
     --gcov-tool $(readlink -f ../llvm-gcov.sh) \
     --capture -o cov.info
genhtml cov.info -o output

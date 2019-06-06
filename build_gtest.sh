#!/bin/bash

GTEST_DIR=/usr/src/gtest
clang++-4.0 -std=c++11 -isystem ${GTEST_DIR}/include -I${GTEST_DIR} \
    -pthread -c ${GTEST_DIR}/src/gtest-all.cc
ar -rv libgtest.a gtest-all.o

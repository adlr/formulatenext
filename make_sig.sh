#!/bin/bash

. ./setup_env.sh
make -f Makefile.signature -j $(cat /proc/cpuinfo| grep processor | wc -l)

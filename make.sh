#!/bin/bash

. ./setup_env.sh
make -j 1 #$(cat /proc/cpuinfo| grep processor | wc -l)

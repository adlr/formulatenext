#!/bin/bash

. ./setup_env.sh
make -j $(cat /proc/cpuinfo| grep processor | wc -l)

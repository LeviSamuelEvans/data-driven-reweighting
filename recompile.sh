#!/bin/bash

cmake  Reweighter -B build
make -C build
source build/*/setup.sh
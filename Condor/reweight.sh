#!/bin/bash

# Go to reweighting directory
cd /scratch4/levans/data-driven-reweighting/

# Compile the code
source compile.sh

# Run the code
reweight --configFile configs/config_1l.cfg
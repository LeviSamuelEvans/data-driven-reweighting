#!/bin/bash

# Go to reweighting directory
cd /scratch4/levans/data-driven-reweighting/

# Compile the code
source compile.sh

# Run the code
reweight --configFile Configs/Nominal/1l/config_1l_nominal_PP8.cfg
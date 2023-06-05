#!/bin/bash

# Go to reweighting directory
cd /scratch4/levans/data-driven-reweighting/

# Compile the code
source compile.sh

# Run the code
options= 
options+=$1
echo "The configs being run on are " ${options}
reweight --configFile Configs/Nominal/1l/${options}
#!/bin/bash
#

export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase
source /cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase/user/atlasLocalSetup.sh
#asetup AnalysisBase,master,latest
asetup AnalysisBase,22.2.60

mkdir -p build
cmake -S Reweighter -B build
make -C build
source build/*/setup.sh


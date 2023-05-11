
executable = /scratch4/levans/data-driven-reweighting/Condor/reweight.sh
arguments = config_1l.cfg 
universe = vanilla
+MaxRuntime = 3600 # 1 Hour 

output = output.$(ClusterId)$(ProcId).out
error = error.$(ClusterId)$(ProcId).err
log = condor.$(ClusterId)$(ProcId).log

should_transfer_files = YES
when_to_transfer_output = ON_EXIT

queue



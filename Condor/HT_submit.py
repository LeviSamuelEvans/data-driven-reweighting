import os
import subprocess

def create_submission_files(configs, output_folder):

    # Create the output folders if they don't exist
    bash_scripts_folder = os.path.join(output_folder, "bashScripts")
    submit_folder = os.path.join(output_folder, "Submit")
    os.makedirs(bash_scripts_folder, exist_ok=True)
    os.makedirs(submit_folder, exist_ok=True)

    for index, config in enumerate(configs):

        # Extract the config file name without the path
        config_filename = os.path.basename(config)

        # Set the job name with the config file name
        job_name = f"Job_{config_filename}"

        sh_filename = f"reweight_{config_filename}.sh"
        sh_filepath = os.path.join(bash_scripts_folder, sh_filename)
        with open(sh_filepath, 'w') as sh_file:
            sh_file.write(f"#!/bin/bash\n\n")
            sh_file.write(f"cd /scratch4/levans/data-driven-reweighting/\n")
            sh_file.write(f"source compile.sh\n")
            sh_file.write(f"reweight --configFile {config}\n")

        sub_filename = f"reweight_{config_filename}.sub"
        sub_filepath = os.path.join(submit_folder, sub_filename)
        with open(sub_filepath, 'w') as sub_file:
            sub_file.write(f"executable = {sh_filepath}\n")
            sub_file.write(f"arguments = {config} $(ClusterId)$(ProcId)\n")
            sub_file.write("universe = vanilla\n")
            sub_file.write("+MaxRuntime = 3600\n")
            sub_file.write(f"output = {output_folder}/output/output_{job_name}_.$(ClusterId)$(ProcId).out\n")
            sub_file.write(f"error = {output_folder}/error/error_{job_name}_.$(ClusterId)$(ProcId).err\n")
            sub_file.write(f"log = {output_folder}/log/log_{job_name}_.$(ClusterId)$(ProcId).log\n")
            sub_file.write("should_transfer_files = YES\n")
            sub_file.write("when_to_transfer_output = ON_EXIT\n")
            sub_file.write("queue\n")

        #submit
        subprocess.run(['condor_submit', sub_filepath], check=True)


configs = [
    # Single Lepton Config files
    "/scratch4/levans/data-driven-reweighting/Configs/Nominal/1l/config_1l_nominal_PP8.cfg", # Nominal
    #"/scratch4/levans/data-driven-reweighting/Configs/Nominal/1l/config_1l_nominal_PH7.cfg", # PH7 PS
    # "/scratch4/levans/data-driven-reweighting/Configs/Nominal/1l/config_1l_nominal_PP8_ptHard1.cfg", # ptHard1 Variation
    # "/scratch4/levans/data-driven-reweighting/Configs/Nominal/1l/config_1l_nominal_DipolePS.cfg", # DipolePS Variation
    # "/scratch4/levans/data-driven-reweighting/Configs/FSR_DOWN/1l/config_1l_FSR_DOWN_PP8.cfg", # Final State Radiation down
    # "/scratch4/levans/data-driven-reweighting/Configs/FSR_UP/1l/config_1l_FSR_UP_PP8.cfg", # Final State Radiation up
    # "/scratch4/levans/data-driven-reweighting/Configs/muR05_muF05/1l/config_1l_muR05_muF05_PP8.cfg", # scale variation down
    # "/scratch4/levans/data-driven-reweighting/Configs/muR20_muF20/1l/config_1l_muR20_muF20_PP8.cfg", # scale variation up
    # "/scratch4/levans/data-driven-reweighting/Configs/var3c_DOWN/1l/config_1l_var3c_DOWN_PP8.cfg", # Initial State Radiation down
    # "/scratch4/levans/data-driven-reweighting/Configs/var3c_UP/1l/config_1l_var3c_UP_PP8.cfg", #  Initial State Radiation up
]
output_folder = "/scratch4/levans/data-driven-reweighting/Condor"

create_submission_files(configs, output_folder)
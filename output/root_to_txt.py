#! /usr/bin/python

# Originally written by Alex Froch for ttc & ttlight reweighting

from ROOT import TFile

import os
from glob import glob
import argparse


def get_parser():
    """
    Argument parser for HT-Reweighter script.

    Returns
    -------
    args: parse_args
    """
    parser = argparse.ArgumentParser(description="Combining command line options")

    parser.add_argument(
        "-v",
        "--variation",
        type=str,
        default=None,
        help="Enter the name of the variation you want to combine.",
    )
    parser.add_argument(
        "-c",
        "--channel",
        type=str,
        default=None,
        help="Enter the name of the variation you want to combine.",
    )
    return parser.parse_args()


if __name__ == "__main__":
    args = get_parser()

    if args.variation is None:
        variation_list = [
            "fsr_down",
            "fsr_up",
            "muR05_muF05",
            "muR20_muF20",
            "nominal",
            "var3c_down",
            "var3c_up",
        ]
    else:
        variation_list = [args.variation]

    if args.channel is None:
        channel_list = ["1l", "2l"]
    else:
        channel_list = [args.channel]

    for variation in variation_list:
        for channel in channel_list:
            selections_list = []
            factors_list = []
            bin_string_list = []

            files_in_output = glob(
                os.path.join(os.path.dirname(__file__), variation, f"*{channel}*.root")
            )
            if len(files_in_output) != 0:
                for file in files_in_output:
                    # Read in root file with the reweightings
                    inputFile = TFile(file, "READ")

                    # Append reweightings, selections and binnings to list
                    selections_list.append(inputFile.Get("selection"))
                    factors_list.append(inputFile.Get("factors"))
                    bin_string_list.append(inputFile.Get("binning"))

                #  Check that selections and reweightings are same length
                if len(factors_list) != len(selections_list):
                    raise ValueError(
                        f"Not the same amount of selections as factors! N_Factors: {len(factors_list)}, N_Selections: {len(selections_list)}"
                    )

                else:
                    with open(
                        os.path.join(
                            os.path.dirname(__file__),
                            variation,
                            f"reweighting_{channel}_{variation}.txt",
                        ),
                        "w",
                    ) as f:
                        for selections, factors in zip(selections_list, factors_list):
                            for selection, factor in zip(selections, factors):
                                f.write("selection: ")
                                f.write(selection + "\n")
                                f.write("factor: ")
                                f.write(factor + "\n")
                                f.write("\n")

                    with open(
                        os.path.join(
                            os.path.dirname(__file__),
                            variation,
                            f"binning_{channel}_{variation}.txt",
                        ),
                        "w",
                    ) as g:
                        for binnings in bin_string_list:
                            for binning in binnings:
                                g.write(binning + "\n")

            else:
                raise ValueError(
                    f"No input files found for channel {channel} and variation {variation}!"
                )

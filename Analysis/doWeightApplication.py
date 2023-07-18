#! /usr/bin/python

import os,sys

from ROOT import TFile, TTree, TF1, gROOT
from array import array
from copy import copy
import math
from glob import glob

gROOT.SetBatch(True)
gROOT.ProcessLine( "gErrorIgnoreLevel = 5001;")


def ReturnSelectionAndWeights(InputTextFile):

    inputTxt = open(InputTextFile, 'r')
    lines    = inputTxt.readlines()

    Selection = []
    Weight    = []

    for entry in lines:
        line = entry.replace("\n", "")
        if "selection" in line:
            values = line.split("(HT_all * 1.e-5)")
            value_low  = float((values[1].split())[1])/(1e-5)
            value_high = float((values[2].split())[1])/(1e-5)
            Selection.append([line, value_low, value_high])
        if "factor" in line:
            funcExp = line.replace("factor: ", "")
            funcExp = funcExp.replace("HT_all", "x")
            func = TF1(funcExp, funcExp, 0.0, 1000.0)
            Weight.append(func)

    if len(Selection) != len(Weight):
        print("Different number of entries for selection and weight, something is wrong!!!")
        return


    return Selection, Weight


def AddWeightToTree(InputRootFile, TreeName, OutputRootFile, Selection, Weight, WeightName):

    doOverwrite = False

    print("======================================================================================")
    print(InputRootFile)
    print(OutputRootFile)

    if InputRootFile == OutputRootFile:
        OutputRootFile = OutputRootFile.replace(".root", "_temp.root")
        doOverwrite = True

    print(InputRootFile)
    print(OutputRootFile)
    print(doOverwrite)

    fileRoot = TFile(InputRootFile, "READ")
    tree     = fileRoot.Get(TreeName)

    fileRootOut = TFile(OutputRootFile, "RECREATE")
    treeOut = tree.CloneTree(0)

    outWeight = array( 'f', [0.])

    treeOut.Branch(WeightName, outWeight, WeightName+'/F')

    for i in range(0, tree.GetEntries()):

        tree.GetEntry(i)

        evNr = tree.eventNumber
        nJ   = tree.nJets
        mcNr = tree.mcChannelNumber
        HT   = tree.HT_all

        HT_Weight = 1.0

        for j in range(0, len(Selection)):

            if not "nJets == "+str(nJ) in Selection[j][0]:
                continue
            if not str(mcNr) in Selection[j][0]:
                continue
            if HT < Selection[j][1]:
                continue
            if HT > Selection[j][2]:
                continue

            HT_Weight = Weight[j].Eval(HT)

            dev = math.fabs((tree.ht_reweight_nominal-HT_Weight)/HT_Weight*100.0)

            #if dev > 0.001:
            #    print(dev,"   ",tree.ht_reweight_nominal,"  ",HT_Weight)

            #print(tree.ht_reweight_nominal-Weight[j].Eval(HT))/tree.ht_reweight_nominal*100.0)

        outWeight[0] = HT_Weight

        treeOut.Fill()



    treeOut.Write()

    fileRootOut.Close()
    fileRoot.Close()

    if doOverwrite == True:
        os.system("mv "+OutputRootFile+" "+OutputRootFile.replace("_temp.root", ".root"))



# at the moment running this only for 1l !!!
InputTextList  = glob("/work/ws/nemo/fr_ak1148-ttH_Rel21-0/HTReweighting/ht-reweighter/output/*nominal*/reweighting_1l*.txt")
InputFolder    = "/work/ws/nemo/fr_af1100-ttH-0/reweight_samples/L2_nomloose_HT_reweight_new/1l/5j2b/"
OutputFolder   = "/work/ws/nemo/fr_ak1148-ttH_Rel21-0/HTReweighting/FilesAfterReweighting/"

os.system("mkdir -p "+OutputFolder)

# This flag will be appended to the weight name, in case different reweightings are foreseen. Otherwise just leave blank!
FLAG = "_noRescale"
TreeList = ["nominal_Loose"]

InputFiles = []
InputFiles.append([InputFolder+"tt_PowH7_mc16a_AFII.nominal_Loose.root", "nominal"])
InputFiles.append([InputFolder+"tt_PowH7_mc16d_AFII.nominal_Loose.root", "nominal"])
InputFiles.append([InputFolder+"tt_PowH7_mc16e_AFII.nominal_Loose.root", "nominal"])
#InputFiles.append([InputFolder+"tt_PP8_hdamp_UP_mc16a_AFII.nominal_Loose.root", "nominal"])
#InputFiles.append([InputFolder+"tt_PP8_hdamp_UP_mc16d_AFII.nominal_Loose.root", "nominal"])
#InputFiles.append([InputFolder+"tt_PP8_hdamp_UP_mc16e_AFII.nominal_Loose.root", "nominal"])
InputFiles.append([InputFolder+"tt_PP8_mc16a_AFII.nominal_Loose.root", "ALL"])
InputFiles.append([InputFolder+"tt_PP8_mc16a.nominal_Loose.root", "ALL"])
InputFiles.append([InputFolder+"tt_PP8_mc16d_AFII.nominal_Loose.root", "ALL"])
InputFiles.append([InputFolder+"tt_PP8_mc16d.nominal_Loose.root", "ALL"])
InputFiles.append([InputFolder+"tt_PP8_mc16e_AFII.nominal_Loose.root", "ALL"])
InputFiles.append([InputFolder+"tt_PP8_mc16e.nominal_Loose.root", "ALL"])
InputFiles.append([InputFolder+"tt_PP8_pthard1_mc16a_AFII.nominal_Loose.root", "nominal"])
InputFiles.append([InputFolder+"tt_PP8_pthard1_mc16d_AFII.nominal_Loose.root", "nominal"])
InputFiles.append([InputFolder+"tt_PP8_pthard1_mc16e_AFII.nominal_Loose.root", "nominal"])
InputFiles.append([InputFolder+"tt_Sherpa_mc16a.nominal_Loose.root", "nominal"])
InputFiles.append([InputFolder+"tt_Sherpa_mc16d.nominal_Loose.root", "nominal"])
InputFiles.append([InputFolder+"tt_Sherpa_mc16e.nominal_Loose.root", "nominal"])


for entry in InputFiles:
    InputRootFile = entry[0]
    Type = entry[1]

    for InputTextFile in InputTextList:

        for TreeName in TreeList:


            applyWeight = False

            if "nominal" in Type and "nominal" in InputTextFile:
                applyWeight = True
            if "ALL" in Type:
                applyWeight = True

            if not applyWeight:
                continue


            weightName = "ht_reweight_nominal"

            if "var3c_up" in InputTextFile:
                weightName = "ht_reweight_isr_up"
            elif  "var3c_down" in InputTextFile:
                weightName = "ht_reweight_isr_down"
            elif "fsr_up" in InputTextFile:
                weightName = "ht_reweight_fsr_up"
            elif  "fsr_down" in InputTextFile:
                weightName = "ht_reweight_fsr_down"
            elif "muR05_muF05" in InputTextFile:
                weightName = "ht_reweight_scale_down"
            elif  "muR20_muF20" in InputTextFile:
                weightName = "ht_reweight_scale_up"

            finalWeightName = weightName+FLAG

            OutputRootFile = InputRootFile.replace(InputFolder, OutputFolder)

            print(OutputRootFile,"   ",finalWeightName)

            # now check if the weight already exists in root file!!!
            if os.path.exists(OutputRootFile):
                testFile = TFile(OutputRootFile, "READ")
                testTree = testFile.Get(TreeName)

                InputRootFile = OutputRootFile

                if testTree.GetListOfBranches().FindObject(finalWeightName):
                    testFile.Close()
                    continue


            Selections, Weights = ReturnSelectionAndWeights(InputTextFile)

            print(len(Selections),"  ",len(Weights))

            AddWeightToTree(InputRootFile, TreeName, OutputRootFile, Selections, Weights, finalWeightName)



## Introduction

The goal is to reduce the data/MC mis-modelling of $HT_{all}$ for $t\bar{t}+\geq{2b}$ components in the ttH(bb) full Run 2 Legacy analysis. The procedure used here obatins re-weighting factors for the $HT$ distribution through a data-driven reweighting approach, paramterised on $HT_{all}$ and the number of final state jets, denoted ` nJets`. 

#### Running the code :
In order to run the code, you will first need to setup the appropriate enviroment. Inside a clean shell and the cloned repository, run the following :
```
source compile.sh
```
The compilation script will setup the ATLAS enviroment, the 22.2.60 release of AnalysisBase and compile the reweighting code. You will then want to run 
```
source build/*/setup.sh
```
Recompile the code with :
```
source recompile.sh
```
The options used in the code are steered by a configuration file, with the following options available for configuration:

| Option Name             | Description |
| ---                                 | ---                   |
|  basePath                  |     The path to the directory with the MC and data ntuples for reweighting                    |
|  weight                       |  weight string to apply to MC                      |
|   selection                  |  pre-selection you would like to apply to the MC and data samples                      |
|   reweightVar             |  The name of the observable you want to reweight                       |
|   minBinWidth            |   The minimum width of the bin used in the region, e.g 50 GeV for the case of reweighting HT                      |
|   outputFile                |   The name of the output .root file containing the re-weighting factors, stored as a vector.                  |
|     reweightSample                               |   The samples you would like to be used in the reweighting                      |
| constSample | The samples you would like to keep constant in the calculation |

- Two example configuration files are given under the names config_1l.cfg and config_2l.cfg.

Inside the Analysis directory you will find a script that will transform the root output file into a text file with the selection and factors.

#### Method :
Due to the loose pre-selection the analysis utilises, finding an orthogonal region appropriately enriched in $t\bar{t} +\geq1b$ components to derive these re-weighting factors is difficult, in contrast to the approach for $t\bar{t} +\geq1c$ and $t\bar{t} +$light.  As such, the first bins from the following control regions were removed from the profile likelihood fit and combined, in order to generate a new orthogonal region. Since these bins are not very pure in each relevant component of $t\bar{t} +\geq1b$, the overall effect on the results of the backrgound-only fit to data and fit to Asimov data are minimal.

- the $t\bar{t} + B$ region 
- the  $t\bar{t} + 1b$  region 
- the $t\bar{t} +\geq 2b$ region 

This region is defined through the following selection :
```
(((DeepSets_tt_1b_fraction>=0.19 && DeepSets_tt_1b_fraction<= 0.32 && DeepSets_class == 1) || (DeepSets_tt_2b_fraction>=0.19 && DeepSets_tt_2b_fraction<= 0.521 && DeepSets_class == 3) || (DeepSets_ttB_fraction >= 0.19 && DeepSets_ttB_fraction<=0.31 && DeepSets_class == 2)))
```
Here, the `DeepSets_class` refers to the output class of the multi-class classifcation transformer used in the analysis, and is used to remove any overlaps when forming this new region. 

-----------------------------------

For the derivation of the factors, only the tt$+\geq1b$ samples are used. The other samples are removed from the MC, and an equal amount is then subtracted from the data ( i.e we multiply the weights by -1 and combine with the data samples.)

On top of the selection above, it should be noted the intial pre-selection for the region is defined by 
```
5j3b@70 for Single Lepton, 3j3b@85,2b@70 for Dilepton
```
The bins of the distribution in the newly defined orthogonal region used in the re-weighting procedure, that is performed bin-by-bin, are selected based on the following conditions :

1. each bin should contain at least 500 events
2. each bin should be at least 50 GeV wide 

-----------------------------------

The MC samples are first normalised to have the same total as data*, where data* is the data distribution after removing samples other than  tt$+\geq1b$. The re-weighting factors, $k_{i}$, are then taken from the ratio between data* and MC

``` math
 k_{i} = \frac{\text{data*}_{i}}{MC_{i}} 

```

In order to make sure there is a smooth transition between consecutive bins, a smooth function, $\mathcal{f}(HT)$, is chosen for the reweigthing, as opposed to constant factors in each bin. 

A piecewise function composed of $2^{nd}$ order polynomials in each bin is used. This is also required to be continous and have a continous first order derivative. Requiring the piecewise function to be continuous and have a continuous first-order derivative ensures that the resulting smooth curve is well-behaved and does not have any abrupt changes or discontinuities that could introduce artifacts or biases into the reweighting, thus we ensure that the values of the function are smoothly connected across the bin boundaries, resulting in a smooth curve that accurately represents the underlying distribution. 

This is given by :

``` math
 {f}(HT) = a_{i}HT^{2} + b_{i}HT + c_{i}

```

Finally, due to the condition of the number of events in each bin should be the same using the smooth reweighting, we get the following :

``` math
k_{i}\sum_{j}w_{j} = \sum_{j}(a_{i}HT^{2} + b_{i}HT + c_{i})w_{j} \; \; \; \; \text{for} \;  j \in { \text{events in bin} \; i }

```
This sets up a system of linear equations that is solved for $a_{i}$, $b_{i}$ and $c_{i}$.

-----------------------------------

Some alternative to this could be using Kernal Density Estimation (KDE) techniques, or using a ML algorithm to learn a mapping between the data and MC distributions. However, a simple approach is used, for now...

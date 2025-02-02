#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include <cstdio>
#include <TChain.h>
#include <ROOT/RDFHelpers.hxx>
#include <TInterpreter.h>
#include <boost/program_options.hpp>
#include <Eigen/Dense>
#include "Reweighter/Regions.h"

namespace po = boost::program_options;


#define BAR_LENGTH 40

class ProgressBar
{
public:
    ProgressBar(ULong64_t total_entries, const std::string &label) : total(total_entries), label(label){};
    ~ProgressBar()
    {
        operator()(total);
        std::fprintf(stderr, "\n");
    };

    void operator()(ULong64_t processed_entries)
    {
        char bar[BAR_LENGTH + 1] = {0};
        float progress = (float)processed_entries / (float)total;
        progress = progress > 1.f ? 1.f : progress;
        progress = progress < 0.f ? 0.f : progress;
        for (int i = 0; i < BAR_LENGTH * progress - 0.5 && i < BAR_LENGTH; ++i)
            bar[i] = '#';
        std::fprintf(stdout, "\r%s [%-" BOOST_PP_STRINGIZE(BAR_LENGTH) "s] %.0f%%", label.c_str(), bar, 100.f * progress);
        std::fflush(stdout);
    };

private:
    ULong64_t total;
    std::string label;
};

int main(int argc, char *argv[])
{
    // Arguments
    std::string configFile;                     // Config filename
    std::string base_path;                      // Path to sample files
    std::vector<std::string> rew_samples;       // Samples to reweight (filenames)
    std::vector<std::string> const_samples;     // Samples to keep constant (filenames)
    std::vector<std::string> ttlight_samples;   // ttlight samples, also to be kept constant but convininent for splitting (filenames))
    std::vector<std::string> ttc_samples;       // ttc samples, also to be kept constant but convininent for splitting (filenames)
    std::vector<std::string> fakes_samples;     // Fakes samples (filenames)
    std::vector<std::string> data_samples;      // Data samples (filenames)
    std::string output_file;                    // Output filename
    std::vector<std::string> channel_numbers;   // DSID numbers for reweighted samples
    std::string selection;                      // Orthogonal Region Selection
    std::string ttbb_selection;                 // ttbb HF selection
    std::string ttc_selection;                  // ttc HF selection
    std::string ttlight_selection;              // ttlight HF selection
    std::string ttbb_selection_comp;            // tt+bb HF selection components
    std::string ttb_selection_comp;             // tt+b/B HF selection components
    std::string weight_expr;                    // Weight expression
    std::string weight_expr_ttb;                // Weight expression used for the ttb scale variations, since these have separate weight string to ttbar samp
    std::string fakes_weight_expr;              // Fakes weight expression
    std::string reweight_var;                   // Variable to use for reweighting
    std::string ttbarReweight;                  // include reweighting prev. done for ttbar
    std::string NormFactor_ttc;                 // Scaling applied to the ttc Sample
    std::string NormFactor_ttb;                 // Scaling applied to the ttb/B Sample
    std::string NormFactor_ttbb;                // Scaling applied to the ttbb Sample
    std::string NormFactor_ttlight;             // Scaling applied to the ttlight Sample
    float min_bin_width;                        // Minimum width of histogram bins




    po::options_description commandline("Command-line options");
    commandline.add_options()                                                    //
        ("help,h", "Produce this help message")                                  //
        ("configFile,c", po::value(&configFile), "The name of the config file"); //

    po::options_description config("Configuration");
    config.add_options()                                                                                                 //
        ("selection", po::value(&selection), "Event selection")                                                          //
        ("basePath", po::value(&base_path), "Path to ntuples")                                                           //
        ("reweightVar", po::value(&reweight_var), "Variable to reweight")                                                //
        ("minBinWidth", po::value(&min_bin_width), "Minimum width of rew. bins")                                         //
        ("mcChannelNumbers", po::value(&channel_numbers)->multitoken(), "DSID numbers for reweighted samples")           //
        ("reweightSample", po::value(&rew_samples)->multitoken(), "List of ttbb filenames to reweight.")                 //
        ("constSample", po::value(&const_samples)->multitoken(), "List of filenames not to be reweighted.")              //
        ("ttlightSample", po::value(&ttlight_samples)->multitoken(), "List of ttlight filenames not to be reweighted.")  //
        ("ttcSample", po::value(&ttc_samples)->multitoken(), "List of ttc filenames not to be reweighted.")              //
        ("fakesSample", po::value(&fakes_samples)->multitoken(), "List of fakes samples not to be reweighted.")          //
        ("dataSample", po::value(&data_samples)->multitoken(), "List of filenames to use as data.")                      //
        ("weight", po::value(&weight_expr), "MC weight expression")                                                      //
        ("weight_ttb", po::value(&weight_expr_ttb), "MC weight expression for ttb scales")                               //
        ("fakes_weight", po::value(&fakes_weight_expr), "Data driven fake estimation weights")                           //
        ("outputFile", po::value(&output_file)->default_value("out.root"), "Output filename")                            //
        ("ttbb_selection", po::value(&ttbb_selection), "tt+bb sample HF selection")                                      //
        ("ttc_selection", po::value(&ttc_selection), "ttc HF selection")                                                 //
        ("ttlight_selection", po::value(&ttlight_selection), "ttlight HF selection")                                     //
        ("ttbb_selection_comp", po::value(&ttbb_selection_comp), "tt+bb HF selection components")                        //
        ("ttb_selection_comp", po::value(&ttb_selection_comp), "tt+b/B HF selection components")                             //
        ("ttbarReweight", po::value(&ttbarReweight), "including previous reweighting done for ttc/ttlight")              //
        ("NormFactor_ttc", po::value(&NormFactor_ttc), "Scaling the ttc sample yield by the post-fit value")             //
        ("NormFactor_ttb", po::value(&NormFactor_ttb), "Scaling the ttb sample yield by the post-fit value")             //
        ("NormFactor_ttbb", po::value(&NormFactor_ttbb), "Scaling the ttbb sample yield by the post-fit value")          //
        ("NormFactor_ttlight", po::value(&NormFactor_ttlight), "Scaling the ttlight sample yield by the post-fit value"); //

    po::options_description cmdline_options;
    cmdline_options.add(commandline).add(config);

    po::options_description config_file_options;
    config_file_options.add(config);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(po::positional_options_description()).run(), vm);
    po::notify(vm);

    if (configFile.empty())
    {
        std::cerr << "ERROR: Config file is required" << std::endl;
        return EXIT_FAILURE;
    }
    std::ifstream ifs(configFile.c_str());
    po::store(po::parse_config_file(ifs, config_file_options), vm);
    po::notify(vm);

    const std::vector<std::string> Cuts = {
        "nJets == 5",
        "nJets == 6",
        "nJets == 7",
        "nJets >= 8",
        //"nJets >= 9",
    };

    /*
       ================================================
        Compute bins for reweighting using rew samples
       ================================================
    */
    std::cout << "\033[1;32m===================================================================" << std::endl;
    std::cout << "The Orthogonal Region being used for the derivation is defined by :" << std::endl;
    std::cout << "====================================================================\033[0m" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << selection << std::endl;
    std::cout << "==================================" << std::endl;

    std::map<std::string, std::vector<float>> rew_bins;
    {   std::cout << "\033[1;32m==================================" << std::endl;
        std::cout << "The samples used for defining bins" << std::endl;
        std::cout << "==================================\033[0m" << std::endl;
        TChain chain("nominal_Loose");
        for (auto &r : region) // Loop over the regions defined in the header file
            {
            for (auto &s : rew_samples) // Loop over the rew samples to be included
                {
            std::string path = base_path + "/" + r + "/" + s + ".root";
            std::cout << path << std::endl;
            chain.Add(path.c_str());
                }
            }
        int n_entries = chain.GetEntries();
        std::cout << n_entries << std::endl;
        std::cout << "\033[1;32m=======================" << std::endl;
        std::cout << "Computing the bins...." << std::endl;
        std::cout << "=======================\033[0m" << std::endl;
        for (const std::string &cut : Cuts)
        {
            ROOT::RDF::RNode df = ROOT::RDataFrame(chain);

            /* Display progress */
            //ROOT::RDF::RResultPtr<ULong64_t> count_result = df.Count();
            //ProgressBar pb(n_entries, "Computing bins (" + cut + ")");
            //count_result.OnPartialResult(n_entries / 100, std::ref(pb));
            /* */
            std::cout << "Number of events in ttbb sample before deriv. region: " << df.Count().GetValue() << std::endl;
            if (!selection.empty())
                df = df.Filter(selection);
            std::cout << "Number of events in ttbb sample after deriv. region: " << df.Count().GetValue() << std::endl;
            // check our selection is working as intended
            df = df.Filter(cut + " && " + "HF_SimpleClassification == 1");
            df = df.Define("x", "(float)(" + reweight_var + ")");
            // special weight selection in the case we are reweigthing ttb scale variations
            df = df.Define("w", "(float)(" + (weight_expr_ttb.empty() ? weight_expr : weight_expr_ttb) + ")");
            std::cout << "Number of events passing HF (1) selection: " << df.Count().GetValue() << std::endl;
            auto n = df.Take<int>("nJets");
            auto v = df.Take<float>("x");
            auto w = df.Take<float>("w");

            std::vector<int> rew_nJets = n.GetValue();
            std::vector<float> rew_var = v.GetValue();
            std::vector<float> rew_weights = w.GetValue();

            /* Compute bins */
            float min_n = 500.f;
            std::vector<size_t> idx(rew_var.size());
            std::iota(idx.begin(), idx.end(), 0);
            std::sort(idx.begin(), idx.end(), [&rew_var](size_t i1, size_t i2)
                      { return rew_var[i1] < rew_var[i2]; });

            float lower_bound = rew_var[idx[0]];
            std::vector<float> bins = {lower_bound};
            float w_sum = 0;
            for (size_t i = 0; i < rew_var.size(); ++i)
            {
                if (rew_var[idx[i]] - lower_bound >= min_bin_width && w_sum >= min_n)
                {
                    lower_bound = rew_var[idx[i]];
                    w_sum = 0;
                    bins.push_back(lower_bound);
                }
                w_sum += rew_weights[idx[i]];
            }
            if (bins.back() < rew_var[idx.back()])
                bins.push_back(rew_var[idx.back()]);

            rew_bins[cut] = std::move(bins);
        }
    }
    /* */

    /* Compute reweighting */
    std::vector<std::string> rew_selection;
    std::vector<std::string> rew_factor;
    std::vector<std::string> vec_bin_str;
    for (auto &cut : Cuts)
    {
        std::cout << std::endl;
        std::cout << "Processing " << cut << ":" << std::endl;
        auto &bins = rew_bins[cut];
        int n_bins = bins.size() - 1;

        // Initialise histograms for computing reweighting factors
        TH1D rew_hist_ttbb, rew_hist_ttb, rew_hist, const_hist_ttlight, const_hist_ttc, const_hist_fakes_ee, const_hist_fakes_mumu, const_hist, data_hist;

        // Integrals for reweighting interpolation
        std::vector<float> fdx(n_bins);
        std::vector<float> fxdx(n_bins);
        std::vector<float> fx2dx(n_bins);

        /* Get rew. histogram for tt+bb and integrals*/
        {
        TChain chain("nominal_Loose");
        for (auto &r : region) // Loop over the regions defined in the header file
            {
                for (auto &s : rew_samples) // Loop over the rew samples to be included
                    {
                std::string path = base_path + "/" + r + "/" + s + ".root";
                chain.Add(path.c_str());
            }
            }
            int n_entries = chain.GetEntries();

            ROOT::RDF::RNode df = ROOT::RDataFrame(chain);

            /* Display progress */
            //ROOT::RDF::RResultPtr<ULong64_t> count_result = df.Count();
            //ProgressBar pb(n_entries, "  Creating rew. ttbb hist.");
            //count_result.OnPartialResult(n_entries / 100, std::ref(pb));
            /* */

            if (!selection.empty())
                df = df.Filter(selection);
            //std::cout << "Number of events before ttbb selection: " << df.Count().GetValue() << std::endl;
            // if (!ttbb_selection.empty())
            //     df = df.Filter(ttbb_selection);
            // if (!ttbb_selection_comp.empty())
            //     df = df.Filter(ttbb_selection_comp);
            // check our selection is working as intended
            //std::cout << "Number of events passing selection: " << df.Count().GetValue() << std::endl;
            df = df.Filter(cut + " && " + "HF_SimpleClassification == 1" +  " && " + "((HF_Classification >= 200 && HF_Classification < 1000) || (HF_Classification >= 1100))");
            std::cout << "Number of events passing tt+bb comp selection: " << df.Count().GetValue() << std::endl;
            df = df.Define("x", "(float)(" + reweight_var + ")");
            // special weight selection in the case we are reweigthing ttb scale variations
            df = df.Define("w", "(float)(" + (weight_expr_ttb.empty() ? weight_expr : weight_expr_ttb) + " * " + NormFactor_ttbb + ")");
            // df = df.Define("w", "(float)(" + weight_expr + " * " + NormFactor_ttbb + ")"); // old for posterity
            df = df.Define("wx", "w * x");
            df = df.Define("wx2", "wx * x");
            auto hist_fdx = df.Histo1D<float>({"", "", n_bins, bins.data()}, "x", "w");
            auto hist_fxdx = df.Histo1D<float>({"", "", n_bins, bins.data()}, "x", "wx");
            auto hist_fx2dx = df.Histo1D<float>({"", "", n_bins, bins.data()}, "x", "wx2");
            rew_hist_ttbb = hist_fdx.GetValue();
            rew_hist_ttbb.Print();


            for (int i = 0; i < n_bins; ++i)
            {
                fdx[i] = hist_fdx->GetBinContent(i + 1);
                fxdx[i] = hist_fxdx->GetBinContent(i + 1);
                fx2dx[i] = hist_fx2dx->GetBinContent(i + 1);
            }
        }

        /* Get rew. histogram for ttb/B and integrals*/
        {
        TChain chain("nominal_Loose");
        for (auto &r : region) // Loop over the regions defined in the header file
            {
                for (auto &s : rew_samples) // Loop over the rew samples to be included
                    {
                std::string path = base_path + "/" + r + "/" + s + ".root";
                chain.Add(path.c_str());
            }
            }
            int n_entries = chain.GetEntries();

            ROOT::RDF::RNode df = ROOT::RDataFrame(chain);

            /* Display progress */
            //ROOT::RDF::RResultPtr<ULong64_t> count_result = df.Count();
            //ProgressBar pb(n_entries, "  Creating rew. ttb/B hist.");
            //count_result.OnPartialResult(n_entries / 100, std::ref(pb));
            /* */

            if (!selection.empty())
                df = df.Filter(selection);
            //std::cout << "Number of events before ttbb selection: " << df.Count().GetValue() << std::endl;
            // if (!ttbb_selection.empty())
            //     df = df.Filter(ttbb_selection);
            // if (!ttb_selection_comp.empty())
            //     df = df.Filter(ttb_selection_comp);
            // check our selection is working as intended
            //std::cout << "Number of events passing selection: " << df.Count().GetValue() << std::endl;
            df = df.Filter(cut + " && " + "HF_SimpleClassification == 1" + " && " + "((HF_Classification >= 100 && HF_Classification < 200) || (HF_Classification >= 1000 && HF_Classification < 1100))");
            std::cout << "Number of events passing tt+1b/B comp selection: " << df.Count().GetValue() << std::endl;
            df = df.Define("x", "(float)(" + reweight_var + ")");
            df = df.Define("w", "(float)(" + (weight_expr_ttb.empty() ? weight_expr : weight_expr_ttb) + " * " + NormFactor_ttbb + ")");
            //df = df.Define("w", "(float)(" + weight_expr + " * " + NormFactor_ttb + ")");
            std::cout << "Number of tt+1b/B events after RW + weight_expr + NF " << df.Count().GetValue() << std::endl;
            df = df.Define("wx", "w * x");
            df = df.Define("wx2", "wx * x");
            auto hist_fdx = df.Histo1D<float>({"", "", n_bins, bins.data()}, "x", "w");
            auto hist_fxdx = df.Histo1D<float>({"", "", n_bins, bins.data()}, "x", "wx");
            auto hist_fx2dx = df.Histo1D<float>({"", "", n_bins, bins.data()}, "x", "wx2");
            rew_hist_ttb = hist_fdx.GetValue();
            rew_hist_ttb.Print();

            for (int i = 0; i < n_bins; ++i)
            {
                fdx[i] = hist_fdx->GetBinContent(i + 1);
                fxdx[i] = hist_fxdx->GetBinContent(i + 1);
                fx2dx[i] = hist_fx2dx->GetBinContent(i + 1);
            }
        }
        /* */

        /*
             ============================
             Get Const. sample Histograms
             ============================
        */

        /*  ==================
            The ttlight Sample
            ==================

        // - Apply HF selection
        // - Apply already derived reweighting

        */

        {
            TChain chain("nominal_Loose");
                for (auto &r : region) // Loop over the regions defined in the header file
                {
                    for (auto &s : ttlight_samples) // Loop over the new samples to be included
                    {
                    std::string path = base_path + "/" + r + "/" + s + ".root";

                        chain.Add(path.c_str());
                    }
                }
            int n_entries = chain.GetEntries();

            ROOT::RDF::RNode df = ROOT::RDataFrame(chain);

            /* Display progress */
            //ROOT::RDF::RResultPtr<ULong64_t> count_result = df.Count();
            //ProgressBar pb(n_entries, "  Creating const. ttlight hist.");
            //count_result.OnPartialResult(n_entries / 100, std::ref(pb));
            /* */
            std::cout << "Number of ttbar events before selection: " << df.Count().GetValue() << std::endl;
            if (!selection.empty())
                df = df.Filter(selection);
            std::cout << "Number of ttbar events passing selection: " << df.Count().GetValue() << std::endl;
            if (!ttlight_selection.empty())
                df.Filter(ttlight_selection);
            // check our selection is working as intended
            //std::cout << "Number of ttlight events passing selection: " << df.Count().GetValue() << std::endl;

            df = df.Filter(cut + " && " + "HF_SimpleClassification == 0");
            std::cout << "Number of ttlight events passing selection: " << df.Count().GetValue() << std::endl;
            df = df.Define("x", "(float)(" + reweight_var + ")");
            df = df.Define("w", "(float)(" + weight_expr + " * " + ttbarReweight + " * " + NormFactor_ttlight + ")"); // add the ht_rew already derived and ttlight normalisation
            std::cout << "Number of ttlight events after RW + weight_expr + NF " << df.Count().GetValue() << std::endl;
            const_hist_ttlight = df.Histo1D<float>({"", "", n_bins, bins.data()}, "x", "w").GetValue();
            const_hist_ttlight.Print();

        }
        /*  ===============
            The ttc Sample
            ===============
            - Apply HF selection
            - Apply already derived reweighting
            - Apply post-fit scale factor
        */

        {
            TChain chain("nominal_Loose");
                for (auto &r : region) // Loop over the regions defined in the header file
                {
                    for (auto &s : ttc_samples) // Loop over the new samples to be included
                    {
                    std::string path = base_path + "/" + r + "/" + s + ".root";

                        chain.Add(path.c_str());
                    }
                }
            int n_entries = chain.GetEntries();

            ROOT::RDF::RNode df = ROOT::RDataFrame(chain);

            /* Display progress */
            //ROOT::RDF::RResultPtr<ULong64_t> count_result = df.Count();
            //ProgressBar pb(n_entries, "  Creating const. ttc hist.");
            //count_result.OnPartialResult(n_entries / 100, std::ref(pb));
            /* */
            std::cout << "Number of ttbar events before selection: " << df.Count().GetValue() << std::endl;
            if (!selection.empty())
                df = df.Filter(selection);
            std::cout << "Number of ttbar events passing selection: " << df.Count().GetValue() << std::endl;
            if (!ttc_selection.empty())
                df.Filter(ttc_selection);
            // check our selection is working as intended
            //std::cout << "Number of ttc events passing selection: " << df.Count().GetValue() << std::endl;
            // " * NormFactor_ttc)"
            df = df.Filter(cut + " && " + "HF_SimpleClassification == -1");
            std::cout << "Number of ttc events passing selection: " << df.Count().GetValue() << std::endl;
            df = df.Define("x", "(float)(" + reweight_var + ")");
            df = df.Define("w", "(float)(" + weight_expr + " * " + ttbarReweight + " * " + NormFactor_ttc + ")"); // add the ht_rew already derived and ttc normalisation
            std::cout << "Number of ttc events after RW + weight_expr + NF " << df.Count().GetValue() << std::endl;
            const_hist_ttc = df.Histo1D<float>({"", "", n_bins, bins.data()}, "x", "w").GetValue();
            const_hist_ttc.Print();
        }
        /*  =================
            The Fakes Samples
            =================
                - apply fakes weight
                - DO NOT apply other weight string
                - apply e/mu loose selection

        */

                /*
                    ===================
                    The ee fakes sample
                    ===================
                */
        {
            TChain chain("nominal_Loose");
                for (auto &r : region) // Loop over the regions defined in the header file
                {
                    for (auto &s : fakes_samples) // Loop over the new samples to be included
                    {
                    std::string path = base_path + "/" + r + "/" + s + ".root";

                        chain.Add(path.c_str());
                    }
                }
            int n_entries = chain.GetEntries();

            ROOT::RDF::RNode df = ROOT::RDataFrame(chain);

            /* Display progress */
            ROOT::RDF::RResultPtr<ULong64_t> count_result = df.Count();
            ProgressBar pb(n_entries, "  Creating const. ee fake hist.");
            count_result.OnPartialResult(n_entries / 100, std::ref(pb));
            /* */
            if (!selection.empty())
                df = df.Filter(selection);

            df = df.Define("passCut", "Sum(el_isTight == 0)");
            df = df.Filter(cut + " && " + "nElectrons == 1" + " &&" + "passCut");
            df = df.Define("x", "(float)(" + reweight_var + ")");
            df = df.Define("w", "(float)(" + fakes_weight_expr + ")");
            const_hist_fakes_ee = df.Histo1D<float>({"", "", n_bins, bins.data()}, "x", "w").GetValue();
            const_hist_fakes_ee.Print();
        }

                /*
                    =====================
                    The mumu fakes sample
                    =====================
                */
        {
            TChain chain("nominal_Loose");
                for (auto &r : region) // Loop over the regions defined in the header file
                {
                    for (auto &s : fakes_samples) // Loop over the new samples to be included
                    {
                    std::string path = base_path + "/" + r + "/" + s + ".root";

                        chain.Add(path.c_str());
                    }
                }
            int n_entries = chain.GetEntries();

            ROOT::RDF::RNode df = ROOT::RDataFrame(chain);

            /* Display progress */
            ROOT::RDF::RResultPtr<ULong64_t> count_result = df.Count();
            ProgressBar pb(n_entries, "  Creating const. mumu fake hist.");
            count_result.OnPartialResult(n_entries / 100, std::ref(pb));
            /* */
            if (!selection.empty())
                df = df.Filter(selection);

            df = df.Define("passCut", "Sum(mu_isTight == 0)");
            df = df.Filter(cut + " && " + "nMuons == 1" + " &&" + "passCut");
            df = df.Define("x", "(float)(" + reweight_var + ")");
            df = df.Define("w", "(float)(" + fakes_weight_expr + ")");
            const_hist_fakes_mumu = df.Histo1D<float>({"", "", n_bins, bins.data()}, "x", "w").GetValue();
            const_hist_fakes_mumu.Print();
        }
        /*
            ========================
            All other const. samples
            ========================

        */

        {
            TChain chain("nominal_Loose");
                for (auto &r : region) // Loop over the regions defined in the header file
                {
                    for (auto &s : const_samples) // Loop over the new samples to be included
                    {
                    std::string path = base_path + "/" + r + "/" + s + ".root";

                        chain.Add(path.c_str());
                    }
                }
            int n_entries = chain.GetEntries();

            ROOT::RDF::RNode df = ROOT::RDataFrame(chain);

            /* Display progress */
            ROOT::RDF::RResultPtr<ULong64_t> count_result = df.Count();
            ProgressBar pb(n_entries, "  Creating const. hist.");
            count_result.OnPartialResult(n_entries / 100, std::ref(pb));
            /* */

            if (!selection.empty())
                df = df.Filter(selection);

            df = df.Filter(cut);
            df = df.Define("x", "(float)(" + reweight_var + ")");
            df = df.Define("w", "(float)(" + weight_expr + ")");
            const_hist = df.Histo1D<float>({"", "", n_bins, bins.data()}, "x", "w").GetValue();
            const_hist.Print();
        }
        /* */

        /*
           ==========================
           Get Data sample Histograms
           ==========================
        */

        {
            TChain chain("nominal_Loose");
            for (auto &r : region) // Loop over the regions defined in the header file
            {
                for (auto &s : data_samples) // Loop over the new samples to be included
                {
                std::string path = base_path + "/" + r + "/" + s + ".root";

                chain.Add(path.c_str());
                }
            }
            int n_entries = chain.GetEntries();

            ROOT::RDF::RNode df = ROOT::RDataFrame(chain);

            /* Display progress */
            ROOT::RDF::RResultPtr<ULong64_t> count_result = df.Count();
            ProgressBar pb(n_entries, "  Creating data hist.");
            count_result.OnPartialResult(n_entries / 100, std::ref(pb));
            /* */

            if (!selection.empty())
                df = df.Filter(selection);
            df = df.Filter(cut);
            df = df.Define("x", "(float)(" + reweight_var + ")");
            data_hist = df.Histo1D<float>({"", "", n_bins, bins.data()}, "x").GetValue();
            std::cout << "Data Histogram : " << std::endl;
            data_hist.Print();
        }
        /* */

        // Substract all const. values from data histogram
        // might be better to carry out sequentiially and have errors for each one

        if (!data_hist.Add(&const_hist, -1.))
        {
            std::cerr << "ERROR: Constant Histogram addition failed" << std::endl;
            return EXIT_FAILURE;
        }

        if (!data_hist.Add(&const_hist_ttlight, -1.))
        {
            std::cerr << "ERROR: ttlight Histogram addition failed" << std::endl;
            return EXIT_FAILURE;
        }

        if (!data_hist.Add(&const_hist_ttc, -1.))
        {
            std::cerr << "ERROR: ttc Histogram addition failed" << std::endl;
            return EXIT_FAILURE;
        }

        if (!data_hist.Add(&const_hist_fakes_ee, -1.))
        {
            std::cerr << "ERROR: ee Fakes Histogram addition failed" << std::endl;
            return EXIT_FAILURE;
        }
        if (!data_hist.Add(&const_hist_fakes_mumu, -1.))
        {
            std::cerr << "ERROR: mumu Fakes Histogram addition failed" << std::endl;
            return EXIT_FAILURE;
        }
        data_hist.Print(); // Should be equal to Data - (const_ttc + const_ttlight + const_fakes + const_other)

        // Combine ttbb and ttb histograms

        std::cout << "Before adding ttb to ttbb" << std::endl;
        rew_hist = rew_hist_ttbb;
        rew_hist.Print();
        rew_hist.Add(&rew_hist_ttb);
        std::cout << "After adding ttb to ttbb" << std::endl;
        rew_hist.Print();

        // Normalize histograms
        data_hist.Scale(1. / data_hist.Integral());
        rew_hist.Scale(1. / rew_hist.Integral());
        //rew_hist_ttbb.Scale(1. / rew_hist_ttbb.Integral());
        //rew_hist_ttb.Scale(1. / rew_hist_ttb.Integral());
        std::cout << "============================" << std::endl;
        std::cout << "Histograms Normalised......." << std::endl;
        std::cout << "============================" << std::endl;
        rew_hist.Print();
        data_hist.Print();

        // Compute ratio
        if (!data_hist.Divide(&rew_hist))
        {
            std::cerr << "ERROR: Histogram division failed" << std::endl;
            return EXIT_FAILURE;
        }

        // Print the computed ratio
        std::cout << "Computed ratio....: ";
        for (int i = 1; i <= n_bins; ++i)
        {
            float ratio = data_hist.GetBinContent(i);
            std::cout << ratio << " ";
        }
        std::cout << std::endl;

        std::vector<float> factors;
        for (int i = 1; i <= n_bins; ++i)
            factors.push_back(data_hist.GetBinContent(i));

        /* Interpolate reweighting */
        std::cout << "  Interpolating ..." << std::endl;

        // The polynomial coefficients are the solution of Ax = y
        Eigen::MatrixXd A(3 * n_bins, 3 * n_bins);
        Eigen::VectorXd y(3 * n_bins);

        int eqn_idx = 0;

        // Area condition
        for (int cond_idx = 0; cond_idx < n_bins; ++cond_idx, ++eqn_idx)
        {
            for (int i = 0; i < n_bins; ++i)
            {
                if (i == cond_idx)
                {
                    A(eqn_idx, i + 0 * n_bins) = fx2dx[cond_idx] / fdx[cond_idx];
                    A(eqn_idx, i + 1 * n_bins) = fxdx[cond_idx] / fdx[cond_idx];
                    A(eqn_idx, i + 2 * n_bins) = 1.f;
                }
                else
                {
                    A(eqn_idx, i + 0 * n_bins) = 0.f;
                    A(eqn_idx, i + 1 * n_bins) = 0.f;
                    A(eqn_idx, i + 2 * n_bins) = 0.f;
                }
            }
            y(eqn_idx) = factors[cond_idx];
        }

        // Continuity condition
        for (int cond_idx = 0; cond_idx < n_bins - 1; ++cond_idx, ++eqn_idx)
        {
            for (int i = 0; i < n_bins; ++i)
            {
                if (i == cond_idx)
                {
                    A(eqn_idx, i + 0 * n_bins) = bins[cond_idx + 1] * bins[cond_idx + 1];
                    A(eqn_idx, i + 1 * n_bins) = bins[cond_idx + 1];
                    A(eqn_idx, i + 2 * n_bins) = 1.f;
                }
                else if (i == cond_idx + 1)
                {
                    A(eqn_idx, i + 0 * n_bins) = -bins[cond_idx + 1] * bins[cond_idx + 1];
                    A(eqn_idx, i + 1 * n_bins) = -bins[cond_idx + 1];
                    A(eqn_idx, i + 2 * n_bins) = -1.f;
                }
                else
                {
                    A(eqn_idx, i + 0 * n_bins) = 0.f;
                    A(eqn_idx, i + 1 * n_bins) = 0.f;
                    A(eqn_idx, i + 2 * n_bins) = 0.f;
                }
            }
            y(eqn_idx) = 0.f;
        }

        // Smoothness condition
        for (int cond_idx = 0; cond_idx < n_bins - 1; ++cond_idx, ++eqn_idx)
        {
            for (int i = 0; i < n_bins; ++i)
            {
                if (i == cond_idx)
                {
                    A(eqn_idx, i + 0 * n_bins) = 2.f * bins[cond_idx + 1];
                    A(eqn_idx, i + 1 * n_bins) = 1.f;
                    A(eqn_idx, i + 2 * n_bins) = 0.f;
                }
                else if (i == cond_idx + 1)
                {
                    A(eqn_idx, i + 0 * n_bins) = -2.f * bins[cond_idx + 1];
                    A(eqn_idx, i + 1 * n_bins) = -1.f;
                    A(eqn_idx, i + 2 * n_bins) = 0.f;
                }
                else
                {
                    A(eqn_idx, i + 0 * n_bins) = 0.f;
                    A(eqn_idx, i + 1 * n_bins) = 0.f;
                    A(eqn_idx, i + 2 * n_bins) = 0.f;
                }
            }
            y(eqn_idx) = 0.f;
        }

        // Boundary conditions
        for (int i = 0; i < n_bins; ++i)
        {
            if (i == 0)
            {
                A(eqn_idx, i + 0 * n_bins) = 1.f;
                A(eqn_idx, i + 1 * n_bins) = 0.f;
                A(eqn_idx, i + 2 * n_bins) = 0.f;
            }
            else
            {
                A(eqn_idx, i + 0 * n_bins) = 0.f;
                A(eqn_idx, i + 1 * n_bins) = 0.f;
                A(eqn_idx, i + 2 * n_bins) = 0.f;
            }
        }
        y(eqn_idx) = 0.f;
        ++eqn_idx;

        for (int i = 0; i < n_bins; ++i)
        {
            if (i == n_bins - 1)
            {
                A(eqn_idx, i + 0 * n_bins) = 1.f;
                A(eqn_idx, i + 1 * n_bins) = 0.f;
                A(eqn_idx, i + 2 * n_bins) = 0.f;
            }
            else
            {
                A(eqn_idx, i + 0 * n_bins) = 0.f;
                A(eqn_idx, i + 1 * n_bins) = 0.f;
                A(eqn_idx, i + 2 * n_bins) = 0.f;
            }
        }
        y(eqn_idx) = 0.f;
        ++eqn_idx;

        // Solve system of linear equations
        Eigen::VectorXd x = A.colPivHouseholderQr().solve(y);

        // Split coefficients
        std::vector<float> a(x.data() + 0 * n_bins, x.data() + 1 * n_bins);
        std::vector<float> b(x.data() + 1 * n_bins, x.data() + 2 * n_bins);
        std::vector<float> c(x.data() + 2 * n_bins, x.data() + 3 * n_bins);

        // Init string for bins
        std::string bin_str = "(" + cut + ")" + " (mcChannelNumber == ";
        for (const std::string &chan_num : channel_numbers) {
            if (chan_num == channel_numbers.back()) {
                bin_str += chan_num + "): " + std::to_string(bins[0] * 100);
            } else {
                bin_str += chan_num + ", ";
            }
        }

        std::string var_str = "(" + reweight_var + ")";
        for (int i = 0; i < n_bins; ++i)
        {
            std::string sel = "(" + cut + ") && (";
            for (const std::string &chan_num : channel_numbers) {
                if (chan_num == channel_numbers.back()) {
                    sel += "(mcChannelNumber == " + chan_num + ")";
                } else {
                    sel += "(mcChannelNumber == " + chan_num + ") || ";
                }
            }
            sel += ") && " + var_str + " >= " + std::to_string(bins[i]) + " && " + var_str + " < " + std::to_string(bins[i + 1]);
            bin_str += std::to_string(bins[i + 1] * 100) + ", ";

            rew_selection.push_back(sel);

            std::ostringstream sout;
            sout << std::setprecision(8);
            sout << a[i] << " * " + var_str + " * " + var_str + " + "
                 << b[i] << " * " + var_str + " + " << c[i];
            rew_factor.push_back(sout.str());
        }
        vec_bin_str.push_back(bin_str);
    }


    TFile *f = TFile::Open(output_file.c_str(), "RECREATE");
    if (!f)
    {
        std::cerr << "ERROR: Failed to create output file" << std::endl;
        return EXIT_FAILURE;
    }
    // Print the values of the vectors to verify the procedure is working
    std::cout << "Printing values of the 'rew_selection' vector:" << std::endl;
    for (const auto& val : rew_selection) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    std::cout << "Printing values of the 'rew_factor' vector:" << std::endl;
    for (const auto& val : rew_factor) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    f->WriteObject(&rew_selection, "selection");
    f->WriteObject(&rew_factor, "factors");
    f->WriteObject(&vec_bin_str, "binning");

    f->Write();
    f->Close();
    return EXIT_SUCCESS;
}
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstdio>
#include <TChain.h>
#include <ROOT/RDFHelpers.hxx>
#include <TInterpreter.h>
#include <boost/program_options.hpp>
#include <Eigen/Dense>

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
        std::fprintf(stderr, "\r%s [%-" BOOST_PP_STRINGIZE(BAR_LENGTH) "s] %.0f%%", label.c_str(), bar, 100.f * progress);
    };

private:
    ULong64_t total;
    std::string label;
};

int main(int argc, char *argv[])
{
    // Arguments
    std::string configFile;                 // Config filename
    std::string base_path;                  // Path to sample files
    std::vector<std::string> rew_samples;   // Samples to reweight (filenames)
    std::vector<std::string> const_samples; // Samples to keep constant (filenames)
    std::vector<std::string> data_samples;  // Data samples (filenames)
    std::string outputFile;                 // Output filename
    std::string selection;                  // Selection
    std::string weight_expr;                // Weight expression
    std::string reweight_var;               // Variable to use for reweighting
    float min_bin_width;                    // Minimum width of histogram bins

    po::options_description commandline("Command-line options");
    commandline.add_options()                                                    //
        ("help,h", "Produce this help message")                                  //
        ("configFile,c", po::value(&configFile), "The name of the config file"); //

    po::options_description config("Configuration");
    config.add_options()                                                                                    //
        ("selection", po::value(&selection), "Event selection")                                             //
        ("basePath", po::value(&base_path), "Path to ntuples")                                              //
        ("reweightVar", po::value(&reweight_var), "Variable to reweight")                                   //
        ("minBinWidth", po::value(&min_bin_width), "Minimum width of rew. bins")                            //
        ("reweightSample", po::value(&rew_samples)->multitoken(), "List of filenames to reweight.")         //
        ("constSample", po::value(&const_samples)->multitoken(), "List of filenames not to be reweighted.") //
        ("dataSample", po::value(&data_samples)->multitoken(), "List of filenames to use as data.")         //
        ("weight", po::value(&weight_expr), "MC weight expression");                                        //

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

    const std::map<int, std::string> nJetsCuts = {
        {5, "nJets == 5"},
        {6, "nJets == 6"},
        {7, "nJets == 7"},
        {8, "nJets == 8"},
        {9, "nJets >= 9"},
    };

    /* Compute bins for reweighting */
    std::map<int, std::vector<float>> rew_bins;
    {
        TChain chain("nominal_Loose");
        for (auto &s : rew_samples)
        {
            std::string path = base_path + "/" + s + ".root";
            chain.Add(path.c_str());
        }
        int n_entries = chain.GetEntries();

        for (auto cut : nJetsCuts)
        {
            ROOT::RDF::RNode df = ROOT::RDataFrame(chain);

            /* Display progress */
            ROOT::RDF::RResultPtr<ULong64_t> count_result = df.Count();
            ProgressBar pb(n_entries, "Computing bins (" + cut.second + ")");
            count_result.OnPartialResult(n_entries / 100, std::ref(pb));
            /* */

            if (!selection.empty())
                df = df.Filter(selection);
            df = df.Filter(cut.second);
            df = df.Define("x", "(float)(" + reweight_var + ")");
            df = df.Define("w", "(float)(" + weight_expr + ")");

            auto n = df.Take<int>("nJets");
            auto v = df.Take<float>("x");
            auto w = df.Take<float>("w");

            std::vector<int> rew_nJets = n.GetValue();
            std::vector<float> rew_var = v.GetValue();
            std::vector<float> rew_weights = w.GetValue();

            /* Compute bins */
            float min_n = 5000.f;
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

            rew_bins[cut.first] = std::move(bins);
        }
    }
    /* */

    /* Compute reweighting */
    std::map<int, std::vector<float>> rew_factors;
    for (auto cut : nJetsCuts)
    {
        std::cerr << std::endl;
        std::cerr << "Processing " << cut.second << ":" << std::endl;
        auto &bins = rew_bins[cut.first];
        int n_bins = bins.size() - 1;

        // Histograms for computing reweighting factors
        TH1D rew_hist, const_hist, data_hist;

        // Integrals for reweighting interpolation
        std::vector<float> fdx(n_bins);
        std::vector<float> fxdx(n_bins);
        std::vector<float> fx2dx(n_bins);

        /* Get rew. histogram and integrals*/
        {
            TChain chain("nominal_Loose");
            for (auto &s : rew_samples)
            {
                std::string path = base_path + "/" + s + ".root";
                chain.Add(path.c_str());
            }
            int n_entries = chain.GetEntries();

            ROOT::RDF::RNode df = ROOT::RDataFrame(chain);

            /* Display progress */
            ROOT::RDF::RResultPtr<ULong64_t> count_result = df.Count();
            ProgressBar pb(n_entries, "  Creating rew. hist.");
            count_result.OnPartialResult(n_entries / 100, std::ref(pb));
            /* */

            if (!selection.empty())
                df = df.Filter(selection);
            df = df.Filter(cut.second);
            df = df.Define("x", "(float)(" + reweight_var + ")");
            df = df.Define("w", "(float)(" + weight_expr + ")");
            df = df.Define("wx", "w * x");
            df = df.Define("wx2", "wx * x");
            auto hist_fdx = df.Histo1D<float>({"", "", n_bins, bins.data()}, "x", "w");
            auto hist_fxdx = df.Histo1D<float>({"", "", n_bins, bins.data()}, "x", "wx");
            auto hist_fx2dx = df.Histo1D<float>({"", "", n_bins, bins.data()}, "x", "wx2");
            rew_hist = hist_fdx.GetValue();

            for (int i = 0; i < n_bins; ++i)
            {
                fdx[i] = hist_fdx->GetBinContent(i + 1);
                fxdx[i] = hist_fxdx->GetBinContent(i + 1);
                fx2dx[i] = hist_fx2dx->GetBinContent(i + 1);
            }
        }
        /* */

        /* Get const. histogram */
        {
            TChain chain("nominal_Loose");
            for (auto &s : const_samples)
            {
                std::string path = base_path + "/" + s + ".root";
                chain.Add(path.c_str());
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
            df = df.Filter(cut.second);
            df = df.Define("x", "(float)(" + reweight_var + ")");
            df = df.Define("w", "(float)(" + weight_expr + ")");
            const_hist = df.Histo1D<float>({"", "", n_bins, bins.data()}, "x", "w").GetValue();
        }
        /* */

        /* Get data histogram */
        {
            TChain chain("nominal_Loose");
            for (auto &s : data_samples)
            {
                std::string path = base_path + "/" + s + ".root";
                chain.Add(path.c_str());
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
            df = df.Filter(cut.second);
            df = df.Define("x", "(float)(" + reweight_var + ")");
            data_hist = df.Histo1D<float>({"", "", n_bins, bins.data()}, "x").GetValue();
        }
        /* */

        // Substract const values from data histogram
        if (!data_hist.Add(&const_hist, -1.))
        {
            std::cerr << "ERROR: Histogram addition failed" << std::endl;
            return EXIT_FAILURE;
        }

        // Compute ratio
        if (!data_hist.Divide(&rew_hist))
        {
            std::cerr << "ERROR: Histogram division failed" << std::endl;
            return EXIT_FAILURE;
        }

        for (int i = 1; i <= n_bins; ++i)
            rew_factors[cut.first].push_back(data_hist.GetBinContent(i));

        /* Interpolate reweighting */
        std::cerr << "  Interpolating ..." << std::endl;

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
            y(eqn_idx) = rew_factors[cut.first][cond_idx];
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

        // std::cout << std::endl;
        // std::cout << "Matrix:" << std::endl;
        // for (int i = 0; i < 57; ++i)
        // {
        //     for (int j = 0; j < 57; ++j)
        //         std::cout << A(i, j) << ", ";
        //     std::cout << std::endl;
        // }
        // // std::cout << A << std::endl;
        // std::cout << std::endl;
        // std::cout << "y:" << std::endl;
        // std::cout << y << std::endl;
        // std::cout << std::endl;
        // std::cout << "x:" << std::endl;
        // std::cout << x << std::endl;
        // std::cout << std::endl;

        // std::cout << std::endl;
        // std::cout << "Test:" << std::endl;
        // std::cout << A * x - y << std::endl;
        // std::cout << std::endl;

        // Split coefficients
        // std::vector<float> a(x.data() + 0 * n_bins, x.data() + 1 * n_bins);
        // std::vector<float> b(x.data() + 1 * n_bins, x.data() + 2 * n_bins);
        // std::vector<float> c(x.data() + 2 * n_bins, x.data() + 3 * n_bins);
        // std::cout << std::endl;

        // for (int i = 0; i < n_bins; ++i)
        //     std::cout << a[i] << "*x2 + " << b[i] << "*x + " << c[i] << std::endl;

        // for (size_t i = 0; i < bins.size(); ++i)
        //     std::cout << bins[i] << std::endl;
    }

    std::vector<std::string> r_selection;
    std::vector<std::string> r_weight_factor;
    for (auto cut : nJetsCuts)
    {
        auto &bins = rew_bins[cut.first];
        int n_bins = bins.size() - 1;
        for (int i = 0; i < n_bins; ++i)
        {
            std::string s = cut.second + " && (" +
                            reweight_var + ") >= " + std::to_string(bins[i]) + " && (" +
                            reweight_var + ") < " + std::to_string(bins[i + 1]);
            r_selection.push_back(s);
            std::cout << s << std::endl;
        }
    }

    return EXIT_SUCCESS;
}


/*
g++ output.cpp -o output `root-config --cflags --libs`

*/

#include <iostream>
#include <vector>
#include "TFile.h"
#include "TTree.h"
#include <fstream>

using namespace std;

void printVectorContents() {

  // Open the ROOT file
  TFile* inputFile = new TFile("reweighting_1l.root", "READ");
  if (!inputFile->IsOpen()) {
    cerr << "ERROR: Could not open input file." << endl;
    return;
  }

  // Get the "selection" branch
  TTree* selectionTree = (TTree*)inputFile->Get("selection");
  if (!selectionTree) {
    cerr << "ERROR: Could not find 'selection' branch in input file." << endl;
    inputFile->Close();
    return;
  }
  TBranch* selectionBranch = selectionTree->GetBranch("selection");
  if (!selectionBranch) {
    cerr << "ERROR: Could not find 'selection' branch in input file." << endl;
    inputFile->Close();
    return;
  }

  // Create a vector to hold the contents of the "selection" branch
  vector<string>* selection = new vector<string>();

  // Set the branch address to the vector
  selectionBranch->SetAddress(&selection);

  // Print out the contents of the "selection" vector
  cout << "Selection:" << endl;
  for (Long64_t i = 0; i < selectionTree->GetEntries(); ++i) {
    selectionTree->GetEntry(i);
    for (const auto& s : *selection) {
      cout << s << endl;
    }
  }

  // Get the "factors" branch
  TTree* factorsTree = (TTree*)inputFile->Get("factors");
  if (!factorsTree) {
    cerr << "ERROR: Could not find 'factors' branch in input file." << endl;
    inputFile->Close();
    delete selection;
    return;
  }
  TBranch* factorsBranch = factorsTree->GetBranch("factors");
  if (!factorsBranch) {
    cerr << "ERROR: Could not find 'factors' branch in input file." << endl;
    inputFile->Close();
    delete selection;
    return;
  }

  // Create a vector to hold the contents of the "factors" branch
  vector<string>* factors = new vector<string>();

  // Set the branch address to the vector
  factorsBranch->SetAddress(&factors);

  // Print out the contents of the "factors" vector
  cout << "Factors:" << endl;
  for (Long64_t i = 0; i < factorsTree->GetEntries(); ++i) {
    factorsTree->GetEntry(i);
    for (const auto& s : *factors) {
      cout << s << endl;
    }
  }

  // Write the contents of the "selection" vector to a text file
  ofstream outputFile("reweighting_1l.txt");
  if (outputFile.is_open()) {
    for (Long64_t i = 0; i < selectionTree->GetEntries(); ++i) {
      selectionTree->GetEntry(i);
      for (const auto& s : *selection) {
        outputFile << s << endl;
      }
    }
    outputFile.close();
    cout << "Selection written to selection.txt" << endl;
  } else {
    cerr << "ERROR: Could not open output file." << endl;
  }

  // Clean up
  delete factors;
  delete selection;
  inputFile->Close();
}

int main() {
  printVectorContents();
  return 0;
}

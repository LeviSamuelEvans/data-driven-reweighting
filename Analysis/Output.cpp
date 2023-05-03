
/*
g++ Output.cpp -o myExecutable `root-config --cflags --libs`

*/

#include <iostream>
#include <vector>
#include "TFile.h"
#include "TTree.h"

using namespace std;

void printVectorContents() {

  
  TFile* inputFile = new TFile("inputFile.root", "READ");

  // Create TTree from the selection vector
  vector<string>* selection = nullptr;
  TTree* selectionTree = (TTree*)inputFile->Get("selection");
  selectionTree->SetBranchAddress("selection", &selection);

  // Print the selection vector output 
  cout << "Contents of selection vector:" << endl;
  selectionTree->Scan("selection");

  // Create  TTree from the factorsvector
  vector<string>* factors = nullptr;
  TTree* factorsTree = (TTree*)inputFile->Get("factors");
  factorsTree->SetBranchAddress("factors", &factors);

  // Print the factors vector
  cout << "Contents of factors vector:" << endl;
  factorsTree->Scan("factors");

  
  delete factorsTree;
  delete selectionTree;
  delete inputFile;
}

int main() {
  printVectorContents();
  return 0;
}

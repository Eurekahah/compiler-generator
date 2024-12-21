#include <iostream>

#include "CompilerGenerator.h"

using namespace std;

int main() {
  CompilerGenerator generator;

  cout << "Loading grammar..." << endl;
  if (!generator.loadGrammar("source/lexSynAnalysis/grammar.txt")) {
    cout << "Failed to load grammar!" << endl;
    return 1;
  }
  cout << "Grammar loaded successfully!" << endl;

  cout << "\nCalculating and printing First sets:" << endl;
  generator.computeFirstSets();
  generator.printFirstSets();

  cout << "\nCalculating and printing Follow sets:" << endl;
  generator.computeFollowSets();
  generator.printFollowSets();

  cout << "\nConstructing and printing parsing table:" << endl;
  generator.constructParseTable();
  generator.printParseTable();

  cout << "\nValidating grammar and generating compiler..." << endl;
  if (!generator.validateGrammar()) {
    cout << "Failed to validate grammar!" << endl;
    return 1;
  }

  if (!generator.generateCompiler("source/lexSynAnalysis/output")) {
    cout << "Failed to generate compiler!" << endl;
    return 1;
  }
  cout << "Compiler generated successfully!" << endl;

  cout << "\nTesting syntax analysis and intermediate code generation..."
       << endl;

  // Test case 1: Variable declaration
  vector<pair<string, Attribute>> input1 = {
      {"type", Attribute("int")}, {"id", Attribute("x")}, {";", Attribute()}};

  if (!generator.parse(input1)) {
    cout << "Syntax analysis failed!" << endl;
    return 1;
  }

  // Test case 2: Assignment
  vector<pair<string, Attribute>> input2 = {{"id", Attribute("x")},
                                            {"=", Attribute()},
                                            {"num", Attribute("5")},
                                            {";", Attribute()}};

  if (!generator.parse(input2)) {
    cout << "Syntax analysis failed!" << endl;
    return 1;
  }

  // Test case 3: Complex expression
  vector<pair<string, Attribute>> input3 = {
      {"id", Attribute("x")}, {"=", Attribute()}, {"(", Attribute()},
      {"id", Attribute("x")}, {"+", Attribute()}, {"num", Attribute("3")},
      {")", Attribute()},     {"*", Attribute()}, {"num", Attribute("2")},
      {";", Attribute()}};

  if (!generator.parse(input3)) {
    cout << "Syntax analysis failed!" << endl;
    return 1;
  }

  cout << "\nGenerated intermediate code:" << endl;
  generator.printIntermediateCode();

  // Save intermediate code to file
  generator.saveIntermediateCode(
      "source/lexSynAnalysis/output/intermediate_code.txt");
  cout << "Intermediate code has been saved to file." << endl;

  return 0;
}
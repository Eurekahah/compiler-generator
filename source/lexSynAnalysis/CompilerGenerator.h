#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// Quadruple structure for intermediate code
struct Quadruple {
  string op;
  string arg1;
  string arg2;
  string result;

  Quadruple(string o, string a1, string a2, string r)
      : op(o), arg1(a1), arg2(a2), result(r) {}
};

// Intermediate code generator
class IntermediateCodeGenerator {
 public:
  void emit(const string& op, const string& arg1, const string& arg2,
            const string& result) {
    quadruples.push_back(Quadruple(op, arg1, arg2, result));
  }

  string newTemp() { return "t" + to_string(tempCount++); }

  void printCode() const {
    cout << "Generated intermediate code:" << endl;
    for (size_t i = 0; i < quadruples.size(); ++i) {
      const auto& q = quadruples[i];
      cout << i << ": (" << q.op << ", " << q.arg1 << ", " << q.arg2 << ", "
           << q.result << ")" << endl;
    }
  }

  void saveToFile(const string& filename) const {
    ofstream file(filename);
    if (file.is_open()) {
      for (size_t i = 0; i < quadruples.size(); ++i) {
        const auto& q = quadruples[i];
        file << i << ": (" << q.op << ", " << q.arg1 << ", " << q.arg2 << ", "
             << q.result << ")" << endl;
      }
      file.close();
    }
  }

 private:
  vector<Quadruple> quadruples;
  int tempCount = 0;
};

enum class SymbolType { TERMINAL, NON_TERMINAL, EPSILON, ACTION };

class Symbol {
 public:
  string name;
  SymbolType type;

  Symbol(string n, SymbolType t) : name(n), type(t) {}
  bool isTerminal() const { return type == SymbolType::TERMINAL; }
};

struct Production {
  Symbol left;
  vector<Symbol> right;
  vector<string> semanticActions;

  Production() : left("", SymbolType::NON_TERMINAL) {}
  Production(Symbol l, vector<Symbol> r) : left(l), right(r) {}
  Production(Symbol l, vector<Symbol> r, vector<string> actions)
      : left(l), right(r), semanticActions(actions) {}
};

class Attribute {
 public:
  string value;
  string type;
  string extra;

  Attribute() : value(""), type(""), extra("") {}
  Attribute(string v) : value(v), type(""), extra("") {}
  Attribute(string v, string t, string e) : value(v), type(t), extra(e) {}
};

class CompilerGenerator {
 public:
  bool loadGrammar(const string& filename);
  void computeFirstSets();
  void computeFollowSets();
  void constructParseTable();
  bool validateGrammar();
  bool generateCompiler(const string& outputDir);
  bool parse(const vector<pair<string, Attribute>>& input);

  void printFirstSets() const;
  void printFollowSets() const;
  void printParseTable() const;
  void printIntermediateCode() const { codeGen.printCode(); }
  void saveIntermediateCode(const string& filename) const {
    codeGen.saveToFile(filename);
  }

 private:
  vector<Production> productions;
  set<string> terminals;
  set<string> nonTerminals;
  map<string, set<string>> firstSets;
  map<string, set<string>> followSets;
  map<pair<string, string>, Production> parseTable;
  IntermediateCodeGenerator codeGen;

  set<string> getFirst(const vector<Symbol>& symbols);
  void executeSemanticAction(const string& action,
                             map<string, Attribute>& symbolTable);
};
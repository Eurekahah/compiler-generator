#include "CompilerGenerator.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

bool CompilerGenerator::loadGrammar(const string& filename) {
  ifstream file(filename);
  if (!file.is_open()) {
    cout << "Failed to open grammar file: " << filename << endl;
    return false;
  }

  cout << "Reading grammar rules..." << endl;
  string line;
  while (getline(file, line)) {
    // Skip comments and empty lines
    if (line.empty() || line[0] == '#' || line[0] == '\n') {
      continue;
    }

    cout << "Processing line: " << line << endl;

    // Parse production
    size_t arrowPos = line.find("->");
    if (arrowPos == string::npos) {
      cout << "Invalid line format (no arrow found)" << endl;
      continue;
    }

    // Get left-hand side
    string leftStr = line.substr(0, arrowPos);
    leftStr.erase(remove_if(leftStr.begin(), leftStr.end(), ::isspace),
                  leftStr.end());
    Symbol left(leftStr, SymbolType::NON_TERMINAL);
    nonTerminals.insert(leftStr);
    cout << "Left-hand side: " << leftStr << endl;

    // Get right-hand side
    string rightStr = line.substr(arrowPos + 2);
    stringstream ss(rightStr);
    string token;
    vector<Symbol> rightSymbols;

    cout << "Right-hand side tokens: ";
    while (ss >> token) {
      cout << token << " ";
      if (token == "|") {
        // Create new production
        cout << "\nCreating production: " << leftStr << " -> ";
        for (const auto& sym : rightSymbols) {
          cout << sym.name << " ";
        }
        cout << endl;

        if (!rightSymbols.empty()) {
          Production prod(left, rightSymbols);
          productions.push_back(prod);
        }
        rightSymbols.clear();
      } else {
        // Process token
        if (token == "@epsilon") {
          rightSymbols.push_back(Symbol("", SymbolType::EPSILON));
        } else if (token[0] == '@') {
          rightSymbols.push_back(Symbol(token, SymbolType::ACTION));
        } else {
          // Determine if terminal or non-terminal
          SymbolType type = isupper(token[0]) ? SymbolType::NON_TERMINAL
                                              : SymbolType::TERMINAL;
          if (type == SymbolType::TERMINAL) {
            terminals.insert(token);
          }
          rightSymbols.push_back(Symbol(token, type));
        }
      }
    }

    // Add last production
    if (!rightSymbols.empty()) {
      cout << "Creating final production: " << leftStr << " -> ";
      for (const auto& sym : rightSymbols) {
        cout << sym.name << " ";
      }
      cout << endl;

      Production prod(left, rightSymbols);
      productions.push_back(prod);
    }
  }

  file.close();
  return true;
}

void CompilerGenerator::computeFirstSets() {
  // Initialize First sets
  for (const auto& terminal : terminals) {
    firstSets[terminal].insert(terminal);
  }
  for (const auto& nonTerminal : nonTerminals) {
    firstSets[nonTerminal] = set<string>();
  }

  bool changed;
  do {
    changed = false;
    for (const auto& prod : productions) {
      string leftNT = prod.left.name;

      // If right side is empty or contains only epsilon
      if (prod.right.empty() || (prod.right.size() == 1 &&
                                 prod.right[0].type == SymbolType::EPSILON)) {
        if (firstSets[leftNT].insert("epsilon").second) {
          changed = true;
        }
        continue;
      }

      // Calculate First set of right-hand side
      set<string> rightFirst = getFirst(prod.right);
      for (const auto& symbol : rightFirst) {
        if (firstSets[leftNT].insert(symbol).second) {
          changed = true;
        }
      }
    }
  } while (changed);
}

set<string> CompilerGenerator::getFirst(const vector<Symbol>& symbols) {
  set<string> result;
  bool allHaveEpsilon = true;

  for (const auto& symbol : symbols) {
    if (symbol.type == SymbolType::EPSILON) {
      result.insert("epsilon");
      continue;
    }

    set<string> symbolFirst = firstSets[symbol.name];
    bool hasEpsilon = (symbolFirst.find("epsilon") != symbolFirst.end());

    // ��ӳ�epsilon֮������з���
    for (const auto& s : symbolFirst) {
      if (s != "epsilon") {
        result.insert(s);
      }
    }

    if (!hasEpsilon) {
      allHaveEpsilon = false;
      break;
    }
  }

  if (allHaveEpsilon) {
    result.insert("epsilon");
  }

  return result;
}

void CompilerGenerator::computeFollowSets() {
  // ��ʼ��Follow��
  for (const auto& nonTerminal : nonTerminals) {
    followSets[nonTerminal] = set<string>();
  }

  // Ϊ��ʼ����ӽ�������
  followSets[productions[0].left.name].insert("$");

  bool changed;
  do {
    changed = false;
    for (const auto& prod : productions) {
      for (size_t i = 0; i < prod.right.size(); i++) {
        if (prod.right[i].type != SymbolType::NON_TERMINAL) continue;

        string currentNT = prod.right[i].name;
        set<string> firstOfRest;

        // ����������ŵ�First��
        if (i < prod.right.size() - 1) {
          vector<Symbol> rest(prod.right.begin() + i + 1, prod.right.end());
          firstOfRest = getFirst(rest);
        }

        // ���First(��)�г�epsilon֮������з��ŵ�Follow(B)
        for (const auto& symbol : firstOfRest) {
          if (symbol != "epsilon" &&
              followSets[currentNT].insert(symbol).second) {
            changed = true;
          }
        }

        // ���First(��)��epsilon��B�����һ�����ţ�
        // ���Follow(A)�����з��ż��뵽Follow(B)
        if (firstOfRest.find("epsilon") != firstOfRest.end() ||
            i == prod.right.size() - 1) {
          for (const auto& symbol : followSets[prod.left.name]) {
            if (followSets[currentNT].insert(symbol).second) {
              changed = true;
            }
          }
        }
      }
    }
  } while (changed);
}

void CompilerGenerator::constructParseTable() {
  for (const auto& prod : productions) {
    set<string> firstOfRight = getFirst(prod.right);

    // For each terminal in First set
    for (const auto& terminal : firstOfRight) {
      if (terminal != "epsilon") {
        parseTable[{prod.left.name, terminal}] = prod;
      }
    }

    // If First set contains epsilon, add production for each terminal in Follow
    // set
    if (firstOfRight.find("epsilon") != firstOfRight.end()) {
      for (const auto& terminal : followSets[prod.left.name]) {
        parseTable[{prod.left.name, terminal}] = prod;
      }
    }
  }
}

bool CompilerGenerator::validateGrammar() {
  // ����First����Follow��
  computeFirstSets();
  computeFollowSets();

  // ����Ԥ�������
  constructParseTable();

  // ����Ƿ���ڳ�ͻ
  map<pair<string, string>, vector<Production>> conflicts;
  for (const auto& prod : productions) {
    set<string> firstOfRight = getFirst(prod.right);

    for (const auto& terminal : firstOfRight) {
      if (terminal != "epsilon") {
        conflicts[{prod.left.name, terminal}].push_back(prod);
      }
    }

    if (firstOfRight.find("epsilon") != firstOfRight.end()) {
      for (const auto& terminal : followSets[prod.left.name]) {
        conflicts[{prod.left.name, terminal}].push_back(prod);
      }
    }
  }

  // ����Ƿ��ж������ʽ��Ӧͬһ��Ԥ���������
  bool isLL1 = true;
  for (const auto& entry : conflicts) {
    if (entry.second.size() > 1) {
      cout << "Grammar is not LL(1): Conflict found for " << entry.first.first
           << " and " << entry.first.second << endl;
      isLL1 = false;
    }
  }

  return isLL1;
}

bool CompilerGenerator::generateCompiler(const string& outputDir) {
  if (!validateGrammar()) {
    cout << "Cannot generate compiler: Grammar is not LL(1)" << endl;
    return false;
  }

  // �������Ŀ¼
  fs::create_directories(outputDir);

  // ���ɽ���������
  string parserPath = outputDir + "/Parser.h";
  ofstream parser(parserPath);
  if (!parser.is_open()) {
    cout << "Failed to create parser file: " << parserPath << endl;
    return false;
  }

  // ���ɽ�����ͷ�ļ�
  parser << "#ifndef PARSER_H\n#define PARSER_H\n\n";
  parser << "#include <string>\n#include <vector>\n#include <map>\n#include "
            "<stack>\n";
  parser << "using namespace std;\n\n";

  // ���ɷ�������ö��
  parser << "enum class Symbol {\n";
  parser << "    epsilon,\n";  // ���epsilon��Ϊ��һ������
  for (const auto& terminal : terminals) {
    parser << "    " << terminal << ",\n";
  }
  for (const auto& nonTerminal : nonTerminals) {
    parser << "    " << nonTerminal << ",\n";
  }
  parser << "};\n\n";

  // ���ɽ�������
  parser << "class Parser {\n";
  parser << "private:\n";
  parser << "    // Ԥ�������\n";
  parser << "    map<pair<Symbol,Symbol>, vector<Symbol>> parseTable;\n\n";

  parser << "public:\n";
  parser << "    Parser();\n";
  parser << "    bool parse(const vector<Symbol>& input);\n";
  parser << "};\n\n";

  parser << "#endif\n";
  parser.close();

  // ���ɽ�����ʵ��
  ofstream parserCpp(outputDir + "/Parser.cpp");
  if (!parserCpp.is_open()) {
    cout << "Failed to create parser implementation file" << endl;
    return false;
  }

  parserCpp << "#include \"Parser.h\"\n\n";
  parserCpp << "Parser::Parser() {\n";
  // ��ʼ��Ԥ�������
  for (const auto& entry : parseTable) {
    parserCpp << "    parseTable[{Symbol::" << entry.first.first
              << ", Symbol::" << entry.first.second << "}] = {";
    for (const auto& symbol : entry.second.right) {
      if (symbol.type == SymbolType::EPSILON) {
        parserCpp << "Symbol::epsilon, ";
      } else {
        parserCpp << "Symbol::" << symbol.name << ", ";
      }
    }
    parserCpp << "};\n";
  }
  parserCpp << "}\n\n";

  // ʵ�ֽ�������
  parserCpp << "bool Parser::parse(const vector<Symbol>& input) {\n";
  parserCpp << "    stack<Symbol> stack;\n";
  parserCpp << "    stack.push(Symbol::" << productions[0].left.name << ");\n";
  parserCpp << "    size_t inputPos = 0;\n\n";

  parserCpp << "    while (!stack.empty() && inputPos <= input.size()) {\n";
  parserCpp << "        if (stack.empty()) return inputPos >= input.size();\n";
  parserCpp << "        Symbol top = stack.top();\n";
  parserCpp << "        stack.pop();\n\n";

  parserCpp << "        if (top == Symbol::epsilon) continue;\n\n";

  parserCpp << "        if (inputPos >= input.size()) return false;\n";
  parserCpp << "        Symbol current = input[inputPos];\n\n";

  parserCpp << "        if (top == current) {\n";
  parserCpp << "            inputPos++;\n";
  parserCpp << "            continue;\n";
  parserCpp << "        }\n\n";

  parserCpp << "        auto it = parseTable.find({top, current});\n";
  parserCpp << "        if (it == parseTable.end()) return false;\n\n";

  parserCpp << "        const auto& production = it->second;\n";
  parserCpp << "        for (auto it = production.rbegin(); it != "
               "production.rend(); ++it) {\n";
  parserCpp << "            stack.push(*it);\n";
  parserCpp << "        }\n";
  parserCpp << "    }\n\n";

  parserCpp << "    return stack.empty() && inputPos >= input.size();\n";
  parserCpp << "}\n";

  parserCpp.close();

  return true;
}

void CompilerGenerator::printFirstSets() const {
  cout << "First sets:" << endl;
  for (const auto& entry : firstSets) {
    cout << entry.first << ": { ";
    for (const auto& symbol : entry.second) {
      cout << symbol << " ";
    }
    cout << "}" << endl;
  }
}

void CompilerGenerator::printFollowSets() const {
  cout << "Follow sets:" << endl;
  for (const auto& entry : followSets) {
    cout << entry.first << ": { ";
    for (const auto& symbol : entry.second) {
      cout << symbol << " ";
    }
    cout << "}" << endl;
  }
}

void CompilerGenerator::printParseTable() const {
  cout << "Parse table:" << endl;
  for (const auto& entry : parseTable) {
    cout << "(" << entry.first.first << ", " << entry.first.second << ") -> ";
    cout << entry.second.left.name << " -> ";
    for (const auto& symbol : entry.second.right) {
      cout << symbol.name << " ";
    }
    cout << endl;
  }
}

bool CompilerGenerator::parse(const vector<pair<string, Attribute>>& input) {
  cout << "Starting parsing..." << endl;

  // Initialize parsing stack and symbol table
  vector<Symbol> stack;
  map<string, Attribute> symbolTable;

  // Push initial symbol
  stack.push_back(Symbol("$", SymbolType::TERMINAL));
  stack.push_back(Symbol("P", SymbolType::NON_TERMINAL));
  cout << "\nPushing start symbol onto stack: P" << endl;

  size_t inputPos = 0;
  int symbolPos = 0;  // Track position for symbol table

  while (!stack.empty()) {
    cout << "\nCurrent state:" << endl;
    Symbol top = stack.back();
    stack.pop_back();

    cout << "Stack top: " << top.name << " (type: "
         << (top.type == SymbolType::TERMINAL       ? "terminal"
             : top.type == SymbolType::NON_TERMINAL ? "non-terminal"
             : top.type == SymbolType::ACTION       ? "action"
                                                    : "epsilon")
         << ")" << endl;

    // Check if this is a semantic action (starts with @)
    if (!top.name.empty() && top.name[0] == '@') {
      cout << "Executing semantic action: " << top.name << endl;
      executeSemanticAction(top.name, symbolTable);
      continue;
    }

    // Handle epsilon production
    if (top.type == SymbolType::EPSILON) {
      cout << "Skipping epsilon production" << endl;
      continue;
    }

    if (inputPos >= input.size()) {
      if (top.name == "$") {
        return true;
      }
      cout << "Syntax error: unexpected end of input" << endl;
      return false;
    }

    string currentToken = input[inputPos].first;
    Attribute currentAttr = input[inputPos].second;

    cout << "Current input: " << currentToken << endl;

    if (top.isTerminal()) {
      if (top.name == currentToken) {
        cout << "Matched terminal: " << currentToken << endl;
        // Store attribute in symbol table with relative position
        string pos = to_string(symbolPos);
        symbolTable[pos].value = currentAttr.value;
        symbolTable[pos].type = currentAttr.type;
        symbolPos++;
        inputPos++;
      } else {
        cout << "Syntax error: expected " << top.name << " but got "
             << currentToken << endl;
        return false;
      }
    } else {
      // Look up production in parse table
      cout << "Looking up production for non-terminal " << top.name
           << " with input " << currentToken << endl;
      auto it = parseTable.find(make_pair(top.name, currentToken));
      if (it == parseTable.end()) {
        cout << "Syntax error: no applicable production found" << endl;
        return false;
      }

      cout << "Found production: " << it->second.left.name << " -> ";
      for (const auto& sym : it->second.right) {
        cout << sym.name << " ";
      }
      cout << endl;

      // Push production right-hand side in reverse order
      const Production& prod = it->second;
      for (auto it = prod.right.rbegin(); it != prod.right.rend(); ++it) {
        cout << "Pushing symbol onto stack: " << it->name << endl;
        stack.push_back(*it);
      }
    }
  }

  return inputPos == input.size();
}

void CompilerGenerator::executeSemanticAction(
    const string& action, map<string, Attribute>& symbolTable) {
  // Remove @ from action name
  string actionName = action.substr(1);

  // ��ӡ��ǰ�����ͷ��ű������״̬
  cout << "\n=== Executing Semantic Action: " << actionName << " ===" << endl;
  cout << "Symbol table before action:" << endl;
  for (const auto& entry : symbolTable) {
    cout << "  " << entry.first << ": " << entry.second.value << endl;
  }

  if (actionName == "declare") {
    // Handle variable declaration
    string type = symbolTable["0"].value;  // type
    string id = symbolTable["1"].value;    // id
    cout << "Declaring variable: " << type << " " << id << endl;
    codeGen.emit("declare", type, "", id);
  } else if (actionName == "assign") {
    // Handle assignment
    string id = symbolTable["0"].value;  // id (target variable)
    string expr;
    // ����Ƿ���������������ֵ
    if (symbolTable["2"].value.empty()) {
      expr = symbolTable["-1"].value;  // ʹ�ñ��ʽ�Ľ��
      // ����Ŀ�����Ϊԭʼ�ı�ʶ��
      id = symbolTable["3"].value;
    } else {
      expr = symbolTable["2"].value;  // ʹ������������
    }
    cout << "Assigning value: " << expr << " -> " << id << endl;
    codeGen.emit("=", expr, "", id);
  } else if (actionName == "add") {
    // Handle addition
    string left = symbolTable["0"].value;   // left operand
    string right = symbolTable["5"].value;  // right operand (after +)
    string temp = codeGen.newTemp();
    cout << "Adding: " << left << " + " << right << " -> " << temp << endl;
    codeGen.emit("+", left, right, temp);
    symbolTable["$$"].value = temp;

    // ���¸��ڵ��ֵ�����ں�������
    symbolTable["-1"].value = temp;
    // ���µ�ǰ�ڵ��ֵ�����ں�������
    symbolTable["0"].value = temp;
  } else if (actionName == "sub") {
    // Handle subtraction
    string left = symbolTable["0"].value;   // left operand
    string right = symbolTable["5"].value;  // right operand (after -)
    string temp = codeGen.newTemp();
    cout << "Subtracting: " << left << " - " << right << " -> " << temp << endl;
    codeGen.emit("-", left, right, temp);
    symbolTable["$$"].value = temp;

    // ���¸��ڵ��ֵ�����ں�������
    symbolTable["-1"].value = temp;
    // ���µ�ǰ�ڵ��ֵ�����ں�������
    symbolTable["0"].value = temp;
  } else if (actionName == "mul") {
    // Handle multiplication
    string left = symbolTable["0"].value;   // left operand
    string right = symbolTable["8"].value;  // right operand (after *)
    string temp = codeGen.newTemp();
    cout << "Multiplying: " << left << " * " << right << " -> " << temp << endl;
    codeGen.emit("*", left, right, temp);
    symbolTable["$$"].value = temp;

    // ���¸��ڵ��ֵ�����ں�������
    symbolTable["-1"].value = temp;
    // ���µ�ǰ�ڵ��ֵ�����ں�������
    symbolTable["0"].value = temp;
  } else if (actionName == "div") {
    // Handle division
    string left = symbolTable["0"].value;   // left operand
    string right = symbolTable["8"].value;  // right operand (after /)
    string temp = codeGen.newTemp();
    cout << "Dividing: " << left << " / " << right << " -> " << temp << endl;
    codeGen.emit("/", left, right, temp);
    symbolTable["$$"].value = temp;

    // ���¸��ڵ��ֵ�����ں�������
    symbolTable["-1"].value = temp;
    // ���µ�ǰ�ڵ��ֵ�����ں�������
    symbolTable["0"].value = temp;
  } else if (actionName == "value") {
    // Handle value - propagate value upward
    if (symbolTable["0"].value == "(") {
      // �������ű��ʽ��ʹ�����ű��ʽ�Ľ��
      string innerValue = symbolTable["1"].value;
      cout << "Processing parentheses: ( " << innerValue << " )" << endl;
      symbolTable["$$"].value = innerValue;
      // ���¸��ڵ��ֵ�����ں�������
      symbolTable["-1"].value = innerValue;
      // ���µ�ǰ�ڵ��ֵ�����ں�������
      symbolTable["0"].value = innerValue;
    } else {
      // �����ʶ��������
      string value = symbolTable["0"].value;
      cout << "Processing value: " << value << endl;
      symbolTable["$$"].value = value;
      // ���¸��ڵ��ֵ�����ں�������
      symbolTable["-1"].value = value;
      // ���µ�ǰ�ڵ��ֵ�����ں�������
      symbolTable["0"].value = value;
    }
  }

  // ��ӡ����ִ�к�ķ��ű�״̬
  cout << "Symbol table after action:" << endl;
  for (const auto& entry : symbolTable) {
    cout << "  " << entry.first << ": " << entry.second.value << endl;
  }
  cout << "=== End of Semantic Action ===" << endl;
}
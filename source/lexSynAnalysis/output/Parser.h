#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <map>
#include <stack>
using namespace std;

enum class Symbol {
    epsilon,
    (,
    ),
    *,
    +,
    -,
    /,
    ;,
    =,
    id,
    num,
    type,
    A,
    D,
    E,
    E',
    F,
    P,
    S,
    T,
    T',
};

class Parser {
private:
    // Ô¤²â·ÖÎö±í
    map<pair<Symbol,Symbol>, vector<Symbol>> parseTable;

public:
    Parser();
    bool parse(const vector<Symbol>& input);
};

#endif
